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

bool io_exe_cmdb(uint cmd, uint gpio, bool value) {
    if (!io_is_gpio_avail(gpio)) return false;

    switch (cmd) {
    case IO_CMD_GET:     return gpio_get(gpio);
    case IO_CMD_PUT:     gpio_put(gpio, value); return value;
    case IO_CMD_GET_DIR: return gpio_get_dir(gpio);
    case IO_CMD_SET_DIR: gpio_set_dir(gpio, value); return value;
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
