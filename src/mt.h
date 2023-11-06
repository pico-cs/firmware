#ifndef _MT_H
#define _MT_H

#include "mt.h"
#include "board.h"
#include "rbuf.h"
#include "dcc_tx.h"

typedef struct {
    bool enabled;
    dcc_tx_pio_t *tx_pio;
    dcc_tx_sm_t *tx_sm;
    rbuf_t *rbuf;
    cmdq_t *cmdq;
} mt_t; // main track

// public interface
void mt_init(mt_t *mt, dcc_tx_pio_t *tx_pio, rbuf_t *rbuf, cmdq_t *cmdq);
void mt_dispatch(mt_t *mt);

#endif
