#ifndef _EXE_H
#define _EXE_H

#include "rbuf.h"
#include "dcc.h"
#include "channel.h"

typedef struct {
    bool enabled;
    rbuf_t *rbuf;
    channel_t *channel;
    dcc_t dcc;
} exe_t; // command

// public interface
void exe_init(exe_t *exe, rbuf_t *rb, channel_t *channel);
void exe_dispatch(exe_t *exe);

#endif
