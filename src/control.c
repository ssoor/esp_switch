#include "esp_common.h"

#include "net.h"
#include "gpio.h"
#include "timer.h"
#include <lwip/dns.h>
#include <lwip/sockets.h>

#define SWITCH_GPIO GPIO2

#define CONTROL_SERVER_PORT 65530
#define CONTROL_SERVER_ADDR "cn.coocn.cn"

typedef struct
{
    ushort port;
    char address[16];
    net_conn_t conn;
} _internal_ctx;

bool _control_update_status(_internal_ctx *ctx)
{
    int len;
    uint8 switch_status;
    struct timeval timeout = {3, 0};

    if (NULL == ctx->conn)
    {
        if (STATION_GOT_IP != wifi_station_get_connect_status())
        {
            return false;
        }

        ctx->conn = net_dial_tcp(ctx->address, ctx->port);
        if (NULL == ctx->conn)
        {
            printf("net_dial(%s:%u) failed, error: %d\n", ctx->address, ctx->port, errno);
            return false;
        }

        //设置发送超时
        // setsockopt(ctx->conn, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
        //设置接收超时
        // setsockopt(ctx->conn, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
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
    _internal_ctx *ctx = (_internal_ctx *)param;

    while (true)
    {
        if (!_control_update_status(ctx))
        {
            vTaskDelay(100 / portTICK_RATE_MS);
        }
    }

    VTaskDelete(NULL);
}

void ICACHE_FLASH_ATTR task_control_heartbeat(void *param)
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

void ICACHE_FLASH_ATTR task_control_dns(const char *name, ip_addr_t *ipaddr, void *param) {
    _internal_ctx *ctx = (_internal_ctx*)param;

    ctx->port = CONTROL_SERVER_PORT;
    inet_ntoa_r(ipaddr, ctx->address, sizeof(ctx->address));

    printf("control_init(address = %s, port = %d)\n", ctx->address, ctx->port);
    
}

void control_init()
{
    ip_addr_t addr;
    _internal_ctx *ctx = calloc(1, sizeof(_internal_ctx));

    dns_init();
    addr.addr = inet_addr("223.5.5.5");
    dns_setserver(0, &addr);
    addr.addr = inet_addr("114.114.114.114");
    dns_setserver(1, &addr);

    GPIO_ENABLE(SWITCH_GPIO);

    dns_gethostbyname(CONTROL_SERVER_ADDR, &addr, task_control_dns, ctx);
    xTaskCreate(task_control_loop, "task_control_loop", 256, ctx, tskIDLE_PRIORITY, NULL);
    xTaskCreate(task_control_heartbeat, "task_control_reconnect", 256, ctx, tskIDLE_PRIORITY, NULL);
}