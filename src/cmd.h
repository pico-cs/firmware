#ifndef _CMD_H
#define _CMD_H

#include "common.h"
#include "board.h"
#include "rbuf.h"
#include "channel.h"
#include "prot.h"

#define CMD_MAX_COMMAND 18
#define CMD_MAX_RV      10

static const char* cmd_rvs[CMD_MAX_RV] = {
	"ok",
	"invcmd",        // invalid command
	"invprm",        // invalid parameter
	"invnumprm",     // invalid number of parameters
	"nodata",        // no data available
	"nochange",      // data not changed by command
	"invgpio",       // invalid gpio number
	"free1",         // currently not used
	"free2",         // currently not used
	"notimpl",       // command not implemented
};

typedef enum {
	CMD_RC_OK          = 0,
	CMD_RC_INVCMD      = 1,
	CMD_RC_INVPRM      = 2,
	CMD_RC_INVNUMPRM   = 3,
	CMD_RC_NODATA      = 4,
	CMD_RC_NOCHANGE    = 5,
	CMD_RC_INVGPIO     = 6, 
	CMD_RC_FREE1       = 7, // currently not used
	CMD_RC_FREE2       = 8, // currently not used
	CMD_RC_NOTIMPL     = 9,
} cmd_rc_t;

typedef struct {
	const char *command;
	const char *syntax;
	const char *help;
} cmd_command_entry_t;
 
static const cmd_command_entry_t cmd_commands[CMD_MAX_COMMAND] = {
	{"h",         NULL,                     "help"},
	{"b",         NULL,                     "board info"},
	{"cl",        "[t|f]",                  "led"},
	{"ct",        NULL,                     "temperature"},
	{"cs",        "[<bits>]",               "DCC sync bits"},
	{"ce",        "[t|f]",                  "enabled"},
	{"cr",        NULL,                     "refresh buffer"},
	{"cd",        "<addr>",                 "delete loco from refresh buffer"},
	{"ld",        "<addr> [t|f|~]",         "loco direction"},
	{"ls",        "<addr> [0..127]",        "loco speed"},
	{"lf",        "<addr> <no> [t|f|~]",    "loco function"},
	{"lcvbyte",   "<addr> <idx> <cv>",      "loco write cv byte"},
	{"lcvbit",    "<addr> <idx> <bit> t|f", "loco write cv bit"},
	{"lcv29bit5", "<addr> t|f",             "loco set cv29 bit 5"},
	{"lladdr",    "<addr> <laddr>",         "loco set long address"},
	{"lcv1718",   "<addr>",                 "loco calculate cv17 cv18"},
	{"ioadc",     "0..4",                   "io read adc input"},
	{"iocmdb",    "<cmd> <gpio> [t|f]",     "io execute boolean gpio command"},
};

/*
Switch: {cmdSwitch, groupOn, cs.switch_, "<no> <init|on|off>", "switch init or set switch on or off"},
*/

typedef enum { 
    CMD_COMMAND_NOP            =  0,
	CMD_COMMAND_HELP           =  1,
	CMD_COMMAND_BOARD          =  2,
	CMD_COMMAND_LED            =  3,
	CMD_COMMAND_TEMP           =  4,
	CMD_COMMAND_DCC_SYNC_BITS  =  5,
	CMD_COMMAND_ENABLED        =  6,
	CMD_COMMAND_RBUF           =  7,
	CMD_COMMAND_DEL_LOCO       =  8,
	CMD_COMMAND_LOCO_DIR       =  9,
	CMD_COMMAND_LOCO_SPEED128  = 10,
	CMD_COMMAND_LOCO_FCT       = 11,
	CMD_COMMAND_LOCO_CV_BYTE   = 12,
	CMD_COMMAND_LOCO_CV_BIT    = 13,
	CMD_COMMAND_LOCO_CV29_BIT5 = 14,
	CMD_COMMAND_LOCO_LADDR     = 15,
	CMD_COMMAND_LOCO_CV1718    = 16,
	CMD_COMMAND_IO_ADC         = 17,
	CMD_COMMAND_IO_CMDB        = 18,
} cmd_command_t;

typedef enum {
	CMD_FLAG_ENABLED = (byte)0x01, // important: byte cast
	CMD_FLAG_LED     = (byte)0x02, // important: byte cast
} cmd_flag_t;

typedef struct {
	board_t *board;
	rbuf_t *rbuf;
	channel_t *channel;
	byte flags; 
} cmd_t; // command

// public interface
void cmd_init(cmd_t *cmd, board_t *board, rbuf_t *rb, channel_t *channel);
void cmd_dispatch(cmd_t *cmd, reader_t *reader, writer_t *writer);

#endif
