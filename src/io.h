#ifndef _IO_H
#define _IO_H

#include "common.h"
#include "prot.h"

typedef enum {
    IO_ADC_INPUT_VSYS =  3, // ADC GP29 (pico VSYS/3, pico_w ~0)
    IO_ADC_INPUT_TEMP =  4, // internal temperature sensor
    IO_NUM_ADC_INPUT  =  5, // number of ADC inputs
    IO_NUM            = 30, // maximum number of GPIOs
} io_adc_input_t;

// public interface
void io_init();
bool io_is_gpio_adc(uint gpio);
bool io_is_gpio_avail(uint gpio);

float io_adc_read(uint input);

bool io_get_value(uint gpio);
bool io_get_dir(uint gpio);
bool io_get_pull_up(uint gpio);
bool io_get_pull_down(uint gpio);

bool io_set_value(uint gpio, bool value);
bool io_set_dir(uint gpio, bool dir);
bool io_set_pull_up(uint gpio, bool pull_up);
bool io_set_pull_down(uint gpio, bool pull_down);

bool io_toggle_value(uint gpio);
bool io_toggle_dir(uint gpio);
bool io_toggle_pull_up(uint gpio);
bool io_toggle_pull_down(uint gpio);

uint io_get_gpio_flags();
void io_write_gpio_input_event(writer_t *writer, uint flags);

#endif