#ifndef _TIMER_H_
#define _TIMER_H_

#include "esp_common.h"

typedef void *timer_context_t;
typedef bool timer_callback_t(void *ctx);

timer_context_t timer_new(uint32 millisecond, timer_callback_t *cb, void *ctx);
timer_context_t *timer_new_us(uint32 nanosecound, timer_callback_t *cb, void *ctx);
void timer_stop(timer_context_t timer);

void timer_after(uint32 millisecond, timer_callback_t *cb, void *ctx);
void timer_after_us(uint32 nanosecound, timer_callback_t *cb, void *ctx);

#endif