#include "hardware/adc.h"

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

static uint32_t io_gpio_adc = 1 << 26 | 1 << 27 | 1 << 28 | 1 << 29;

static uint32_t io_gpio_avail =
    1 << 6 | 1 << 7 | 1 << 8 | 1 << 9 | 1 << 10 | 1 << 11 | 1 << 12 | 1 << 13 | 1 << 14 | 1 << 15 | 1 << 20 | 1 << 21;

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

bool io_exe_cmdb(uint cmd, uint gpio, ternary_t value) {
    if (!io_is_gpio_avail(gpio)) return false;

    bool b, pull_up, pull_down;

    switch (cmd) {
    case IO_CMD_GET:           return gpio_get(gpio);
    case IO_CMD_GET_DIR:       return gpio_get_dir(gpio);
    case IO_CMD_GET_PULL_DOWN: return gpio_is_pulled_down(gpio);
    case IO_CMD_GET_PULL_UP:   return gpio_is_pulled_up(gpio);
    case IO_CMD_PUT:
        switch (value) {
        case TERNARY_FALSE:  gpio_put(gpio, false); return false;
        case TERNARY_TRUE:   gpio_put(gpio, true); return true;
        case TERNARY_TOGGLE: b = !gpio_get(gpio); gpio_put(gpio, b); return b;
        default:             return false; // should never happen
        }
    case IO_CMD_SET_DIR:
        switch (value) {
        case TERNARY_FALSE:  gpio_set_dir(gpio, false); return false;
        case TERNARY_TRUE:   gpio_set_dir(gpio, true); return true;
        case TERNARY_TOGGLE: b = !gpio_get_dir(gpio); gpio_set_dir(gpio, b); return b;
        default:             return false; // should never happen
        }
    case IO_CMD_SET_PULL_UP:
        pull_up   = gpio_is_pulled_up(gpio);
        pull_down = gpio_is_pulled_down(gpio);
        switch (value) {
        case TERNARY_FALSE:  pull_up = false;    break;
        case TERNARY_TRUE:   pull_up = true;     break;
        case TERNARY_TOGGLE: pull_up = !pull_up; break;
        }
        gpio_set_pulls(gpio, pull_up, pull_down);
        return pull_up;
    case IO_CMD_SET_PULL_DOWN:
        pull_up   = gpio_is_pulled_up(gpio);
        pull_down = gpio_is_pulled_down(gpio);
        switch (value) {
        case TERNARY_FALSE:  pull_down = false;      break;
        case TERNARY_TRUE:   pull_down = true;       break;
        case TERNARY_TOGGLE: pull_down = !pull_down; break;
        }
        gpio_set_pulls(gpio, pull_up, pull_down);
        return pull_down;
    }
}

void io_init() {
    adc_init();                                     // configure adc
    for (uint i = 0; i < IO_NUM; i++) {
        if (io_is_gpio_adc(i))   adc_gpio_init(i);  // init adc gpio
        if (io_is_gpio_avail(i)) gpio_init(i);      // init gpio
    }
    adc_set_temp_sensor_enabled(true); // enable temperature sensor
}
