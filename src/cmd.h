#ifndef _CMD_H
#define _CMD_H

#include "board.h"
#include "rbuf.h"
#include "channel.h"

typedef struct {
	board_t *board;
	rbuf_t *rbuf;
	cfgch_t *cfgch;
	cmdch_t *cmdch;
} cmd_t; // command

// public interface
void cmd_init(cmd_t *cmd, board_t *board, rbuf_t *rbuf, cfgch_t *cfgch, cmdch_t *cmdch);
void cmd_dispatch(cmd_t *cmd, reader_t *reader, writer_t *writer);

#endif
