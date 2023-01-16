#ifndef _MT_H
#define _MT_H

#include "rbuf.h"
#include "dcc_tx.h"
#include "channel.h"

// configuration variables
static const byte MT_CV_NUM_SYNC_BIT        = 0;
static const byte MT_CV_NUM_REPEAT          = 1;
static const byte MT_CV_NUM_REPEAT_CV       = 2;
static const byte MT_CV_NUM_REPEAT_ACC      = 3;
static const byte MT_CV_BIDI                = 4;
static const byte MT_CV_BIDI_TS             = 5; // cutout start time after end bit
static const byte MT_CV_BIDI_TE             = 6; // cutout stop time  before 5th sync bit

#define MT_NUM_CV 7

typedef struct {
    bool enabled;
    dcc_tx_pio_t *tx_pio;
    dcc_tx_sm_t *tx_sm;
    rbuf_t *rbuf;
    cfgch_t *cfgch;
    cmdch_t *cmdch;
    byte cv[MT_NUM_CV];
} mt_t; // main track

// public interface
void mt_init(mt_t *mt, dcc_tx_pio_t *tx_pio, rbuf_t *rbuf, cfgch_t *cfgch, cmdch_t *cmdch);
void mt_dispatch(mt_t *mt);

#endif
