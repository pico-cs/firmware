#ifndef _BOARD_H
#define _BOARD_H

#include "pico/unique_id.h"

#include "common.h"
#include "prot.h"

typedef enum { 
    BOARD_TYPE_PICO   = 1, // pico board
    BOARD_TYPE_PICO_W = 2, // pico w board
} board_type_t;

#define MAC_SIZE_BYTES 6

typedef struct {
    writer_t *logger;
    board_type_t type;
    char id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES*3];
    char mac[MAC_SIZE_BYTES*3]; // wifi station mac address (empty in case of pico)
} board_t;

// public interface
void board_init_common(board_t *board, writer_t *logger);

bool board_init(board_t *board, writer_t *logger);
void board_deinit(board_t *board);
void board_set_led(board_t *board, bool v);
bool board_get_led(board_t *board);

#endif