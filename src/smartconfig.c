#include "esp_common.h"

#include "net.h"
#include "timer.h"
#include "airkiss.h"
#include "smartconfig.h"
#include <espressif/smartconfig.h>

const airkiss_config_t airkiss_conf = {
    (airkiss_memset_fn)&memset,
    (airkiss_memcpy_fn)&memcpy,
    (airkiss_memcmp_fn)&memcmp,
    (airkiss_printf_fn)&printf,
};

bool ICACHE_FLASH_ATTR timer_airkiss_discover(void *params)
{
    int err = 0;
    char buff[200];
    ushort buff_len = 0;
    net_conn_t conn = params;
    static uint8 send_count = 0;

    if ((++send_count) > 30)
    {
        send_count = 0;
        return false;
    }

    buff_len = sizeof(buff);
    err = airkiss_lan_pack(AIRKISS_LAN_SSDP_NOTIFY_CMD, AIRKISS_DEVICE_TYPE, AIRKISS_DEVICE_ID, 0, 0, buff, &buff_len, &airkiss_conf);
    if (err != AIRKISS_LAN_PAKE_READY)
    {
        os_printf("Pack lan packet error!");
        return false;
    }

    err = net_write(conn, buff, buff_len);
    if (err != SOCKET_OK)
    {
        os_printf("UDP send error!");
        return false;
    }

    os_printf("Finish send notify!\n");
    return true;
}

void ICACHE_FLASH_ATTR on_smartconfig_status_change(sc_status status, void *pdata)
{
    switch (status)
    {
    case SC_STATUS_WAIT:
        printf("SC_STATUS_WAIT\n");
        break;
    case SC_STATUS_FIND_CHANNEL:
        printf("SC_STATUS_FIND_CHANNEL\n");
        break;
    case SC_STATUS_GETTING_SSID_PSWD:
        printf("SC_STATUS_GETTING_SSID_PSWD\n");
        break;
    case SC_STATUS_LINK:
        printf("SC_STATUS_LINK\n");
        struct station_config *sta_conf = pdata;

        wifi_station_set_config(sta_conf);
        wifi_station_disconnect();
        wifi_station_connect();
        break;
    case SC_STATUS_LINK_OVER:
        printf("SC_STATUS_LINK_OVER\n");
        if (NULL != pdata)
        {
            uint8 *remote_ip = pdata;
            printf("smartconfig remote ip: %d.%d.%d.%d\n", remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3]);
        }
        else
        {
            net_conn_t conn = net_dial("udp", "255.255.255.255:12476");
            timer_new(1000, timer_airkiss_discover, conn);
        }

        smartconfig_stop();
        break;
    default:
        printf("unknown smartconfig status: %d\n", status);
        break;
    }
}

void ICACHE_FLASH_ATTR task_smartconfig(void *params)
{
    //printf("start airkiss...\n");
    //airkiss_start(true);
    //printf("start smartconfig...\n");
    //smartconfig_start(on_smartconfig_status_change, 1);

    vTaskDelete(NULL);
}