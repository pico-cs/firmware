// board_pico: add to executables only in pico case

#include "pico/binary_info.h"

#include "board.h"

bool board_init(board_t *board, writer_t *logger) {
    bi_decl(bi_1pin_with_name(PICO_DEFAULT_LED_PIN, "On-board LED"));

    board_init_common(board, logger);
    
    // init pico gpio led
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    // enable led during bootstrap
    board_set_led(board, true);

    return true;
}

void board_deinit(board_t *board) {
    board_set_led(board, false);
}

void board_set_led(board_t *board, bool v) {gpio_put(PICO_DEFAULT_LED_PIN, v);}

bool board_get_led(board_t *board) {return gpio_get(PICO_DEFAULT_LED_PIN);}
