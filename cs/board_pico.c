// board_pico: add to executables only in pico case

#include <stdio.h>
#include "pico/binary_info.h"

#include "board.h"

bool board_init(board_t *board, writer_t *writer) {
    bi_decl(bi_1pin_with_name(PICO_DEFAULT_LED_PIN, "On-board LED"));

    board_init_common(board, writer);
    
    // init pico gpio led
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return true;
}

void board_deinit(board_t *board) {}

void board_set_led(board_t *board, bool v) { gpio_put(PICO_DEFAULT_LED_PIN, v ? 1 : 0); }
bool board_get_led(board_t *board)         { return gpio_get(PICO_DEFAULT_LED_PIN); }
