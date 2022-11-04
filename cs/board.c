#include <stdio.h>
#include "hardware/adc.h"

#include "pico/cyw43_arch.h"

#include "board.h"

board_type_t board_get_type() {
    // adc_init(); // already done
    adc_gpio_init(29);
    adc_select_input(3);
    const float conversion_factor = 3.3f / (1 << 12);
    uint16_t result = adc_read();
    float voltage = result * conversion_factor;
    // pico w: should be nearly to 0V
    // pico:   1/3 VSYS
    //   tested:
    //   ~0.56... (directly after start)
    //   ~1.63... (else)
    const float threshold = 0.1f; 
    return (voltage <= threshold ? BOARD_TYPE_PICO_W : BOARD_TYPE_PICO);
    /* debug
        printf("ADC3 value: 0x%03x, voltage: %f V\n", result, result * conversion_factor);
        gpio_init(25);
        gpio_set_dir(25, GPIO_IN);
        uint value = gpio_get(25);
        printf("GP25 value: %i\n", value);
    */
}

bool board_init(board_t *board) {
    adc_init();                        // configure adc
    adc_set_temp_sensor_enabled(true); // enable temperature sensor

    pico_get_unique_board_id(&board->board_id);
    board->type = board_get_type();

    switch (board->type) {
    case BOARD_TYPE_PICO:
        // init pico gpio led
        gpio_init(BOARD_GPIO_LED_PIN);
        gpio_set_dir(BOARD_GPIO_LED_PIN, GPIO_OUT);
        break;
    case BOARD_TYPE_PICO_W:
        if (cyw43_arch_init()) {
            printf("WiFi init failed");
            return false;
        }
        break;
    }
    return true;
}

void board_set_led(board_t *board, bool v) {
    switch (board->type) {
        case BOARD_TYPE_PICO:   gpio_put(BOARD_GPIO_LED_PIN, v ? 1 : 0);               break;
        case BOARD_TYPE_PICO_W: cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, v ? 1 : 0); break;
    }
}

bool board_get_led(board_t *board) {
    switch (board->type) {
        case BOARD_TYPE_PICO:   return gpio_get(BOARD_GPIO_LED_PIN)               == 1; break;
        case BOARD_TYPE_PICO_W: return cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN) == 1; break;
    }
}
