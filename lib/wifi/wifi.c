#include "esp_common.h"
#include "wifi.h"
#include "timer.h"

void ICACHE_FLASH_ATTR _internal_wifi_connect(struct station_config *conf)
{
    wifi_station_set_config(conf);

    wifi_station_disconnect();
    wifi_station_connect();
}

typedef struct __internal_scan_ctx_t
{
    sint8 rssi;
    timer_context_t t;
    struct station_config conf;
} _internal_scan_ctx_t;

_internal_scan_ctx_t *_scan_ctx = NULL;
static void ICACHE_FLASH_ATTR on_station_scan(void *arg, STATUS status)
{
    struct bss_info *bss_link = (struct bss_info *)arg;
    if (NULL == _scan_ctx || status != OK)
    {
        return;
    }

    //printf("on_station_scan() ssid: %s, bssid: " MACSTR ", channel: %d, rssi: %d\n", bss_link->ssid, MAC2STR(bss_link->bssid), bss_link->channel, bss_link->rssi);

    if (0 == _scan_ctx->rssi || _scan_ctx->rssi > bss_link->rssi)
    {
        _scan_ctx->conf.bssid_set = 1;
        memcpy(_scan_ctx->conf.bssid, bss_link->bssid, sizeof(_scan_ctx->conf.bssid));

        printf("_internal_wifi_connect() ssid: %s, bssid: " MACSTR "\n", _scan_ctx->conf.ssid, MAC2STR(_scan_ctx->conf.bssid));
        _internal_wifi_connect(&_scan_ctx->conf);
    }
}

bool ICACHE_FLASH_ATTR timer_wifi_connect_scan(void *params)
{
    _internal_scan_ctx_t *ctx = (_internal_scan_ctx_t *)params;

    struct scan_config conf = {
        .ssid = ctx->conf.ssid,
        .show_hidden = true,
    };

    switch (wifi_station_get_connect_status())
    {
    case STATION_GOT_IP:
        //printf("wifi_connect_scan(STATION_GOT_IP)\n");
        break;
    case STATION_CONNECTING:
        printf("wifi_connect_scan(STATION_CONNECTING)\n");
        break;
    default:
        wifi_station_disconnect();
        wifi_station_scan(&conf, on_station_scan);
        printf("wifi_station_scan(ssid = %s, show_hidder = %d)\n", conf.ssid, conf.show_hidden);
        break;
    }

    return true;
}

void ICACHE_FLASH_ATTR wifi_connect(char *ssid, char *passwd, char *bssid)
{
    if (NULL == _scan_ctx)
    {
        _scan_ctx = calloc(1, sizeof(_internal_scan_ctx_t));
    }

    _scan_ctx->conf.bssid_set = 0;
    strcpy(_scan_ctx->conf.ssid, ssid);
    strcpy(_scan_ctx->conf.password, passwd);

    if (bssid) {
        _scan_ctx->conf.bssid_set = 1;
        memcpy(_scan_ctx->conf.bssid, bssid, sizeof(_scan_ctx->conf.bssid));

    }

    _internal_wifi_connect(&_scan_ctx->conf);

    if (NULL == bssid)
    {
        printf("wifi_connect_scan(STARTING, t = 0x%08X)\n", _scan_ctx->t);

        if (NULL == _scan_ctx->t)
        {
            timer_wifi_connect_scan(_scan_ctx);
            _scan_ctx->t = timer_new(5000, timer_wifi_connect_scan, _scan_ctx);
        }
    }
    else
    {
        printf("wifi_connect_scan(STOPPING, t = 0x%08X)\n", _scan_ctx->t);

        if (NULL != _scan_ctx->t)
        {
            timer_stop(_scan_ctx->t);
        }
    }
}