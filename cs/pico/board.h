#ifndef _BOARD_H
#define _BOARD_H

#include "pico/unique_id.h"

#include "common.h"

#define BOARD_GPIO_LED_PIN 25

typedef enum { 
    BOARD_TYPE_PICO   = 0, // pico board
    BOARD_TYPE_PICO_W = 1, // pico w board
} board_type_t;

typedef struct {
    board_type_t type;
    pico_unique_board_id_t board_id;
} board_t;

// public interface
bool board_init();
void board_set_led(board_t *board, bool v);
bool board_get_led(board_t *board);

#endif