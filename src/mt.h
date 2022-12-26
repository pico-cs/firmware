#ifndef _MT_H
#define _MT_H

#include "rbuf.h"
#include "dcc.h"
#include "channel.h"

#define MT_NUM_CV 4

typedef struct {
    bool enabled;
    rbuf_t *rbuf;
    cfgch_t *cfgch;
    cmdch_t *cmdch;
    dcc_t dcc;
    byte cv[MT_NUM_CV];
} mt_t; // main track

// public interface
void mt_init(mt_t *mt, rbuf_t *rb, cfgch_t *cfgch, cmdch_t *cmdch);
void mt_dispatch(mt_t *mt);

#endif
