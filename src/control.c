#include "config.h"
#include "esp_common.h"

#include "net.h"
#include "gpio.h"
#include "timer.h"
#include <lwip/dns.h>
#include <lwip/sys.h>
#include <lwip/sockets.h>
#include <esp8266/ets_sys.h>

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

        printf("control connection succeeded, address = %s:%d\n", ctx->address, ctx->port);
    }

    len = net_read_with_timeout(ctx->conn, CONTROL_READ_TIMEOUT, &switch_status, sizeof(switch_status));
    if (sizeof(switch_status) != len)
    {
        if (ERR_TIMEOUT == errno)
        {
            return true;
        }

        printf("control will reconnect, read is failed, length: %d, error: %d\n", len, errno);
        net_close(ctx->conn);
        ctx->conn = NULL;

        return false;
    }

    GPIO_OUTPUT_SET(GPIO_LIGHT, switch_status);
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

        len = net_write(ctx->conn, &package_update, sizeof(package_update));
        if (sizeof(package_update) == len)
        {
            continue;
        }
        printf("control send update failed, error: 0x%d\n", errno);
    }
}

void ICACHE_FLASH_ATTR on_control_gethostbyaddr(const char *name, ip_addr_t *ipaddr, void *param)
{
    _internal_ctx *ctx = (_internal_ctx *)param;

    //inet_ntoa_r(ipaddr, ctx->address, sizeof(ctx->address)); // DNS 解析出来的 IP 不对

    printf("on_control_gethostbyaddr(hostname = %s, address = %s, port = %d)\n", name, ctx->address, ctx->port);
}

void IRAM_ATTR on_switch_local_intr(void *param)
{
    uint32 status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

    ETS_INTR_LOCK();

    if (status & BIT(GPIO_LIGHT_SWITCH))
    {
        GPIO_OUTPUT_SET(GPIO_LIGHT, !GPIO_INPUT_GET(GPIO_LIGHT));
        printf("control switch status to %d\n", GPIO_INPUT_GET(GPIO_LIGHT));
    }

    os_delay_us(10000);
    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, status);

    ETS_INTR_UNLOCK();
}

void control_init()
{
    ip_addr_t addr;
    _internal_ctx *ctx = calloc(1, sizeof(_internal_ctx));

    dns_init();
    GPIO_ENABLE(GPIO_LIGHT, true);
    GPIO_ENABLE(GPIO_LIGHT_SWITCH, false);

    ETS_INTR_LOCK();

    GPIO_INTR_ATTACH(on_switch_local_intr, NULL);
    GPIO_INTR_STATE_SET(GPIO_ID_PIN(GPIO_LIGHT_SWITCH), GPIO_PIN_INTR_ANYEDGE);

    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(GPIO_LIGHT_SWITCH));

    ETS_INTR_UNLOCK();

    _xt_isr_unmask(1 << ETS_GPIO_INUM); // 使能GPIO中断

    ctx->port = CONTROL_SERVER_PORT;
    strcpy(ctx->address, CONTROL_SERVER_ADDR);
    dns_gethostbyname(CONTROL_SERVER_ADDR, &addr, on_control_gethostbyaddr, ctx);

    xTaskCreate(task_control_loop, "task_control_loop", 256, ctx, tskIDLE_PRIORITY, NULL);
    xTaskCreate(task_control_heartbeat, "task_control_reconnect", 256, ctx, tskIDLE_PRIORITY, NULL);
}