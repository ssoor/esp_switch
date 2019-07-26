#include "esp_common.h"

#include "timer.h"
#include "airkiss.h"

typedef struct __internal_timer_context
{
    bool locked;
    uint8 channel;
    airkiss_context_t context;
} _internal_timer_context;

/*
 * 平台相关定时器中断处理函数，100ms中断后切换信道
 */
bool ICACHE_FLASH_ATTR timer_switch_channel(void *params)
{
    int err = 0;
    _internal_timer_context *ctx = (_internal_timer_context *)params;

    if (ctx->locked)
    {
        printf("switch channel stop, channel is locked to %d\n", ctx->channel);
        return false;
    }

    if (ctx->channel >= 13)
    {
        ctx->channel = 1;
    }

    printf("switch channel to %d ...\n", ctx->channel);

    if (!wifi_set_channel(ctx->channel)) //切换信道
    {
        printf("switch channel %d failed\n", ctx->channel);
        return false;
    }

    err = airkiss_change_channel(&ctx->context); //清缓存
    if (err < 0)
    {
        printf("airkiss clear channel %d context failed\n", ctx->channel);
        return false;
    }

    ctx->channel++;

    return true;
}

/*
 * airkiss成功后读取配置信息，平台无关，修改打印函数即可
 */
void ICACHE_FLASH_ATTR airkiss_finish(_internal_timer_context *ctx)
{
    int8_t err;
    uint8 buffer[256];
    airkiss_result_t result;
    err = airkiss_get_result(&ctx->context, &result);
    if (err == 0)
    {
        printf("airkiss_get_result() ok!\n");
        printf("ssid = \"%s\", pwd = \"%s\", ssid_length = %d, pwd_length = %d, random = 0x%02x\r\n", result.ssid,
               result.pwd, result.ssid_length, result.pwd_length, result.random);
    }
    else
    {
        printf("airkiss_get_result() failed !\r\n");
    }
}

struct RxControl
{
    signed rssi : 8;
    unsigned rate : 4;
    unsigned is_group : 1;
    unsigned : 1;
    unsigned sig_mode : 2;       // 0:is 11n packet; 1:is not 11n packet;
    unsigned legacy_length : 12; // if not 11n packet, shows length of packet
    unsigned damatch0 : 1;
    unsigned damatch1 : 1;
    unsigned bssidmatch0 : 1;
    unsigned bssidmatch1 : 1;
    unsigned MCS : 7;        // if is 11n packet, shows the modulation and code used (rnage from 0 to 76)
    unsigned CWB : 1;        // if is 11n packet, shows if is HT40 packet or not
    unsigned HT_length : 16; // if is 11n packet, shows length of packet
    unsigned Smoothing : 1;
    unsigned Not_Sounding : 1;
    unsigned : 1;
    unsigned Aggregation : 1;
    unsigned STBC : 2;
    unsigned FEC_CODING : 1; // if is 11n packet, shows if is LDPC packet or net
    unsigned SGI : 1;
    unsigned rxend_state : 8;
    unsigned ampdu_cnt : 8;
    unsigned channel : 4; // switch channel this packet in
    unsigned : 12;
};

struct LenSeq
{
    uint16_t length;     // length of packet
    uint16_t seq;        // serial number of packet, the high 12bits are serial number, low 14 bits are Fragment number (usually be 0)
    uint8_t address3[6]; // the third address in packet
};

struct sniffer_buf
{
    struct RxControl rx_ctrl;
    uint8_t buf[36];         // head of ieee80211 packet
    uint16_t cnt;            // number count of packet
    struct LenSeq lenseq[1]; // length of packet
};

struct sniffer_buf2
{
    struct RxControl rx_ctrl;
    uint8_t buf[112];
    uint16_t cnt;
    uint16_t len; // length of packet
};

void ICACHE_FLASH_ATTR printmac(char *mac, int offset)
{
    int i = 0;
    printf(" %02X", mac[offset + 0]);

    for (i = 1; i < 6; i++)
    {
        printf(":%02X", mac[offset + i]);
    }
}
/*
 * 混杂模式下抓到的802.11网络帧及长度，平台相关
 */
