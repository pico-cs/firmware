#ifndef _CMD_H
#define _CMD_H

#include "pico/stdlib.h"

#include "common.h"
#include "rbuf.h"
#include "channel.h"

#define CMD_BUFFER_SIZE 255
#define CMD_MAX_COMMAND 15
#define CMD_MAX_RV 10

static const char cmd_tag_start     = '+';
static const char cmd_tag_success   = '=';
static const char cmd_tag_nosuccess = '?';
static const char cmd_tag_multi     = '-';
static const char cmd_tag_eor       = '.';
static const char cmd_tag_push      = '!';

static const char cmd_char_cr    = '\r';
static const char cmd_char_nl    = '\n';
static const char cmd_char_null  = '\0';
static const char cmd_char_quote = '"';
static const char cmd_char_space = ' ';

static const char cmd_char_true   = 't';
static const char cmd_char_false  = 'f';
static const char cmd_char_toggle = '~';

typedef enum{
	CMD_TERNARY_FALSE  = 0,
	CMD_TERNARY_TRUE   = 1,
	CMD_TERNARY_TOGGLE = 2,
} cmd_ternary_t;

static const char* cmd_rvs[CMD_MAX_RV] = {
	"ok",
	"invcmd",        // invalid command
	"invprm",        // invalid parameter
	"invnumprm",     // invalid number of parameters
    "nodata",        // no data available
	"nochange",      // data not changed by command
	"invgpio",       // currently not used
	"rsrvgpio",      // currently not used
	"nooutgpio",     // currently not used
	"notimpl",       // command not implemented
};

typedef enum {
	CMD_RC_OK          = 0,
	CMD_RC_INVCMD      = 1,
	CMD_RC_INVPRM      = 2,
	CMD_RC_INVNUMPRM   = 3,
    CMD_RC_NODATA      = 4,
	CMD_RC_NOCHANGE    = 5,
	CMD_RC_INVGPIO_X   = 6, // currently not used 
	CMD_RC_RSRVGPIO_X  = 7, // currently not used
	CMD_RC_NOOUTGPIO_X = 8, // currently not used
	CMD_RC_NOTIMPL     = 9,
} cmd_rc_t;

typedef enum {
    CMD_TOKEN_START      = 0,
    CMD_TOKEN_END        = 1,
    CMD_TOKEN_STRING     = 2,
    CMD_TOKEN_WHITESPACE = 3,
    CMD_TOKEN_CHAR       = 4,
} cmd_token_t;

typedef struct {
	const char *command;
	const char *syntax;
	const char *help;
 } cmd_command_entry_t;
 
static const cmd_command_entry_t cmd_commands[CMD_MAX_COMMAND] = {
	{"h",         "",                       "help"},
	{"cl",        "[t|f]",                  "led"},
    {"ct",        "",                       "temperature"},
    {"cs",        "[<bits>]",               "DCC sync bits"},
	{"ce",        "[t|f]",                  "enabled"},
	{"cr",        "",                       "refresh buffer"},
	{"cd",        "<addr>",                 "delete loco from refresh buffer"},
	{"ld",        "<addr> [t|f|~]",         "loco direction"},
	{"ls",        "<addr> [<speed128]",     "loco speed"},
	{"lf",        "<addr> <no> [t|f|~]",    "loco function"},
	{"lcvbyte",   "<addr> <idx> <cv>",      "loco write cv byte"},
	{"lcvbit",    "<addr> <idx> <bit> t|f", "loco write cv bit"},
	{"lcv29bit5", "<addr> t|f",             "loco set cv29 bit 5"},
	{"lladdr",    "<addr> <laddr>",         "loco set long address"},
	{"lcv1718",   "<addr>",                 "loco calculate cv17 cv18"}
};

/*
Switch: {cmdSwitch, groupOn, cs.switch_, "<no> <init|on|off>", "switch init or set switch on or off"},
*/

typedef enum { 
    CMD_COMMAND_NOP            =  0,
	CMD_COMMAND_HELP           =  1,
	CMD_COMMAND_LED            =  2,
    CMD_COMMAND_TEMP           =  3,
    CMD_COMMAND_DCC_SYNC_BITS  =  4,
	CMD_COMMAND_ENABLED        =  5,
	CMD_COMMAND_RBUF           =  6,
	CMD_COMMAND_DEL_LOCO       =  7,
	CMD_COMMAND_LOCO_DIR       =  8,
	CMD_COMMAND_LOCO_SPEED128  =  9,
	CMD_COMMAND_LOCO_FCT       = 10,
	CMD_COMMAND_LOCO_CV_BYTE   = 11,
	CMD_COMMAND_LOCO_CV_BIT    = 12,
	CMD_COMMAND_LOCO_CV29_BIT5 = 13,
	CMD_COMMAND_LOCO_LADDR     = 14,
	CMD_COMMAND_LOCO_CV1718    = 15,
} cmd_command_t;

/*
cmdSwitch          = "sw"
*/

typedef enum {
	CMD_FLAG_ENABLED = (byte)0x01, // important: byte cast
	CMD_FLAG_LED     = (byte)0x02, // important: byte cast
} cmd_flag_t;

typedef struct {
    rbuf_t *rbuf;
    channel_t *channel;
    byte buf[CMD_BUFFER_SIZE];
    int pos; // position in buf
	byte flags; 
} cmd_t; // command

// public interface
void cmd_init(cmd_t *cmd, rbuf_t *rb, channel_t *channel);
void cmd_dispatch(cmd_t *cmd);

#endif
