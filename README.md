# HCMUT-CO3053-EmbeddedSystem-Project

Simulate a smart garden tracking system that aim to:

-   Collecting temperature and humidity value from sensor
-   Run a webserver to provide user a dashboard to track sensor value
-   Sensor value are realtime updated using websocket
-   Trigger action based on conditions (running the fogging system when humidity is too low, ...)

Currently, the sensors value are faked, user can integrate real sensors and modify the `sensor.c` source to read real value.

## Prerequisites

-   Docker
-   [ESP RFC server](https://github.com/espressif/esptool/releases/latest)
-   ESP32-WROOM-32 (not tested on other device but should work well)

## Serial port

On Linux

```shell
ls -la /sys/bus/usb-serial/devices/
```

On Windows

```shell
Get-PnpDevice -PresentOnly -Class Ports | Select-Object Name, Description, DeviceID | Format-List
```

## Remote serial port

Run the ESP RFC server with the serial port as argument

```
./esp_rfc2217_server /dev/ttyUSB0
```

> **NOTE**\
> Run with elevated privileges

## Configuration

User can change wifi station (the wifi that ESP device connect to) and wifi access point (the wifi that ESP device serving) configuration in `app/main/app.c`

By default wifi station is not configured, wifi access point name is `ESP32-AP-TEST` and password is `12345678`

## Flash project

```shell
docker run --rm -it -v $PWD/app/:/app/ -w /app/ docker.io/espressif/idf idf.py -p rfc2217://host.docker.internal:2217?ign_set_control flash monitor
```

## Dashboard

User have to check the console log when running flash to know where the dashboard is being hosted

```text
esp_netif_handlers: sta ip: 192.168.1.7, mask: 255.255.255.0, gw: 192.168.1.1
```

means that device is connected to configured wifi and user in same network can access dashboard at 192.168.1.7

```text
esp_netif_lwip: DHCP server started on interface WIFI_AP_DEF with IP: 192.168.4.1
```

means that device is serving an access point as configured, user can connect to network and access dashboard at 192.168.4.1
