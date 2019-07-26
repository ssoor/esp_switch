#include "esp_common.h"

#include "net.h"
#include "gpio.h"
#include "timer.h"
#include <lwip/dns.h>
#include <lwip/sockets.h>

#define SWITCH_GPIO GPIO2

typedef struct
{
    ushort port;
    char *address;
    net_conn_t conn;
} _internal_ctx;

bool control_update_status(_internal_ctx *ctx)
{
    int len;
    uint8 switch_status;
    struct timeval timeout = {3, 0};

    if (NULL == ctx->conn)
    {
        if (STATION_GOT_IP != wifi_station_get_connect_status())
        {
            printf("wifi_station_get_connect_status() = %d\n", wifi_station_get_connect_status());
            return false;
        }

        ctx->conn = net_dial_tcp(ctx->address, ctx->port);
        if (NULL == ctx->conn)
        {
            printf("net_dial(%s:%u) failed, error: %d\n", ctx->address, ctx->port, errno);
            return false;
        }

        //设置发送超时
        setsockopt(ctx->conn, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
        //设置接收超时
        setsockopt(ctx->conn, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
    }

    len = net_read(ctx->conn, &switch_status, sizeof(switch_status));
    if (sizeof(switch_status) != len)
    {
        printf("control will reconnect, read is failed, length: %d, error: %d\n", len, errno);
        net_close(ctx->conn);
        ctx->conn = NULL;

        return false;
    }

    GPIO_OUTPUT_SET(SWITCH_GPIO, switch_status)
    printf("control switch status to %d\n", switch_status);

    return true;
}

void ICACHE_FLASH_ATTR task_control_loop(void *param)
{
    int len, delay;
    uint8 switch_status;
    _internal_ctx *ctx = (_internal_ctx *)param;

    delay = 0;

    while (true)
    {
        if (!control_update_status(ctx->conn))
        {
            vTaskDelay(100 / portTICK_RATE_MS);
        }
    }

    VTaskDelete(NULL);
}

_internal_ctx *_wakeup_ctx;
void on_fpm_wakeup()
{
    wifi_fpm_close();

    while (!control_update_status(_wakeup_ctx->conn))
    {
        vTaskDelay(100 / portTICK_RATE_MS);
    }

    wifi_fpm_open();
    wifi_fpm_do_sleep(1 * 1000);
}

void ICACHE_FLASH_ATTR task_control_reconnect(void *param)
{
    int len;
    uint8 package_update[] = {'U'};
    _internal_ctx *ctx = (_internal_ctx *)param;

    while (true)
    {
        vTaskDelay(1000 / portTICK_RATE_MS);

        if (NULL == ctx->conn)
        {
            continue;
        }

        printf("control send update, conn: 0x%08X\n", ctx->conn);
        len = net_write(ctx->conn, &package_update, sizeof(package_update));
        if (sizeof(package_update) == len)
        {
            continue;
        }
    }
}

void control_init(bool loop)
{
    ip_addr_t addr;
    _internal_ctx *ctx = calloc(1, sizeof(_internal_ctx));

    GPIO_ENABLE(SWITCH_GPIO);

    wifi_set_sleep_type(LIGHT_SLEEP_T);
    wifi_fpm_set_wakeup_cb(on_fpm_wakeup);

    wifi_station_disconnect();
    wifi_set_opmode(NULL_MODE); //	set	WiFi	mode	to	null	mode.
    // dns_init();
    // addr.addr = inet_addr("223.5.5.5");
    // dns_setserver(0, &addr);
    // addr.addr = inet_addr("114.114.114.114");
    // dns_setserver(1, &addr);

    ctx->port = 65530;
    ctx->address = "172.12.0.185";

    printf("control_init(address = %s, port = %d)\n", ctx->address, ctx->port);

    wifi_fpm_open();
    wifi_fpm_do_sleep(1 * 1000);
    if (loop)
    {
        xTaskCreate(task_control_loop, "task_control_loop", 256, ctx, tskIDLE_PRIORITY, NULL);
        xTaskCreate(task_control_reconnect, "task_control_reconnect", 256, ctx, tskIDLE_PRIORITY, NULL);
    }
}