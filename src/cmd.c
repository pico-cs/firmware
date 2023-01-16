#include <stdio.h>
#include <string.h>

#include "cmd.h"
#include "board.h"
#include "io.h"
#include "mt.h"
#include "dcc.h"

// return values
static const char* cmd_rvs[] = {
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

// command function
typedef cmd_rc_t (*cmd_cmder)(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);

// command prototypes
static cmd_rc_t cmd_help(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_board(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_led(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_temp(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_mt_enabled(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_mt_cv(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_loco_dir(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_loco_speed128(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_loco_fct(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_loco_cv_byte(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_loco_cv_bit(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_loco_cv29_bit5(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_loco_laddr(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_loco_cv1718(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_acc_fct(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_acc_time(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_acc_status(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_io_adc(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_io_value(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_io_dir(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_io_pull_up(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_io_pull_down(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);

static cmd_rc_t cmd_rbuf(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_rbuf_reset(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);
static cmd_rc_t cmd_rbuf_del(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);

typedef struct {
	const char *command;
    const cmd_cmder cmd;
    const bool show;
	const char *syntax;
	const char *help;
} cmd_command_entry_t;

static const cmd_command_entry_t cmd_commands[] = {
	{"h",         cmd_help,           true,  NULL,                     "help"},
	{"b",         cmd_board,          true,  NULL,                     "board info"},
	{"cl",        cmd_led,            true,  "[t|f]",                  "led"},
	{"ct",        cmd_temp,           true,  NULL,                     "temperature"},
	{"mte",       cmd_mt_enabled,     true,  "[t|f]",                  "main track enabled"},
	{"mtcv",      cmd_mt_cv,          true,  "<idx> [0..255]",         "main track cv"},
	{"ld",        cmd_loco_dir,       true,  "<addr> [t|f|~]",         "loco direction"},
	{"ls",        cmd_loco_speed128,  true,  "<addr> [0..127]",        "loco speed"},
	{"lf",        cmd_loco_fct,       true,  "<addr> <no> [t|f|~]",    "loco function"},
	{"lcvbyte",   cmd_loco_cv_byte,   true,  "<addr> <idx> 0..255",    "loco write cv byte"},
	{"lcvbit",    cmd_loco_cv_bit,    true,  "<addr> <idx> 0..7 t|f",  "loco write cv bit"},
	{"lcv29bit5", cmd_loco_cv29_bit5, true,  "<addr> t|f",             "loco set cv29 bit 5"},
	{"lladdr",    cmd_loco_laddr,     true,  "<addr> <laddr>",         "loco set long address"},
	{"lcv1718",   cmd_loco_cv1718,    true,  "<addr>",                 "loco calculate cv17 cv18"},
	{"af",        cmd_acc_fct,        true,  "<addr> 0|1 t|f",         "accessory function"},
	{"at",        cmd_acc_time,       true,  "<addr> 0|1 0..127",      "accessory time"},
	{"as",        cmd_acc_status,     true,  "<addr> 0..255",          "accessory status"},
	{"ioadc",     cmd_io_adc,         true,  "0..4",                   "io read adc input"},
	{"ioval",     cmd_io_value,       true,  "<gpio> [t|f|~]",         "io gpio value"},
	{"iodir",     cmd_io_dir,         true,  "<gpio> [t|f|~]",         "io gpio direction"},
	{"ioup",      cmd_io_pull_up,     true,  "<gpio> [t|f|~]",         "io gpio pull-up"},
	{"iodown",    cmd_io_pull_down,   true,  "<gpio> [t|f|~]",         "io gpio pull-down"},

	// debugging only
	{"r",         cmd_rbuf,           false, NULL,                     "refresh buffer"},
	{"rr",        cmd_rbuf_reset,     false, NULL,                     "refresh buffer reset"},
	{"rd",        cmd_rbuf_del,       false, "<addr>",                 "refresh buffer delete"},
};

inline static bool cmd_check_num_prm(int act, int min, int max) {
    return (act >=min && act <= max);
}

static int cmd_find_command(char *command) {
    for (int i = 0; i < count_of(cmd_commands); i++) {
        if (strcmp(command, cmd_commands[i].command) == 0) {
            return i;
        }
    }
    return -1;
}

void cmd_init(cmd_t *cmd, board_t *board, rbuf_t *rbuf, cfgch_t *cfgch, cmdch_t *cmdch) {
    cmd->board = board;
    cmd->rbuf = rbuf;
    cmd->cfgch = cfgch;
    cmd->cmdch = cmdch;
}

static cmd_rc_t cmd_help(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;

    for (int i = 0; i < count_of(cmd_commands); i++) {
        if (cmd_commands[i].show) {
            if (cmd_commands[i].syntax == NULL) {
                write_multi(writer, "%s (%s)", cmd_commands[i].command, cmd_commands[i].help);
            } else {
                write_multi(writer, "%s %s (%s)", cmd_commands[i].command, cmd_commands[i].syntax, cmd_commands[i].help);
            }
        }
    }
    write_eor(writer);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_board(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;

    switch (cmd->board->type) {
    case BOARD_TYPE_PICO:   write_success(writer, "pico %s", cmd->board->id); break;
    case BOARD_TYPE_PICO_W: write_success(writer, "pico_w %s %s", cmd->board->id, cmd->board->mac); break;
    }

    return CMD_RC_OK;
}

static cmd_rc_t cmd_led(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 2)) return CMD_RC_INVNUMPRM;

    bool on;
        
    switch (num_prm) {
    case 1:
        on = board_get_led_enabled(cmd->board);
        break;
    case 2:
        if (!parse_bool(reader_get_prm(reader, 1), &on)) return CMD_RC_INVPRM;
        board_set_led_enabled(cmd->board, on);
        bool enabled = cfgch_get_enabled(cmd->cfgch);
        board_set_led(cmd->board, enabled);
        break;
    }

    write_success(writer, "%c", on?prot_true:prot_false);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_temp(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;
    float result = io_adc_read(IO_ADC_INPUT_TEMP);
    
    write_success(writer, "%f", 27 - (result - 0.706) / 0.001721);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_mt_enabled(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 2)) return CMD_RC_INVNUMPRM;

    bool on;
    
    switch (num_prm) {
    case 1:
        on = cfgch_get_enabled(cmd->cfgch);
        break;
    case 2:
        if (!parse_bool(reader_get_prm(reader, 1), &on)) return CMD_RC_INVPRM;
        cfgch_set_enabled(cmd->cfgch, on);
        board_set_led(cmd->board, on);
        break;
    }

    write_success(writer, "%c", on?prot_true:prot_false);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_mt_cv(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 2, 3)) return CMD_RC_INVNUMPRM;

    byte idx;
    if (!parse_byte(reader_get_prm(reader, 1), &idx)) return CMD_RC_INVPRM;
    if (idx >= MT_NUM_CV) return CMD_RC_INVPRM;

    byte cv;
    
    switch (num_prm) {
    case 2:
        cv = cfgch_get_cv(cmd->cfgch, idx);
        break;
    case 3:
        if (!parse_byte(reader_get_prm(reader, 2), &cv)) return CMD_RC_INVPRM;
        cv = cfgch_set_cv(cmd->cfgch, idx, cv);
        break;
    }
    
    write_success(writer, "%d", cv);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_loco_dir(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 2, 3)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!parse_uint(reader_get_prm(reader, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    ternary_t value;
    bool dir;
    
    switch (num_prm) {
    case 2:
        if (!rbuf_get_dir(cmd->rbuf, addr, &dir)) return CMD_RC_NODATA;
        break;
    case 3:
        if (!parse_ternary(reader_get_prm(reader, 2), &value)) return CMD_RC_INVPRM;
        switch (value) {
        case TERNARY_FALSE:
            dir = false;
            if (!rbuf_set_dir(cmd->rbuf, addr, dir)) return CMD_RC_NOCHANGE;
            break;
        case TERNARY_TRUE:
            dir = true;
            if (!rbuf_set_dir(cmd->rbuf, addr, dir)) return CMD_RC_NOCHANGE;
            break;
        case TERNARY_TOGGLE:
            if (!rbuf_toggle_dir(cmd->rbuf, addr, &dir)) return CMD_RC_NODATA;
            break;
        }
        break;
    }

    write_success(writer, "%c", dir?prot_true:prot_false);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_loco_speed128(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 2, 3)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!parse_uint(reader_get_prm(reader, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    byte speed128;
        
    switch (num_prm) {
    case 2:
        if (!rbuf_get_speed128(cmd->rbuf, addr, &speed128)) return CMD_RC_NODATA;
        break;
    case 3:
        if (!parse_byte(reader_get_prm(reader, 2), &speed128)) return CMD_RC_INVPRM;
        if (!dcc_check_loco_speed128(speed128)) return CMD_RC_INVPRM;
        if (!rbuf_set_speed128(cmd->rbuf, addr, speed128)) return CMD_RC_NOCHANGE;
        break;
    }

    write_success(writer, "%d", speed128);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_loco_fct(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 3, 4)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!parse_uint(reader_get_prm(reader, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    byte no;
    if (!parse_byte(reader_get_prm(reader, 2), &no)) return CMD_RC_INVPRM;

    ternary_t value;
    bool fct;
        
    switch (num_prm) {
    case 3:
        if (!rbuf_get_fct(cmd->rbuf, addr, no, &fct)) return CMD_RC_NODATA;
        break;
    case 4:
        if (!parse_ternary(reader_get_prm(reader, 3), &value)) return CMD_RC_INVPRM;
        switch (value) {
        case TERNARY_FALSE:
            fct = false;
            if (!rbuf_set_fct(cmd->rbuf, addr, no, fct)) return CMD_RC_NOCHANGE;
            break;
        case TERNARY_TRUE:
            fct = true;
            if (!rbuf_set_fct(cmd->rbuf, addr, no, fct)) return CMD_RC_NOCHANGE;
            break;
        case TERNARY_TOGGLE:
            if (!rbuf_toggle_fct(cmd->rbuf, addr, no, &fct)) return CMD_RC_NODATA;
            break;
        }
        break;
    }

    write_success(writer, "%c", fct?prot_true:prot_false);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_loco_cv_byte(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 4, 4)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!parse_uint(reader_get_prm(reader, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    uint idx;
    if (!parse_uint(reader_get_prm(reader, 2), &idx)) return CMD_RC_INVPRM;
    if (!dcc_check_cv_idx(idx)) return CMD_RC_INVPRM;

    byte cv;
    if (!parse_byte(reader_get_prm(reader, 3), &cv)) return CMD_RC_INVPRM;
    if (!dcc_check_cv(cv)) return CMD_RC_INVPRM;

    // cv address is zero based -> cv index 1 is address 0
    uint cv_addr = idx - 1;

    cmdch_cv_byte(cmd->cmdch, MSB(addr), LSB(addr), MSB(cv_addr), LSB(cv_addr), cv);

    write_success(writer, "%d", cv);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_loco_cv_bit(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 5, 5)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!parse_uint(reader_get_prm(reader, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    uint idx;
    if (!parse_uint(reader_get_prm(reader, 2), &idx)) return CMD_RC_INVPRM;
    if (!dcc_check_cv_idx(idx)) return CMD_RC_INVPRM;

    byte bit;
    if (!parse_byte(reader_get_prm(reader, 3), &bit)) return CMD_RC_INVPRM;
    if (!dcc_check_bit(bit)) return CMD_RC_INVPRM;

    bool flag;
    if (!parse_bool(reader_get_prm(reader, 4), &flag)) return CMD_RC_INVPRM;

    // cv address is zero based -> cv index 1 is address 0
    uint cv_addr = idx - 1;

    cmdch_cv_bit(cmd->cmdch, MSB(addr), LSB(addr), MSB(cv_addr), LSB(cv_addr), bit, flag);
    
    write_success(writer, "%c", flag?prot_true:prot_false);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_loco_cv29_bit5(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 3, 3)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!parse_uint(reader_get_prm(reader, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    bool cv29_bit5;
    if (!parse_bool(reader_get_prm(reader, 2), &cv29_bit5)) return CMD_RC_INVPRM;

    cmdch_cv29_bit5(cmd->cmdch, MSB(addr), LSB(addr), cv29_bit5);

    write_success(writer, "%c", cv29_bit5?prot_true:prot_false);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_loco_laddr(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 3, 3)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!parse_uint(reader_get_prm(reader, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    uint laddr;
    if (!parse_uint(reader_get_prm(reader, 2), &laddr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(laddr)) return CMD_RC_INVPRM;

    cmdch_laddr(cmd->cmdch, MSB(addr), LSB(addr), MSB(laddr), LSB(laddr));

    write_success(writer, "%d", laddr);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_loco_cv1718(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 2, 2)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!parse_uint(reader_get_prm(reader, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    byte cv17 = 0xc0 | (byte) (addr>>8);
    byte cv18 = (byte) addr;

    write_success(writer, "%d %d", cv17, cv18);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_acc_fct(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 4, 4)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!parse_uint(reader_get_prm(reader, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_acc_addr(addr)) return CMD_RC_INVPRM;

    byte out;
    if (!parse_byte(reader_get_prm(reader, 2), &out)) return CMD_RC_INVPRM;
    if (!dcc_check_acc_out(out)) return CMD_RC_INVPRM;

    bool flag;
    if (!parse_bool(reader_get_prm(reader, 3), &flag)) return CMD_RC_INVPRM;

    cmdch_acc(cmd->cmdch, MSB(addr), LSB(addr), out, flag);

    write_success(writer, "%c", flag?prot_true:prot_false);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_acc_time(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 4, 4)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!parse_uint(reader_get_prm(reader, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_acc_addr(addr)) return CMD_RC_INVPRM;

    byte out;
    if (!parse_byte(reader_get_prm(reader, 2), &out)) return CMD_RC_INVPRM;
    if (!dcc_check_acc_out(out)) return CMD_RC_INVPRM;

    byte time;
    if (!parse_byte(reader_get_prm(reader, 3), &time)) return CMD_RC_INVPRM;
    if (!dcc_check_acc_time(time)) return CMD_RC_INVPRM;

    // TODO: explain
    cmdch_acc_ext(cmd->cmdch, MSB(addr), LSB(addr), (out << 7) | time);

    write_success(writer, "%d", time);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_acc_status(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 3, 3)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!parse_uint(reader_get_prm(reader, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_acc_addr(addr)) return CMD_RC_INVPRM;

    byte status;
    if (!parse_byte(reader_get_prm(reader, 2), &status)) return CMD_RC_INVPRM;
    
    cmdch_acc_ext(cmd->cmdch, MSB(addr), LSB(addr), status);

    write_success(writer, "%d", status);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_io_adc(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 2, 2)) return CMD_RC_INVNUMPRM;
    
    uint input;

    if (!parse_uint(reader_get_prm(reader, 1), &input)) return CMD_RC_INVPRM;
    if (!(input < IO_NUM_ADC_INPUT)) return CMD_RC_INVPRM;

    write_success(writer, "%f", io_adc_read(input));
    return CMD_RC_OK;
}

static cmd_rc_t cmd_io_value(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 2, 3)) return CMD_RC_INVNUMPRM;

    uint gpio;
    if (!parse_uint(reader_get_prm(reader, 1), &gpio)) return CMD_RC_INVPRM;
    if (!io_is_gpio_avail(gpio)) return CMD_RC_INVGPIO;

    ternary_t value;
    bool b;

    switch (num_prm) {
    case 2:
        b = io_get_value(gpio); break;
    case 3:
        if (!parse_ternary(reader_get_prm(reader, 2), &value)) return CMD_RC_INVPRM;
        switch (value) {
        case TERNARY_FALSE:  b = io_set_value(gpio, false); break;
        case TERNARY_TRUE:   b = io_set_value(gpio, true);  break;
        case TERNARY_TOGGLE: b = io_toggle_value(gpio);     break;
        }
        break;
    }

    write_success(writer, "%c", b?prot_true:prot_false);
    
    return CMD_RC_OK;
}

static cmd_rc_t cmd_io_dir(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 2, 3)) return CMD_RC_INVNUMPRM;

    uint gpio;
    if (!parse_uint(reader_get_prm(reader, 1), &gpio)) return CMD_RC_INVPRM;
    if (!io_is_gpio_avail(gpio)) return CMD_RC_INVGPIO;

    ternary_t value;
    bool b;

    switch (num_prm) {
    case 2:
        b = io_get_dir(gpio); break;
    case 3:
        if (!parse_ternary(reader_get_prm(reader, 2), &value)) return CMD_RC_INVPRM;
        switch (value) {
        case TERNARY_FALSE:  b = io_set_dir(gpio, false); break;
        case TERNARY_TRUE:   b = io_set_dir(gpio, true);  break;
        case TERNARY_TOGGLE: b = io_toggle_dir(gpio);     break;
        }
        break;
    }

    write_success(writer, "%c", b?prot_true:prot_false);
    
    return CMD_RC_OK;
}

static cmd_rc_t cmd_io_pull_up(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 2, 3)) return CMD_RC_INVNUMPRM;

    uint gpio;
    if (!parse_uint(reader_get_prm(reader, 1), &gpio)) return CMD_RC_INVPRM;
    if (!io_is_gpio_avail(gpio)) return CMD_RC_INVGPIO;

    ternary_t value;
    bool b;

    switch (num_prm) {
    case 2:
        b = io_get_pull_up(gpio); break;
    case 3:
        if (!parse_ternary(reader_get_prm(reader, 2), &value)) return CMD_RC_INVPRM;
        switch (value) {
        case TERNARY_FALSE:  b = io_set_pull_up(gpio, false); break;
        case TERNARY_TRUE:   b = io_set_pull_up(gpio, true);  break;
        case TERNARY_TOGGLE: b = io_toggle_pull_up(gpio);     break;
        }
        break;
    }

    write_success(writer, "%c", b?prot_true:prot_false);
    
    return CMD_RC_OK;
}

static cmd_rc_t cmd_io_pull_down(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 2, 3)) return CMD_RC_INVNUMPRM;

    uint gpio;
    if (!parse_uint(reader_get_prm(reader, 1), &gpio)) return CMD_RC_INVPRM;
    if (!io_is_gpio_avail(gpio)) return CMD_RC_INVGPIO;

    ternary_t value;
    bool b;

    switch (num_prm) {
    case 2:
        b = io_get_pull_down(gpio); break;
    case 3:
        if (!parse_ternary(reader_get_prm(reader, 2), &value)) return CMD_RC_INVPRM;
        switch (value) {
        case TERNARY_FALSE:  b = io_set_pull_down(gpio, false); break;
        case TERNARY_TRUE:   b = io_set_pull_down(gpio, true);  break;
        case TERNARY_TOGGLE: b = io_toggle_pull_down(gpio);     break;
        }
        break;
    }

    write_success(writer, "%c", b?prot_true:prot_false);
    
    return CMD_RC_OK;
}

// debugging
static cmd_rc_t cmd_rbuf(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;

    write_multi(writer, "%d %d", cmd->rbuf->first, cmd->rbuf->next);
    if (cmd->rbuf->first != -1) {
        uint idx = cmd->rbuf->first;
        do {
            volatile rbuf_entry_t *entry = &cmd->rbuf->buf[idx];
            f5_68_t f5_68 = entry->f5_68;
    
            write_multi(writer, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                idx,
                ADDR(entry->msb, entry->lsb),
                entry->max_refresh_cmd,
                entry->refresh_cmd,
                entry->dir_speed,
                entry->f0_4,
                f5_68.f5_8,
                f5_68.f9_12,
                f5_68.f5_12,
                f5_68.f13_20,
                f5_68.f21_28,
                f5_68.f29_36,
                f5_68.f37_44,
                f5_68.f45_52,
                f5_68.f53_60,
                f5_68.f61_68,
                entry->prev,
                entry->next
            );
            
            idx = cmd->rbuf->buf[idx].next;
        } while (idx != cmd->rbuf->first);
    }
    
    write_eor(writer);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_rbuf_reset(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;

    rbuf_reset(cmd->rbuf);

    write_success(writer, "%c", prot_true);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_rbuf_del(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 2, 2)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!parse_uint(reader_get_prm(reader, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    if (!rbuf_del(cmd->rbuf, addr)) return CMD_RC_NODATA;

    write_success(writer, "%d", addr);
    return CMD_RC_OK;
}

void cmd_dispatch(cmd_t *cmd, reader_t *reader, writer_t *writer) {

    int num_prm = reader_num_prm(reader);
       
    cmd_rc_t rc = CMD_RC_INVCMD;

    int idx = cmd_find_command(reader_get_prm(reader, 0));
    if (idx != -1) rc = cmd_commands[idx].cmd(cmd, num_prm, reader, writer);

    if (rc != CMD_RC_OK) write_error(writer, cmd_rvs[rc]);
    return;
}    
