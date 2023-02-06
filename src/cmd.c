#include <string.h>

#include "cmd.h"
#include "cfg.h"
#include "flash.h"
#include "io.h"
#include "dcc.h"

typedef enum {
    CMD_RC_EXIT      = -1,
    CMD_RC_OK        =  0,
    CMD_RC_INVCMD    =  1,
	CMD_RC_INVPRM    =  2,
	CMD_RC_INVNUMPRM =  3,
	CMD_RC_NODATA    =  4,
	CMD_RC_NOCHANGE  =  5,
	CMD_RC_INVGPIO   =  6, 
	CMD_RC_NOTIMPL   =  7,
    CMD_RC_NOTEXEC   =  8,
	CMD_RC_IOERR     =  9,
} cmd_rc_t;

// return values
static const char* cmd_rvs[] = {
	"ok",
    "invcmd",       // invalid command
	"invprm",       // invalid parameter
	"invnumprm",    // invalid number of parameters
	"nodata",       // no data available
	"nochange",     // data not changed by command
	"invgpio",      // invalid gpio number
	"notimpl",      // command not implemented
    "notexec",      // command not executed (not allowed in this context)
    "ioerr",        // I/O error (flash)
};

inline static bool cmd_check_num_prm(int act, int min, int max) {
    return (act >= min && act <= max);
}

static cmd_rc_t cmd_board(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;

    switch (cmd->board->type) {
    case BOARD_TYPE_PICO:   write_success(writer, "pico %s", cmd->board->id); break;
    case BOARD_TYPE_PICO_W: write_success(writer, "pico_w %s %s", cmd->board->id, cmd->board->mac); break;
    }

    return CMD_RC_OK;
}

