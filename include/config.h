#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifndef LWIP_DNS
#define LWIP_DNS 1
#endif

// #define STATION_WIFI_SSID "HYS_WiFi_1"
// #define STATION_WIFI_PASSWD "huoys.com"
#define STATION_WIFI_SSID "Ziroom-1101"
#define STATION_WIFI_PASSWD "4001001111"

#define GPIO_LIGHT GPIO2
#define GPIO_LIGHT_SWITCH GPIO15

#define CONTROL_SERVER_PORT 65530
#define CONTROL_SERVER_ADDR "129.28.196.75" // "cn.coocn.cn"
#define CONTROL_READ_TIMEOUT 3000

#endif