void ICACHE_FLASH_ATTR on_wifi_promiscuous_rx_with_ctx(_internal_timer_context *ctx, uint8 *buf, uint16 len)
{
    typedef struct
    {
        struct
        {
            uint8 protocol_version : 2;
            uint8 type : 2;
            uint8 sub_type : 4;
            uint8 to_ds : 1;
            uint8 from_ds : 1;
            uint8 more_fragment : 1;
            uint8 retry : 1;
            uint8 power_management : 1;
            uint8 more_data : 1;
            uint8 protect_frame : 1;
            uint8 order : 1;
        } frame_control;        /* frame control */
        unsigned char dur[2];   /* duration/ID */
        unsigned char addr1[6]; /* address 1 */
        unsigned char addr2[6]; /* address 2 */
        unsigned char addr3[6]; /* address 3 */
        unsigned char seq[2];   /* sequence control */
        unsigned char addr4[6]; /* address 4 */
    } __ieee80211_frame;

    switch (len)
    {
    case 12:
    {
        struct RxControl *sniffer = (struct RxControl *)buf;
        //printf("RxControl = mode: %d, channel: %d, 1_length: %d, 0_length: %d\n", sniffer->sig_mode, sniffer->channel, sniffer->legacy_length, sniffer->HT_length);
        break;
    }
    case 128:
    {
        struct sniffer_buf2 *sniffer2 = (struct sniffer_buf2 *)buf;
        struct RxControl *sniffer = &sniffer2->rx_ctrl;

        len = (0 == sniffer->sig_mode) ? sniffer->HT_length : sniffer->legacy_length;

        int code = airkiss_recv(&ctx->context, sniffer2->buf, len);
        switch (code)
        {
        case AIRKISS_STATUS_CONTINUE:
            break;
        case AIRKISS_STATUS_CHANNEL_LOCKED:
            printf("AIRKISS_STATUS_CHANNEL_LOCKED\n");
            ctx->locked = true;
            break;
        case AIRKISS_STATUS_COMPLETE:
            printf("AIRKISS_STATUS_COMPLETE\n");
            airkiss_finish(ctx);
            wifi_promiscuous_enable(0); //关闭混杂模式，平台相关

            break;
            //default:
            //printf("airkiss_recv(), code: %d\r\n", err); // 其他值不用处理
        }
        //printf("sniffer_buf2 = cnt: %d, len: %d, ", sniffer2->cnt, sniffer2->len);
        //printf("mode: %d, channel: %d, 1_length: %d, 0_length: %d\n", sniffer->sig_mode, sniffer->channel, sniffer->legacy_length, sniffer->HT_length);
        break;
    }
    default:
    {
        int i = 0;
        static char client[6] = {0xA8, 0xBE, 0x27, 0xAB, 0xD4, 0x8D};
        static char temp_mac[6] = {0};
        struct sniffer_buf *sniffer2 = (struct sniffer_buf *)buf;
        struct RxControl *sniffer = &sniffer2->rx_ctrl;

        len = (0 == sniffer->sig_mode) ? sniffer->HT_length : sniffer->legacy_length;

        int code = airkiss_recv(&ctx->context, sniffer2->buf, len);
        switch (code)
        {
        case AIRKISS_STATUS_CONTINUE:
            break;
        case AIRKISS_STATUS_CHANNEL_LOCKED:
            printf("AIRKISS_STATUS_CHANNEL_LOCKED\n");
            ctx->locked = true;
            break;
        case AIRKISS_STATUS_COMPLETE:
            printf("AIRKISS_STATUS_COMPLETE\n");
            airkiss_finish(ctx);
            wifi_promiscuous_enable(0); //关闭混杂模式，平台相关

            break;
            //default:
            //printf("airkiss_recv(), code: %d\r\n", err); // 其他值不用处理
        }

        // 如果MAC地址和上一次一样就返回
        if (0 == memcmp(temp_mac, &sniffer2->buf[4], 6))
        {
            return;
        }

        // 缓存上次的MAC，避免重复打印
        memcpy(temp_mac, &sniffer2->buf[4], 6);

#ifdef SNIFFER_TEST
        printf("-> %3d: %d", wifi_get_channel());
        printmac(sniffer2->buf, 4);
        printmac(sniffer2->buf, 10);
        printf("\n");
        printf("sniffer_buf = cnt: %d", sniffer2->cnt);
        for (i = 0; i < sniffer2->cnt; ++i)
        {
            printf(", %d_len: %d", i, sniffer2->lenseq[0].length);
        }
        printf(", mode: %d, channel: %d, 1_length: %d, 0_length: %d\n", sniffer->sig_mode, sniffer->channel, sniffer->legacy_length, sniffer->HT_length);
#endif

        if (0 != memcmp(client, &sniffer2->buf[4], 6))
        {
            return;
        }
        printmac(sniffer2->buf, 4);

        printf("\r\n");
        printf("\trssi:%d\r\n", sniffer->rssi);
        printf("\tchannel:%d\r\n", sniffer->channel);
        printf("\trate:%d\r\n", sniffer->rate);
        printf("\tsig_mode:%d\r\n", sniffer->sig_mode);

        // 判断AP
        //for (i=0; i<6; i++) if (sniffer->buf[i+10] != ap[i]) return;
        //printmac(sniffer->buf, 10);

        //os_timer_disarm(&channelHop_timer);

        break;
    }
    }
}

/*
 * 混杂模式下抓到的802.11网络帧及长度，平台相关
 */
_internal_timer_context *_promiscuous_cb_ctx = NULL;
void ICACHE_FLASH_ATTR on_wifi_promiscuous_rx(uint8 *buf, uint16 len)
{
    on_wifi_promiscuous_rx_with_ctx(_promiscuous_cb_ctx, buf, len);
}

bool airkiss_start(uint8 log)
{
    int err = 0;
    timer_context_t timer;
    airkiss_config_t conf;
    _internal_timer_context *ctx;

    conf.memset = (airkiss_memset_fn)&memset,
    conf.memcpy = (airkiss_memcpy_fn)&memcpy,
    conf.memcmp = (airkiss_memcmp_fn)&memcmp,
    ctx = calloc(1, sizeof(_internal_timer_context));

    do
    {
        if (log)
        {
            printf("enable airkiss log output\n");
            conf.printf = (airkiss_printf_fn)&printf;
        }
        err = airkiss_init(&ctx->context, &conf);
        if (err < 0)
        {
            printf("airkiss init faied, error: %d", err);
            break;
        }

        err = 0;
        ctx->locked = false;
        wifi_station_disconnect();
        wifi_set_opmode(STATION_MODE);

        ctx->channel = 1;
        timer = timer_new(100, timer_switch_channel, ctx);

        _promiscuous_cb_ctx = ctx;
        wifi_set_promiscuous_rx_cb(on_wifi_promiscuous_rx);

        wifi_promiscuous_enable(true);
    } while (false);

    if (err != 0)
    {
        if (timer)
        {
            timer_stop(timer);
        }

        if (ctx)
        {
            free(ctx);
        }
    }

    return err;
}
