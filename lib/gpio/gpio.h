#ifndef _GPIO_H_
#define _GPIO_H_

#include <esp8266/gpio_register.h>

#define GPIO0 0
#define GPIO1 1
#define GPIO2 2
#define GPIO3 3
#define GPIO4 4
#define GPIO5 5
#define GPIO6 6
#define GPIO7 7
#define GPIO8 8
#define GPIO9 9
#define GPIO10 10
#define GPIO11 11
#define GPIO12 12
#define GPIO13 13
#define GPIO14 14
#define GPIO15 15

#define GPIO_PIN_REG_0 PERIPHS_IO_MUX_GPIO0_U
#define GPIO_PIN_REG_1 PERIPHS_IO_MUX_U0TXD_U
#define GPIO_PIN_REG_2 PERIPHS_IO_MUX_GPIO2_U
#define GPIO_PIN_REG_3 PERIPHS_IO_MUX_U0RXD_U
#define GPIO_PIN_REG_4 PERIPHS_IO_MUX_GPIO4_U
#define GPIO_PIN_REG_5 PERIPHS_IO_MUX_GPIO5_U
#define GPIO_PIN_REG_6 PERIPHS_IO_MUX_SD_CLK_U
#define GPIO_PIN_REG_7 PERIPHS_IO_MUX_SD_DATA0_U
#define GPIO_PIN_REG_8 PERIPHS_IO_MUX_SD_DATA1_U
#define GPIO_PIN_REG_9 PERIPHS_IO_MUX_SD_DATA2_U
#define GPIO_PIN_REG_10 PERIPHS_IO_MUX_SD_DATA3_U
#define GPIO_PIN_REG_11 PERIPHS_IO_MUX_SD_CMD_U
#define GPIO_PIN_REG_12 PERIPHS_IO_MUX_MTDI_U
#define GPIO_PIN_REG_13 PERIPHS_IO_MUX_MTCK_U
#define GPIO_PIN_REG_14 PERIPHS_IO_MUX_MTMS_U
#define GPIO_PIN_REG_15 PERIPHS_IO_MUX_MTDO_U

#define GPIO_FUNC(no) FUNC_GPIO##no
#define GPIO_PIN_REG(no) GPIO_PIN_REG_##no
#define GPIO_PIN_ADDR(i) (GPIO_PIN0_ADDRESS + i * 4)

#define GPIO_ENABLE(no, output)                       \
    PIN_FUNC_SELECT(GPIO_PIN_REG(no), GPIO_FUNC(no)); \
    GPIO_REG_WRITE(((output) ? GPIO_ENABLE_W1TS_ADDRESS : GPIO_ENABLE_W1TC_ADDRESS), BIT(no))

#define GPIO_PULLUP_ENABLE(no) \ 
    WRITE_PERI_REG((GPIO_PIN_REG(no)), (READ_PERI_REG(GPIO_PIN_REG(no)) & (~(PERIPHS_IO_MUX_PULLUP))))

#define GPIO_PULLUP_DISABLE(no) \ 
    WRITE_PERI_REG((GPIO_PIN_REG(no)), (READ_PERI_REG(GPIO_PIN_REG(no)) | PERIPHS_IO_MUX_PULLUP))

/**  
  * @brief   Sample the level of GPIO input.
  * 
  * @param   gpio_no : The GPIO sequence number.
  *  
  * @return  the level of GPIO input 
  */
#define GPIO_INPUT_GET(gpio_no) ((gpio_input_get() >> gpio_no) & BIT(0))

#define GPIO_OUTPUT_SET(no, value) \
    GPIO_REG_WRITE(((value) ? GPIO_OUT_W1TS_ADDRESS : GPIO_OUT_W1TC_ADDRESS), BIT(no))

#define GPIO_INTR_ATTACH(func, args) \
    _xt_isr_attach(ETS_GPIO_INUM, func, args);

#define GPIO_INTR_STATE_SET(no, type)                                                                                                                              \
    portENTER_CRITICAL();                                                                                                                                          \
    GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(no)), (GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(no))) & (~GPIO_PIN_INT_TYPE_MASK)) | (type << GPIO_PIN_INT_TYPE_LSB)); \
    portEXIT_CRITICAL()

typedef enum
{
    GPIO_PIN_INTR_DISABLE = 0, /**< disable GPIO interrupt */
    GPIO_PIN_INTR_POSEDGE = 1, /**< GPIO interrupt type : rising edge */
    GPIO_PIN_INTR_NEGEDGE = 2, /**< GPIO interrupt type : falling edge */
    GPIO_PIN_INTR_ANYEDGE = 3, /**< GPIO interrupt type : bothe rising and falling edge */
    GPIO_PIN_INTR_LOLEVEL = 4, /**< GPIO interrupt type : low level */
    GPIO_PIN_INTR_HILEVEL = 5  /**< GPIO interrupt type : high level */
} GPIO_INT_TYPE;

#endif