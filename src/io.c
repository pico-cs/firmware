
#include "hardware/adc.h"
#include "pico/critical_section.h"

#include "io.h"

/* GPIOs
- GPIO0:           reserved: UART0 TX (default)
- GPIO1:           reserved: UART0 RX (default)
- GPIO2:           DCC output
- GPIO3:           DCC output inverted
- GPIO4:           reserved: I2C0 SDA (default)
- GPIO5:           reserved: I2C0 SCL (default)
- GPIO6  - GPIO15: available
- GPIO16:          reserved: SPIO RX  (default)
- GPIO17:          reserved: SPIO SCn (default)
- GPIO18:          reserved: SPIO SCK (default)
- GPIO19:          reserved: SPIO TX  (default)
- GPIO20 - GPIO21: available
- GPIO22:          DCC enabled
- GPIO23:          reserved: PICO internal use
- GPIO24:          reserved: PICO internal use
- GPIO25:          reserved: (on board LED - pico only)
- GPIO26:          ADC: ADC0
- GPIO27:          ADC: ADC1
- GPIO28:          ADC: ADC2
- GPIO29:          ADC: ADC3 / PICO internal use
*/

static uint io_gpio_adc = 1 << 26 | 1 << 27 | 1 << 28 | 1 << 29;

static uint io_gpio_avail =
    1 << 10 | 1 << 11 | 1 << 12 | 1 << 13 | 1 << 14 | 1 << 15 | 1 << 16 | 1 << 17 | 1 << 18 | 1 << 19 | 1 << 20 | 1 << 21 | 1 << 22;

// 12-bit conversion, assume max value == ADC_VREF == 3.3 V
static const float conversion_factor = 3.3f / (1 << 12);

bool io_is_gpio_adc(uint gpio) {
    if (gpio > IO_NUM) return false;
    return io_gpio_adc & (1 << gpio) ? true : false;
}

bool io_is_gpio_avail(uint gpio) {
    if (gpio > IO_NUM) return false;
    return io_gpio_avail & (1 << gpio) ? true : false;
}

float io_adc_read(uint input) {
    adc_select_input(input);
    uint16_t result = adc_read();
    return (float)result * conversion_factor;
}

bool io_get_value(uint gpio)                     { return gpio_get(gpio); }
bool io_get_dir(uint gpio)                       { return gpio_get_dir(gpio); }
bool io_get_pull_up(uint gpio)                   { return gpio_is_pulled_up(gpio); }
bool io_get_pull_down(uint gpio)                 { return gpio_is_pulled_down(gpio); }

bool io_set_value(uint gpio, bool value)         { gpio_put(gpio, value); return value; }
bool io_set_dir(uint gpio, bool dir)             { gpio_set_dir(gpio, dir); return dir; }
bool io_set_pull_up(uint gpio, bool pull_up)     { gpio_set_pulls(gpio, pull_up, gpio_is_pulled_down(gpio)); return pull_up; }
bool io_set_pull_down(uint gpio, bool pull_down) { gpio_set_pulls(gpio, gpio_is_pulled_up(gpio), pull_down); return pull_down; }

bool io_toggle_value(uint gpio)                  { bool b = !gpio_get(gpio); gpio_put(gpio, b); return b; }
bool io_toggle_dir(uint gpio)                    { bool b = !gpio_get_dir(gpio); gpio_set_dir(gpio, b); return b; }
bool io_toggle_pull_up(uint gpio)                { bool b = !gpio_is_pulled_up(gpio); gpio_set_pulls(gpio, b, gpio_is_pulled_down(gpio)); return b; }
bool io_toggle_pull_down(uint gpio)              { bool b = !gpio_is_pulled_down(gpio); gpio_set_pulls(gpio, gpio_is_pulled_up(gpio), b); return b; }

// gpio callback
static critical_section_t gpio_callback_lock;
static volatile uint gpio_flags = 0;

static void gpio_callback(uint gpio, uint32_t events) {
    gpio_flags |= (1 << gpio); // flag change
}

uint io_get_gpio_flags() {
    uint flags;
    critical_section_enter_blocking(&gpio_callback_lock);
    flags = gpio_flags;
    gpio_flags = 0;
    critical_section_exit(&gpio_callback_lock);
    return (flags & io_gpio_avail); // filter against available gpio
}

void io_write_gpio_input_event(writer_t *writer, uint flags) {
    for (uint i = 0; i < IO_NUM; i++) {
        uint mask = (1 << i);
        if (flags & mask) {
            write_eventf(writer, "ioie: %d %c", i, gpio_get(i)?prot_true:prot_false);
        }
    }
}

void io_init() {
    critical_section_init(&gpio_callback_lock);     // init gpio callback lock
    adc_init();                                     // configure adc
    bool first = true;
    // uint32_t event_mask = GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL | GPIO_IRQ_LEVEL_LOW | GPIO_IRQ_LEVEL_HIGH;
    uint32_t event_mask = GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL;
    // | GPIO_IRQ_LEVEL_LOW | GPIO_IRQ_LEVEL_HIGH;
    
    for (uint i = 0; i < IO_NUM; i++) {
        if (io_is_gpio_adc(i))   adc_gpio_init(i);  // init adc gpio
        if (io_is_gpio_avail(i)) {
            gpio_init(i);                           // init gpio
            if (first) {                            // enable gpio interrupt
                gpio_set_irq_enabled_with_callback(i, event_mask, true, &gpio_callback);
                first = false;
            } else {
                gpio_set_irq_enabled(i, event_mask, true);
            }
        }
    }
    adc_set_temp_sensor_enabled(true); // enable temperature sensor
}
