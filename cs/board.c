#include <stdio.h>
#include "hardware/adc.h"

#include "board.h"

static board_type_t board_get_type() {
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

void board_init_common(board_t *board, writer_t *writer) {
    board->writer = writer;

    adc_init();                        // configure adc
    adc_set_temp_sensor_enabled(true); // enable temperature sensor

    // unique board id    
    pico_unique_board_id_t board_id;
    pico_get_unique_board_id(&board_id);
    uint8_hex_to_string(board_id.id, PICO_UNIQUE_BOARD_ID_SIZE_BYTES, board->id, '-');

    // get real board type
    board->type = board_get_type();
    // clear mac
    board->mac[0] = 0;
}
