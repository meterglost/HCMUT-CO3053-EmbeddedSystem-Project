idf_component_register(SRCS "sensor.c" "wifi.c" "web.c" "app.c" INCLUDE_DIRS "")
spiffs_create_partition_image(storage ../storage FLASH_IN_PROJECT)
