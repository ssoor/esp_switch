#include "esp_common.h"

#include "timer.h"

typedef struct __internal_timer_t
{
    bool is_close;
    bool is_repeat;
    os_timer_t t;
    void *context;
    timer_callback_t *f;
} _internal_timer_t;

void ICACHE_FLASH_ATTR _internal_on_timer(_internal_timer_t *ctx)
{
    //printf("_internal_on_timer(is_close = %d, is_repeat = %d, func = 0x%08X)\n", ctx->is_close, ctx->is_repeat, ctx->f);
    if (ctx->is_close || !ctx->f(ctx->context) || !ctx->is_repeat)
    {
        //printf("_internal_on_timer(is_close = %d, is_repeat = %d, func = 0x%08X) close\n", ctx->is_close, ctx->is_repeat, ctx->f);
        os_timer_disarm(&ctx->t);
        free(ctx);
    }
}

_internal_timer_t *_internal_timer_new(uint32 millisecond, bool is_repeat, bool is_nanosecond, timer_callback_t *cb, void *ctx)
{
    _internal_timer_t *timer = (_internal_timer_t *)malloc(sizeof(_internal_timer_t));

    timer->is_close = false;
    timer->is_repeat = is_repeat;

    timer->f = cb;
    timer->context = ctx;

    os_timer_disarm(&timer->t);
    os_timer_setfn(&timer->t, (os_timer_func_t *)_internal_on_timer, timer);

    if (!is_nanosecond)
    {
        os_timer_arm(&timer->t, millisecond, timer->is_repeat); //1s
    }
    else
    {
        os_timer_arm_us(&timer->t, millisecond, timer->is_repeat); //1s
    }

    return timer;
}

timer_context_t timer_new(uint32 millisecond, timer_callback_t *cb, void *ctx)
{
    return (timer_context_t)_internal_timer_new(millisecond, true, false, cb, ctx);
}

timer_context_t *timer_new_us(uint32 nanosecound, timer_callback_t *cb, void *ctx)
{
    return (timer_context_t)_internal_timer_new(nanosecound, true, true, cb, ctx);
}

void timer_after(uint32 millisecond, timer_callback_t *cb, void *ctx)
{
    _internal_timer_new(millisecond, false, false, cb, ctx);
}

void timer_after_us(uint32 nanosecound, timer_callback_t *cb, void *ctx)
{
    _internal_timer_new(nanosecound, false, true, cb, ctx);
}

void timer_stop(timer_context_t timer)
{
    ((_internal_timer_t *)timer)->is_close = true;
}