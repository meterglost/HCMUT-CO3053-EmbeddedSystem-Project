#include <string.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_spiffs.h"
#include "esp_http_server.h"

#include "esp_log.h"
#include "esp_check.h"

#include "web.h"

#include "sensor.h"

void load_storage()
{
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(
        &(esp_vfs_spiffs_conf_t){
            .base_path = "/storage",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = false
        }
    ));
}

char *load_file(const char *filename)
{
    char *path = malloc(strlen("/storage/web/") + strlen(filename) + 1);
    strcpy(path, "/storage/web/");
    strcat(path, filename);

    FILE *f = fopen(path, "r");
    free(path);

    if (f == NULL)
    {
        printf("File not found\n");
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = malloc(fsize + 1);
    fread(buffer, fsize, 1, f);
    fclose(f);

    buffer[fsize] = 0;
    return buffer;
}

esp_err_t http_handler(httpd_req_t *req)
{
    return httpd_resp_send(req, load_file(req->user_ctx), HTTPD_RESP_USE_STRLEN);
}

typedef struct {
    httpd_handle_t handle;
    int fd;
    uint8_t failed_count;
} ws_conn_t;

#ifndef WS_CONN_MAX
    #error WS_CONN_MAX is not defined
#endif

#define WS_FAILED_MAX 5

#define MSG_MAX_LEN 200

static ws_conn_t * ws_conn[WS_CONN_MAX];

esp_err_t ws_send(char * msg, size_t msg_len, httpd_handle_t handle, int fd)
{
    httpd_ws_frame_t *pkt = &(httpd_ws_frame_t){
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = NULL,
        .len = 0,
    };

    pkt->len = msg_len;
    pkt->payload = calloc(1, pkt->len + 1);
    memcpy(pkt->payload, msg, pkt->len);

    return httpd_ws_send_frame_async(handle, fd, pkt);
}

void ws_conn_upsert(httpd_handle_t handle, int fd)
{
    for (int i = 0; i < WS_CONN_MAX; i++)
    {
        if (ws_conn[i] != NULL && ws_conn[i]->fd == fd) {
            free(ws_conn[i]);
            ws_conn[i] = NULL;
        }
    }

    char msg[MSG_MAX_LEN];

    snprintf(
        msg, MSG_MAX_LEN, "{'tem':{'min':%.2f,'max':%.2f,'xpcd':%.2f},'hum':{'min':%.2f,'max':%.2f,'xpcd':%.2f}}",
        TEM_RMIN, TEM_RMAX, get_xpcd_tem(),
        HUM_RMIN, HUM_RMAX, get_xpcd_hum()
    );

    if (ws_send(msg, strlen(msg), handle, fd) != ESP_OK)
    {
        return;
    }

    for (int i = 0; i < WS_CONN_MAX; i++)
    {
        if (ws_conn[i] == NULL) {
            ws_conn[i] = malloc(sizeof(ws_conn_t));
            ws_conn[i]->failed_count = WS_FAILED_MAX;
            ws_conn[i]->handle = handle;
            ws_conn[i]->fd = fd;
            break;
        }
    }
}

void ws_conn_send(void *arg)
{
    char msg[MSG_MAX_LEN];

    for ( ; ; )
    {
        snprintf(msg, MSG_MAX_LEN, "{'tem':{'curr':%.2f},'hum':{'curr':%.2f}}", get_curr_tem(), get_curr_hum());

        uint8_t conn_count = 0;

        for (int i = 0; i < WS_CONN_MAX; i++)
        {
            if (ws_conn[i] != NULL) {
                conn_count++;
                if (ws_send(msg, strlen(msg), ws_conn[i]->handle, ws_conn[i]->fd) != ESP_OK)
                    ws_conn[i]->failed_count++;
            }
        }

        printf("%d connected\n", conn_count);

        for (int i = 0; i < WS_CONN_MAX; i++)
        {
            if (ws_conn[i] != NULL)
                if (ws_conn[i]->failed_count >= WS_FAILED_MAX)
                    ws_conn_upsert(ws_conn[i]->handle, ws_conn[i]->fd);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    vTaskDelete(NULL);
}

esp_err_t ws_recv(httpd_req_t *req)
{
    if (req->method == HTTP_GET) return ESP_OK;

    ws_conn_upsert(req->handle, httpd_req_to_sockfd(req));

    httpd_ws_frame_t *pkt = &(httpd_ws_frame_t){
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = NULL,
        .len = 0,
    };

    ESP_RETURN_ON_ERROR(httpd_ws_recv_frame(req, pkt, 0), "Websocket", "Failed to get packet length");
    ESP_RETURN_ON_FALSE(pkt->len > 0, ESP_OK, "Websocket", "Empty packet");

    char buffer[pkt->len + 1];
    pkt->payload = (uint8_t *)&buffer;

    ESP_RETURN_ON_FALSE(pkt->payload != NULL, ESP_ERR_NO_MEM, "Websocket", "Failed to allocate memory for packet content");
    ESP_RETURN_ON_ERROR(httpd_ws_recv_frame(req, pkt, pkt->len), "WebSocket", "Failed to get packet content");

    char *key = strtok((char *)pkt->payload, "=");
    char *val = strtok(NULL, "=");

    while (key != NULL && val != NULL)
    {
        if (strcmp(key, "tem_xpcd") == 0) set_xpcd_tem(strtof(val, NULL));
        if (strcmp(key, "hum_xpcd") == 0) set_xpcd_hum(strtof(val, NULL));
        key = strtok(NULL, "=");
        val = strtok(NULL, "=");
    }

    return ESP_OK;
}

void init_webserver()
{
    load_storage();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_register_uri_handler(server,
        &(httpd_uri_t){
            .uri = "/",
            .method = HTTP_GET,
            .is_websocket = false,
            .handler = http_handler,
            .user_ctx = "index.html",
        }
    );

    httpd_register_uri_handler(server,
        &(httpd_uri_t){
            .uri = "/ws",
            .method = HTTP_GET,
            .is_websocket = true,
            .handler = ws_recv,
            .user_ctx = NULL,
        }
    );

    xTaskCreate(ws_conn_send, "ws_conn_send", 8192, NULL, 5, NULL);
}
