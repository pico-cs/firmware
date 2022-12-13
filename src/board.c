#include <stdio.h>

#include "board.h"
#include "io.h"

static board_type_t board_get_type() {
    float voltage = io_adc_read(IO_ADC_INPUT_VSYS);
    // pico w: should be nearly to 0V
    // pico:   1/3 VSYS
    //   tested:
    //   ~0.56... (directly after start)
    //   ~1.63... (else)
    /* debug
        printf("ADC3 value: 0x%03x, voltage: %f V\n", result, result * conversion_factor);
    */
    const float threshold = 0.1f; 
    return (voltage <= threshold ? BOARD_TYPE_PICO_W : BOARD_TYPE_PICO);
}

void board_init_common(board_t *board, writer_t *logger) {
    board->logger = logger;

    // unique board id    
    pico_unique_board_id_t board_id;
    pico_get_unique_board_id(&board_id);
    uint8_hex_to_string(board_id.id, PICO_UNIQUE_BOARD_ID_SIZE_BYTES, board->id, '-');

    // get real board type
    board->type = board_get_type();
    // clear mac
    board->mac[0] = 0;
}
