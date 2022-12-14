#ifndef _IO_H
#define _IO_H

#include "common.h"

typedef enum {
    IO_ADC_INPUT_VSYS =  3, // ADC GP29 (pico VSYS/3, pico_w ~0)
    IO_ADC_INPUT_TEMP =  4, // internal temperature sensor
    IO_NUM_ADC_INPUT  =  5, // number of ADC inputs
    IO_NUM            = 30, // maximum number of GPIOs
} io_adc_input_t;

typedef enum {
    IO_CMD_GET     = 0,
    IO_CMD_PUT     = 1,
    IO_CMD_GET_DIR = 2,
    IO_CMD_SET_DIR = 3,
    IO_NUM_CMD     = 4,
} io_cmd_t;

// public interface
void io_init();
bool io_is_gpio_adc(uint gpio);
bool io_is_gpio_avail(uint gpio);
float io_adc_read(uint input);
bool io_exe_cmdb(uint cmd, uint gpio, bool value);

#endif