#include <stdio.h>
#include <string.h>

#include "cmd.h"
#include "board.h"
#include "io.h"
#include "exe.h"

inline static bool cmd_check_num_prm(int act, int min, int max) {
    return (act >=min && act <= max);
}

static cmd_command_t cmd_find_command(char *command) {
    for (int i = 0; i < CMD_MAX_COMMAND; i++) {
        if (strcmp(command, cmd_commands[i].command) == 0) {
            return (cmd_command_t) i+1;
        }
    }
    return CMD_COMMAND_NOP;
}

void cmd_init(cmd_t *cmd, board_t *board, rbuf_t *rbuf, channel_t *channel) {
    cmd->board = board;
    cmd->rbuf = rbuf;
    cmd->channel = channel;
    // channel_set_enabled(cmd->channel, false);   // start disabled
}

static cmd_rc_t cmd_help(cmd_t *cmd, int num_prm, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;

    for (int i = 0; i < CMD_MAX_COMMAND; i++) {
        if (cmd_commands[i].syntax == NULL) {
            write_multi(writer, "%s (%s)", cmd_commands[i].command, cmd_commands[i].help);
        } else {
            write_multi(writer, "%s %s (%s)", cmd_commands[i].command, cmd_commands[i].syntax, cmd_commands[i].help);
        }
    }
    write_eor(writer);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_board(cmd_t *cmd, int num_prm, writer_t *writer) {
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
        bool enabled = channel_get_enabled(cmd->channel);
        board_set_led(cmd->board, enabled);
        break;
    }

    write_success(writer, "%c", on?prot_true:prot_false);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_temp(cmd_t *cmd, int num_prm, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;
    float result = io_adc_read(IO_ADC_INPUT_TEMP);
    write_success(writer, "%f", 27 - (result - 0.706) / 0.001721);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_dcc_sync_bits(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 2)) return CMD_RC_INVNUMPRM;

    uint sync_bits;
    
    switch (num_prm) {
    case 1:
        sync_bits = channel_get_dcc_sync_bits(cmd->channel);
        break;
    case 2:
        if (!parse_uint(reader_get_prm(reader, 1), &sync_bits)) return CMD_RC_INVPRM;
        sync_bits = channel_set_dcc_sync_bits(cmd->channel, sync_bits);
        break;
    }
    
    write_success(writer, "%d", sync_bits);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_enabled(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 2)) return CMD_RC_INVNUMPRM;

    bool on;
    
    switch (num_prm) {
    case 1:
        on = channel_get_enabled(cmd->channel);
        break;
    case 2:
        if (!parse_bool(reader_get_prm(reader, 1), &on)) return CMD_RC_INVPRM;
        channel_set_enabled(cmd->channel, on);
        board_set_led(cmd->board, on);
        break;
    }

    write_success(writer, "%c", on?prot_true:prot_false);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_rbuf(cmd_t *cmd, int num_prm, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;

    write_multi(writer, "%d %d", cmd->rbuf->first, cmd->rbuf->next);
    if (cmd->rbuf->first != -1) {
        int idx = cmd->rbuf->first;
        do {
            volatile rbuf_entry_t *entry = &cmd->rbuf->buf[idx];
            f5_68_t f5_68 = entry->f5_68;
    
            write_multi(writer, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                idx,
                ADDR(entry->msb, entry->lsb),
                entry->num_refresh_cycle,
                entry->refresh_cycle,
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

static cmd_rc_t cmd_del_loco(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 2, 2)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!parse_uint(reader_get_prm(reader, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    if (!rbuf_deregister(cmd->rbuf, addr)) return CMD_RC_NODATA;

    write_success(writer, "%d", addr);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_loco_dir(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 2, 3)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!parse_uint(reader_get_prm(reader, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    prot_ternary_t ternary;
    bool dir;
    
    switch (num_prm) {
    case 2:
        if (!rbuf_get_dir(cmd->rbuf, addr, &dir)) return CMD_RC_NODATA;
        break;
    case 3:
        if (!parse_ternary(reader_get_prm(reader, 2), &ternary)) return CMD_RC_INVPRM;
        switch (ternary) {
        case PROT_TERNARY_FALSE:
            dir = false;
            if (!rbuf_set_dir(cmd->rbuf, addr, dir)) return CMD_RC_NOCHANGE;
            break;
        case PROT_TERNARY_TRUE:
            dir = true;
            if (!rbuf_set_dir(cmd->rbuf, addr, dir)) return CMD_RC_NOCHANGE;
            break;
        case PROT_TERNARY_TOGGLE:
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

    prot_ternary_t ternary;
    bool fct;
        
    switch (num_prm) {
    case 3:
        if (!rbuf_get_fct(cmd->rbuf, addr, no, &fct)) return CMD_RC_NODATA;
        break;
    case 4:
        if (!parse_ternary(reader_get_prm(reader, 3), &ternary)) return CMD_RC_INVPRM;
        switch (ternary) {
        case PROT_TERNARY_FALSE:
            fct = false;
            if (!rbuf_set_fct(cmd->rbuf, addr, no, fct)) return CMD_RC_NOCHANGE;
            break;
        case PROT_TERNARY_TRUE:
            fct = true;
            if (!rbuf_set_fct(cmd->rbuf, addr, no, fct)) return CMD_RC_NOCHANGE;
            break;
        case PROT_TERNARY_TOGGLE:
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

    channel_cv_byte(cmd->channel, MSB(addr), LSB(addr), MSB(cv_addr), LSB(cv_addr), cv);

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

    channel_cv_bit(cmd->channel, MSB(addr), LSB(addr), MSB(cv_addr), LSB(cv_addr), bit, flag);
    
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

    channel_cv29_bit5(cmd->channel, MSB(addr), LSB(addr), cv29_bit5);

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

    channel_laddr(cmd->channel, MSB(addr), LSB(addr), MSB(laddr), LSB(laddr));

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

static cmd_rc_t cmd_io_adc(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 2, 2)) return CMD_RC_INVNUMPRM;
    
    uint input;

    if (!parse_uint(reader_get_prm(reader, 1), &input)) return CMD_RC_INVPRM;
    if (!(input < IO_NUM_ADC_INPUT)) return CMD_RC_INVPRM;

    write_success(writer, "%f", io_adc_read(input));
    return CMD_RC_OK;
}

static cmd_rc_t cmd_io_cmdb(cmd_t *cmd, int num_prm, reader_t *reader, writer_t *writer) {
    if (!cmd_check_num_prm(num_prm, 3, 4)) return CMD_RC_INVNUMPRM;

    uint io_cmd;
    if (!parse_uint(reader_get_prm(reader, 1), &io_cmd)) return CMD_RC_INVPRM;
    if (!(io_cmd < IO_NUM_CMD)) return CMD_RC_INVPRM;

    uint gpio;
    if (!parse_uint(reader_get_prm(reader, 2), &gpio)) return CMD_RC_INVPRM;
    if (!io_is_gpio_avail(gpio)) return CMD_RC_INVGPIO;

    bool value;
    if (num_prm == 4) {
        if (!parse_bool(reader_get_prm(reader, 3), &value)) return CMD_RC_INVPRM;
    }

    value = io_exe_cmdb(io_cmd, gpio, value);
    write_success(writer, "%c", value?prot_true:prot_false);
    
    return CMD_RC_OK;
}

void cmd_dispatch(cmd_t *cmd, reader_t *reader, writer_t *writer) {

    int num_prm = reader_num_prm(reader);
       
    cmd_rc_t rc;
    
    switch (cmd_find_command(reader_get_prm(reader, 0))) {
    case CMD_COMMAND_NOP:            rc = CMD_RC_INVCMD;                                    break;
    case CMD_COMMAND_HELP:           rc = cmd_help(cmd, num_prm, writer);                   break;
    case CMD_COMMAND_BOARD:          rc = cmd_board(cmd, num_prm, writer);                  break;
    case CMD_COMMAND_LED:            rc = cmd_led(cmd, num_prm, reader, writer);            break;
    case CMD_COMMAND_TEMP:           rc = cmd_temp(cmd, num_prm, writer);                   break;
    case CMD_COMMAND_DCC_SYNC_BITS:  rc = cmd_dcc_sync_bits(cmd, num_prm, reader, writer);  break;
    case CMD_COMMAND_ENABLED:        rc = cmd_enabled(cmd, num_prm, reader, writer);        break;
    case CMD_COMMAND_RBUF:           rc = cmd_rbuf(cmd, num_prm, writer);                   break;
    case CMD_COMMAND_DEL_LOCO:       rc = cmd_del_loco(cmd, num_prm, reader, writer);       break;
    case CMD_COMMAND_LOCO_DIR:       rc = cmd_loco_dir(cmd, num_prm, reader, writer);       break;
    case CMD_COMMAND_LOCO_SPEED128:  rc = cmd_loco_speed128(cmd, num_prm, reader, writer);  break;
    case CMD_COMMAND_LOCO_FCT:       rc = cmd_loco_fct(cmd, num_prm, reader,writer);        break;
    case CMD_COMMAND_LOCO_CV_BYTE:   rc = cmd_loco_cv_byte(cmd, num_prm, reader, writer);   break;
    case CMD_COMMAND_LOCO_CV_BIT:    rc = cmd_loco_cv_bit(cmd, num_prm, reader, writer);    break;
    case CMD_COMMAND_LOCO_CV29_BIT5: rc = cmd_loco_cv29_bit5(cmd, num_prm, reader, writer); break;
    case CMD_COMMAND_LOCO_LADDR:     rc = cmd_loco_laddr(cmd, num_prm, reader, writer);     break;
    case CMD_COMMAND_LOCO_CV1718:    rc = cmd_loco_cv1718(cmd, num_prm, reader, writer);    break;
    case CMD_COMMAND_IO_ADC:         rc = cmd_io_adc(cmd, num_prm, reader, writer);         break;
    case CMD_COMMAND_IO_CMDB:        rc = cmd_io_cmdb(cmd, num_prm, reader, writer);        break;
    default:                         rc = CMD_RC_NOTIMPL;                                   break;
    }

    if (rc != CMD_RC_OK) write_error(writer, cmd_rvs[rc]);
    return;
}    
