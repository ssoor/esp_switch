/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "config.h"
#include "esp_common.h"

#include "wifi.h"
#include "timer.h"
#include "smartconfig.h"

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map)
    {
    case FLASH_SIZE_4M_MAP_256_256:
        rf_cal_sec = 128 - 5;
        break;

    case FLASH_SIZE_8M_MAP_512_512:
        rf_cal_sec = 256 - 5;
        break;

    case FLASH_SIZE_16M_MAP_512_512:
    case FLASH_SIZE_16M_MAP_1024_1024:
        rf_cal_sec = 512 - 5;
        break;

    case FLASH_SIZE_32M_MAP_512_512:
    case FLASH_SIZE_32M_MAP_1024_1024:
        rf_cal_sec = 1024 - 5;
        break;
    case FLASH_SIZE_64M_MAP_1024_1024:
        rf_cal_sec = 2048 - 5;
        break;
    case FLASH_SIZE_128M_MAP_1024_1024:
        rf_cal_sec = 4096 - 5;
        break;
    default:
        rf_cal_sec = 0;
        break;
    }

    return rf_cal_sec;
}

void on_wifi_event(System_Event_t *event)
{
    switch (event->event_id)
    {
    case EVENT_STAMODE_CONNECTED:
        printf("connect to ssid %s, channel %d\n",
               event->event_info.connected.ssid,
               event->event_info.connected.channel);
        break;
    case EVENT_STAMODE_DISCONNECTED:
        printf("disconnect from	ssid %s, reason %d\n",
               event->event_info.disconnected.ssid,
               event->event_info.disconnected.reason);
        break;
    case EVENT_STAMODE_AUTHMODE_CHANGE:
        printf("mode: %d -> %d\n",
               event->event_info.auth_change.old_mode,
               event->event_info.auth_change.new_mode);
        break;
    case EVENT_STAMODE_GOT_IP:
    {
        printf("ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR,
               IP2STR(&event->event_info.got_ip.ip),
               IP2STR(&event->event_info.got_ip.mask),
               IP2STR(&event->event_info.got_ip.gw));
        printf("\n");
        break;
    }
    case EVENT_SOFTAPMODE_STACONNECTED:
        printf("station: " MACSTR "join, AID = %d\n",
               MAC2STR(event->event_info.sta_connected.mac),
               event->event_info.sta_connected.aid);
        break;
    case EVENT_SOFTAPMODE_STADISCONNECTED:
        printf("station: " MACSTR "leave, AID = %d\n",
               MAC2STR(event->event_info.sta_disconnected.mac),
               event->event_info.sta_disconnected.aid);
        break;
    default:

        break;
    }
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    printf("SDK version:%s\n", system_get_sdk_version());

    wifi_set_opmode(STATION_MODE);
    wifi_set_event_handler_cb(on_wifi_event);

    wifi_connect(STATION_WIFI_SSID, STATION_WIFI_PASSWD, NULL);

    control_init();
    //xTaskCreate(task_smartconfig, "smartconfig", 256, NULL, 2, NULL);

    wifi_set_sleep_type(LIGHT_SLEEP_T);
    wifi_fpm_open();
}