static cmd_rc_t cmd_temp(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;
    float result = io_adc_read(IO_ADC_INPUT_TEMP);
    
    write_success(writer, "%f", 27 - (result - 0.706) / 0.001721);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_store(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;

    if (cfg_get_mt_enabled()) return CMD_RC_NOTEXEC;
    if (!cfg_store()) return CMD_RC_IOERR;
    write_success(writer, "%c", prot_true);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_cv(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 2, 3)) return CMD_RC_INVNUMPRM;

    byte idx;
    if (!parse_byte(reader_get_prm(reader, 1), &idx)) return CMD_RC_INVPRM;
    if (idx >= CFG_NUM_CV) return CMD_RC_INVPRM;

    byte cv;
    
    switch (num_prm) {
    case 2:
        cv = cfg_get_cv(idx);
        break;
    case 3:
        if (!parse_byte(reader_get_prm(reader, 2), &cv)) return CMD_RC_INVPRM;
        cv = cfg_set_cv(idx, cv);
        break;
    }
    
    write_success(writer, "%d", cv);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_mt_enabled(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 2)) return CMD_RC_INVNUMPRM;

    bool on;
    
    switch (num_prm) {
    case 1:
        on = cfg_get_mt_enabled();
        break;
    case 2:
        if (!parse_bool(reader_get_prm(reader, 1), &on)) return CMD_RC_INVPRM;
        cfg_set_mt_enabled(on);
        break;
    }

    write_success(writer, "%c", on?prot_true:prot_false);
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

    cmdq_cv_byte(cmd->cmdq, MSB(addr), LSB(addr), MSB(cv_addr), LSB(cv_addr), cv);

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

    cmdq_cv_bit(cmd->cmdq, MSB(addr), LSB(addr), MSB(cv_addr), LSB(cv_addr), bit, flag);
    
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

    cmdq_cv29_bit5(cmd->cmdq, MSB(addr), LSB(addr), cv29_bit5);

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

    cmdq_laddr(cmd->cmdq, MSB(addr), LSB(addr), MSB(laddr), LSB(laddr));

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

    cmdq_acc(cmd->cmdq, MSB(addr), LSB(addr), out, flag);

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
    cmdq_acc_ext(cmd->cmdq, MSB(addr), LSB(addr), (out << 7) | time);

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
    
    cmdq_acc_ext(cmd->cmdq, MSB(addr), LSB(addr), status);

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
    
            write_multi(writer, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                idx,
                entry->msb,
                entry->lsb,
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

static const int CMD_NUM_FLASH_VALUE_PER_LINE = 32;

static cmd_rc_t cmd_flash(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;

    flash_t *f = flash_object();

    write_multi(writer, "%d %d %d", f->read_idx, f->write_idx, f->page_no);
    
    for (int i = 0; i < FLASH_SECTOR_SIZE / CMD_NUM_FLASH_VALUE_PER_LINE; i++) {
        write_multi_start(writer);
        for (int j = 0; j < CMD_NUM_FLASH_VALUE_PER_LINE; j++) {
            write(writer, "%02x ", FLASH_TARGET_CONTENTS[i*CMD_NUM_FLASH_VALUE_PER_LINE+j]);
        }
        write_multi_end(writer);
    }
    write_eor(writer);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_flash_format(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;

    if (cfg_get_mt_enabled()) return CMD_RC_NOTEXEC;
    if (!cfg_store_format()) return CMD_RC_IOERR;
    write_success(writer, "%c", prot_true);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_reboot(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;

    cfg_set_mt_enabled(false);
    if (cfg_store()) {
        write_success(writer, "%c", prot_true);
    } else {
        write_error(writer, cmd_rvs[CMD_RC_IOERR]);
    };
    return CMD_RC_EXIT;
}

// command function
typedef cmd_rc_t (*cmd_fn_t)(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);

typedef struct {
    const char *command;
    const cmd_fn_t fn;
    const char *syntax;
    const char *help;
} cmd_command_entry_t;

typedef struct {
	const char *command;
    const cmd_fn_t fn;
} cmd_hidden_command_entry_t;

// command prototypes
static cmd_rc_t cmd_help(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer);

static const cmd_command_entry_t cmd_commands[] = {
	{"h",         cmd_help,           NULL,                     "help"},
	{"b",         cmd_board,          NULL,                     "board info"},
	{"s",         cmd_store,          NULL,                     "store"},
	{"t",         cmd_temp,           NULL,                     "temperature"},
	{"cv",        cmd_cv,             "<idx> [0..255]",         "configuration variables"},
    {"mte",       cmd_mt_enabled,     "[t|f]",                  "main track enabled"},
	{"ld",        cmd_loco_dir,       "<addr> [t|f|~]",         "loco direction"},
	{"ls",        cmd_loco_speed128,  "<addr> [0..127]",        "loco speed"},
	{"lf",        cmd_loco_fct,       "<addr> <no> [t|f|~]",    "loco function"},
	{"lcvbyte",   cmd_loco_cv_byte,   "<addr> <idx> 0..255",    "loco write cv byte"},
	{"lcvbit",    cmd_loco_cv_bit,    "<addr> <idx> 0..7 t|f",  "loco write cv bit"},
	{"lcv29bit5", cmd_loco_cv29_bit5, "<addr> t|f",             "loco set cv29 bit 5"},
	{"lladdr",    cmd_loco_laddr,     "<addr> <laddr>",         "loco set long address"},
	{"lcv1718",   cmd_loco_cv1718,    "<addr>",                 "loco calculate cv17 cv18"},
	{"af",        cmd_acc_fct,        "<addr> 0|1 t|f",         "accessory function"},
	{"at",        cmd_acc_time,       "<addr> 0|1 0..127",      "accessory time"},
	{"as",        cmd_acc_status,     "<addr> 0..255",          "accessory status"},
	{"ioadc",     cmd_io_adc,         "0..4",                   "io read adc input"},
	{"ioval",     cmd_io_value,       "<gpio> [t|f|~]",         "io gpio value"},
	{"iodir",     cmd_io_dir,         "<gpio> [t|f|~]",         "io gpio direction"},
	{"ioup",      cmd_io_pull_up,     "<gpio> [t|f|~]",         "io gpio pull-up"},
	{"iodown",    cmd_io_pull_down,   "<gpio> [t|f|~]",         "io gpio pull-down"},
};

// debugging
static const cmd_command_entry_t cmd_hidden_commands[] = {
    {"r",         cmd_rbuf},         // NULL,     "refresh buffer"
    {"rr",        cmd_rbuf_reset},   // NULL,     "refresh buffer reset"
    {"rd",        cmd_rbuf_del},     // "<addr>", "refresh buffer delete"
    
    {"f",         cmd_flash},        // NULL,     "flash",
    {"ff",        cmd_flash_format}, // NULL,     "flash format,

    {"reboot",    cmd_reboot},       // NULL,     "reboot pico",
};

static cmd_fn_t cmd_find_command(char *command) {
    for (int i = 0; i < count_of(cmd_commands); i++) {
        if (strcmp(command, cmd_commands[i].command) == 0) {
            return cmd_commands[i].fn;
        }
    }
    for (int i = 0; i < count_of(cmd_hidden_commands); i++) {
        if (strcmp(command, cmd_hidden_commands[i].command) == 0) {
            return cmd_hidden_commands[i].fn;
        }
    }
    return NULL;
}

static cmd_rc_t cmd_help(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;

    for (int i = 0; i < count_of(cmd_commands); i++) {
        if (cmd_commands[i].syntax == NULL) {
            write_multi(writer, "%s (%s)", cmd_commands[i].command, cmd_commands[i].help);
        } else {
            write_multi(writer, "%s %s (%s)", cmd_commands[i].command, cmd_commands[i].syntax, cmd_commands[i].help);
        }
    }
    write_eor(writer);
    return CMD_RC_OK;
}

void cmd_init(cmd_t *cmd, board_t *board, rbuf_t *rbuf, cmdq_t *cmdq) {
    cmd->board = board;
    cmd->rbuf  = rbuf;
    cmd->cmdq  = cmdq;
}

bool cmd_dispatch(cmd_t *cmd, reader_t *reader, writer_t *writer) {

    int num_prm = reader_num_prm(reader);
       
    cmd_fn_t fn = cmd_find_command(reader_get_prm(reader, 0));
    if (fn == NULL) {
        write_error(writer, cmd_rvs[CMD_RC_INVCMD]);
        return true; // continue
    }

    cmd_rc_t rc = fn(cmd, num_prm, reader, writer);
    if (rc == CMD_RC_EXIT) return false; // stop
    if (rc != CMD_RC_OK) write_error(writer, cmd_rvs[rc]);
    return true; // continue
}
