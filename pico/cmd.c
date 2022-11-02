#include <stdio.h>
#include <limits.h>
#include <string.h>
#include "hardware/adc.h"

#include "cmd.h"
#include "board.h"
#include "io.h"
#include "exe.h"

inline static int read_char() {
    //TODO timeout + docu
    while (true) {
        int ch = getchar_timeout_us(10000000);
        if (ch != PICO_ERROR_TIMEOUT) return ch;
        //printf("TIMEOUT\n");
    }
}

inline static void write_char(int ch) {
    putchar(ch);
}

static void cmd_write_uint(int v)          { printf("%c%d\n", cmd_tag_success, v); }
static void cmd_write_byte(byte v)         { printf("%c%d\n", cmd_tag_success, v); }
//static void cmd_write_int(int v)           { printf("%c%d\n", cmd_res_success, v); }
static void cmd_write_double(double v)     { printf("%c%f\n", cmd_tag_success, v); }
static void cmd_write_bool(bool v)         { printf("%c%c\n", cmd_tag_success, v?cmd_char_true:cmd_char_false); }
static void cmd_write_string(char *v)      { printf("%c%s\n", cmd_tag_success, v); }
static void cmd_write_error(cmd_rc_t rc)   { printf("%c%s\n", cmd_tag_nosuccess, cmd_rvs[rc]); }
static void cmd_write_eor()                { printf("%c\n",   cmd_tag_eor); }

static void cmd_write_cv1718(byte cv17, byte cv18) { printf("%c%d %d\n", cmd_tag_success, cv17, cv18); }

static void cmd_write_rbuf(int first, int next)    { printf("%c%d %d\n", cmd_tag_multi, first, next); }

static void cmd_write_rbuf_entry(int idx, volatile rbuf_entry_t *entry) {
    f5_68_t f5_68 = entry->f5_68;
    printf("%c%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
        cmd_tag_multi,
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
}

static void cmd_write_help() {
    for (int i = 0; i < CMD_MAX_COMMAND; i++) {
        if (cmd_commands[i].syntax == NULL) {
            printf("%c%s (%s)\n", cmd_tag_multi, cmd_commands[i].command, cmd_commands[i].help);
        } else {
            printf("%c%s %s (%s)\n", cmd_tag_multi, cmd_commands[i].command, cmd_commands[i].syntax, cmd_commands[i].help);
        }
    }
    cmd_write_eor();
}

static void cmd_write_board(cmd_t *cmd) {
    switch (cmd->board->type) {
    case BOARD_TYPE_PICO:   printf("%cpico",   cmd_tag_success); break;
    case BOARD_TYPE_PICO_W: printf("%cpico_w", cmd_tag_success); break;
    }

    for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; ++i) {
        printf(" %02x", cmd->board->board_id.id[i]);
    }
    printf("\n");
}

static bool cmd_parse_uint(char *ptr, uint *v) {
    //TODO overflow
    *v = 0;
    while (*ptr != cmd_char_null) {
        if (*ptr < '0' || *ptr > '9') return false;
        // printf("loop %c\n", *ptr);
        *v = (*v * 10) + (*ptr - '0');
        // printf("%d\n", *v);
        *ptr++;
    }
    return true;
}

static bool cmd_parse_byte(char *ptr, byte *v) {
    uint u;
    if (!cmd_parse_uint(ptr, &u)) return false;
    if (u > UCHAR_MAX) return false;
    *v = (byte) u;
    return true;
}

static bool cmd_parse_bool(char *ptr, bool *v) {
    *v = false;
    if (strlen(ptr) != 1) return false;
    switch (*ptr) {
    case cmd_char_true:  *v = true;  return true;
    case cmd_char_false: *v = false; return true;
    }
    return false;
}

static bool cmd_parse_ternary(char *ptr, cmd_ternary_t *v) {
    *v = CMD_TERNARY_FALSE;
    if (strlen(ptr) != 1) return false;
    switch (*ptr) {
    case cmd_char_true:   *v = CMD_TERNARY_TRUE;   return true;
    case cmd_char_false:  *v = CMD_TERNARY_FALSE;  return true;
    case cmd_char_toggle: *v = CMD_TERNARY_TOGGLE; return true;
    }
    return false;
}

static int cmd_num_prm(cmd_t *cmd) {
    int num = 0;
    for (int i = 0; i < cmd->pos; i++) {
        if (cmd->buf[i] == '\0') num++;
    }
    return num;
}

inline static bool cmd_check_num_prm(int act, int min, int max) {
    return (act >=min && act <= max);
}

static char *cmd_prm(cmd_t *cmd, int idx) {
    int start = 0;
    int part = 0;

    for (int i = 0; i < cmd->pos; i++) {
        if (cmd->buf[i] == '\0') {
            if (part == idx) {
                return &cmd->buf[start];
            }
            part++;
            start = i+1;
        }
    }
    return NULL;
}

/*
static ReturnCode cmd_init_gpio(cmd_t *cmd, const ReqInitGPIO *req) {
    if (!req->out) return ReturnCode_NOTIMPL;
        
    switch (io_init_out(req->no)) {
    case IO_OK:   return ReturnCode_OK;       break;
    case IO_INV:  return ReturnCode_INVGPIO;  break;
    case IO_RSRV: return ReturnCode_RSRVGPIO; break;
    default:      return ReturnCode_NOTIMPL;  break;
    }
}

static ReturnCode cmd_set_gpio(cmd_t *cmd, const ReqSetGPIO *req) {
    switch (io_set_gp(req->no, req->on)) {
    case IO_OK:    return ReturnCode_OK;        break;
    case IO_INV:   return ReturnCode_INVGPIO;   break;
    case IO_NOOUT: return ReturnCode_NOOUTGPIO; break;
    default:       return ReturnCode_NOTIMPL;   break;
    }
}
*/

static cmd_token_t cmd_next_token(int *ch) {
    *ch = read_char();
    if (*ch == cmd_tag_start) return CMD_TOKEN_START;
    if (*ch == cmd_char_cr || *ch == cmd_char_nl || *ch == cmd_char_null) return CMD_TOKEN_END;
    if (*ch == cmd_char_quote) return CMD_TOKEN_STRING;
    if (*ch == cmd_char_space) return CMD_TOKEN_WHITESPACE;
    return CMD_TOKEN_CHAR;
}

static void cmd_buf_write(cmd_t *cmd, int ch) {
    if (cmd->pos < CMD_BUFFER_SIZE) {
        cmd->buf[cmd->pos++] = (char) ch;
    }
}

static void cmd_buf_end(cmd_t *cmd) {
    if (cmd->pos == 0 || cmd->buf[cmd->pos-1] != cmd_char_null) cmd_buf_write(cmd, cmd_char_null);
}

static bool cmd_read_frame(cmd_t *cmd) {
    cmd->pos = 0;
    int ch;
    bool start = false;
    
    while (cmd_next_token(&ch) != CMD_TOKEN_START);

    while (true) {

        switch (cmd_next_token(&ch)) {

        case CMD_TOKEN_END:
            cmd_buf_end(cmd);
            return (cmd->pos < CMD_BUFFER_SIZE);

        case CMD_TOKEN_WHITESPACE:
            if (start) {
                cmd_buf_write(cmd, cmd_char_null);
                start = false;
            }
            break;

        case CMD_TOKEN_START: // handle like TOKEN_CHAR
        case CMD_TOKEN_CHAR:
            start = true;
            cmd_buf_write(cmd, ch);
            break;

        case CMD_TOKEN_STRING:
            //TODO
            break;
        }
    }
}

static cmd_command_t cmd_find_command(char *command) {
    for (int i = 0; i < CMD_MAX_COMMAND; i++) {
        if (strcmp(command, cmd_commands[i].command) == 0) {
            return (cmd_command_t) i+1;
        }
    }
    return CMD_COMMAND_NOP;
}

inline static void cmd_set_flag(cmd_t *cmd, bool on, byte flag) {
    if (on == true) {
        cmd->flags |= flag;
    } else {
        cmd->flags &= ~flag;
    }
}
inline static bool cmd_get_flag(cmd_t *cmd, byte flag) {return ((cmd->flags & flag) != 0);}

inline static void cmd_set_led(cmd_t *cmd) {
    // io_get_led();
    board_set_led(cmd->board, cmd_get_flag(cmd, CMD_FLAG_ENABLED) && cmd_get_flag(cmd, CMD_FLAG_LED));
}

void cmd_init(cmd_t *cmd, board_t *board, rbuf_t *rbuf, channel_t *channel) {
    cmd->board = board;
    cmd->rbuf = rbuf;
    cmd->channel = channel;
    channel_set_enabled(cmd->channel, false);   // start disabled
    cmd_set_flag(cmd, false, CMD_FLAG_ENABLED); // set enabled off
    cmd_set_flag(cmd, true, CMD_FLAG_LED);  // set led on 
}

static cmd_rc_t cmd_help(cmd_t *cmd, int num_prm) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;

    cmd_write_help();
    return CMD_RC_OK;
}

static cmd_rc_t cmd_board(cmd_t *cmd, int num_prm) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;

    cmd_write_board(cmd);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_led(cmd_t *cmd, int num_prm) {
    if (!cmd_check_num_prm(num_prm, 1, 2)) return CMD_RC_INVNUMPRM;

    bool on;
        
    switch (num_prm) {
    case 1:
        on = cmd_get_flag(cmd, CMD_FLAG_LED);
        break;
    case 2:
        if (!cmd_parse_bool(cmd_prm(cmd, 1), &on)) return CMD_RC_INVPRM;
        cmd_set_flag(cmd, on, CMD_FLAG_LED);
        cmd_set_led(cmd);
        break;
    }
    cmd_write_bool(on);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_temp(cmd_t *cmd, int num_prm) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;
    adc_select_input(4); // internal temperature sensor 
    // 12-bit conversion, assume max value == ADC_VREF == 3.3 V
    const double conversion_factor = 3.3f / (1 << 12);
    uint16_t result = adc_read();
    cmd_write_double(27 - ((result * conversion_factor) - 0.706) / 0.001721);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_dcc_sync_bits(cmd_t *cmd, int num_prm) {
    if (!cmd_check_num_prm(num_prm, 1, 2)) return CMD_RC_INVNUMPRM;

    uint sync_bits;
    
    switch (num_prm) {
    case 1:
        sync_bits = channel_get_dcc_sync_bits(cmd->channel);
        break;
    case 2:
        if (!cmd_parse_uint(cmd_prm(cmd, 1), &sync_bits)) return CMD_RC_INVPRM;
        sync_bits = channel_set_dcc_sync_bits(cmd->channel, sync_bits);
        break;
    }
    cmd_write_uint(sync_bits);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_enabled(cmd_t *cmd, int num_prm) {
    if (!cmd_check_num_prm(num_prm, 1, 2)) return CMD_RC_INVNUMPRM;

    bool on;
    
    switch (num_prm) {
    case 1:
        on = cmd_get_flag(cmd, CMD_FLAG_ENABLED);
        //on = channel_get_enabled(cmd->channel);
        break;
    case 2:
        if (!cmd_parse_bool(cmd_prm(cmd, 1), &on)) return CMD_RC_INVPRM;
        channel_set_enabled(cmd->channel, on);
        cmd_set_flag(cmd, on, CMD_FLAG_ENABLED);
        cmd_set_led(cmd);
        break;
    }
    cmd_write_bool(on);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_rbuf(cmd_t *cmd, int num_prm) {
    if (!cmd_check_num_prm(num_prm, 1, 1)) return CMD_RC_INVNUMPRM;

    cmd_write_rbuf(cmd->rbuf->first, cmd->rbuf->next);
    if (cmd->rbuf->first != -1) {
        int idx = cmd->rbuf->first;
        do {
            cmd_write_rbuf_entry(idx, &cmd->rbuf->buf[idx]);
            idx = cmd->rbuf->buf[idx].next;
        } while (idx != cmd->rbuf->first);
    }
    cmd_write_eor();
    return CMD_RC_OK;
}

static cmd_rc_t cmd_del_loco(cmd_t *cmd, int num_prm) {
    if (!cmd_check_num_prm(num_prm, 2, 2)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!cmd_parse_uint(cmd_prm(cmd, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    if (!rbuf_deregister(cmd->rbuf, addr)) return CMD_RC_NODATA;

    cmd_write_uint(addr);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_loco_dir(cmd_t *cmd, int num_prm) {
    if (!cmd_check_num_prm(num_prm, 2, 3)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!cmd_parse_uint(cmd_prm(cmd, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    cmd_ternary_t ternary;
    bool dir;
    
    switch (num_prm) {
    case 2:
        if (!rbuf_get_dir(cmd->rbuf, addr, &dir)) return CMD_RC_NODATA;
        break;
    case 3:
        if (!cmd_parse_ternary(cmd_prm(cmd, 2), &ternary)) return CMD_RC_INVPRM;
        switch (ternary) {
        case CMD_TERNARY_FALSE:
            dir = false;
            if (!rbuf_set_dir(cmd->rbuf, addr, dir)) return CMD_RC_NOCHANGE;
            break;
        case CMD_TERNARY_TRUE:
            dir = true;
            if (!rbuf_set_dir(cmd->rbuf, addr, dir)) return CMD_RC_NOCHANGE;
            break;
        case CMD_TERNARY_TOGGLE:
            if (!rbuf_toggle_dir(cmd->rbuf, addr, &dir)) return CMD_RC_NODATA;
            break;
        }
        break;
    }
    cmd_write_bool(dir);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_loco_speed128(cmd_t *cmd, int num_prm) {
    if (!cmd_check_num_prm(num_prm, 2, 3)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!cmd_parse_uint(cmd_prm(cmd, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    byte speed128;
        
    switch (num_prm) {
    case 2:
        if (!rbuf_get_speed128(cmd->rbuf, addr, &speed128)) return CMD_RC_NODATA;
        break;
    case 3:
        if (!cmd_parse_byte(cmd_prm(cmd, 2), &speed128)) return CMD_RC_INVPRM;
        if (!dcc_check_loco_speed128(speed128)) return CMD_RC_INVPRM;
        if (!rbuf_set_speed128(cmd->rbuf, addr, speed128)) return CMD_RC_NOCHANGE;
        break;
    }
    cmd_write_byte(speed128);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_loco_fct(cmd_t *cmd, int num_prm) {
    if (!cmd_check_num_prm(num_prm, 3, 4)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!cmd_parse_uint(cmd_prm(cmd, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    byte no;
    if (!cmd_parse_byte(cmd_prm(cmd, 2), &no)) return CMD_RC_INVPRM;

    cmd_ternary_t ternary;
    bool fct;
        
    switch (num_prm) {
    case 3:
        if (!rbuf_get_fct(cmd->rbuf, addr, no, &fct)) return CMD_RC_NODATA;
        break;
    case 4:
        if (!cmd_parse_ternary(cmd_prm(cmd, 3), &ternary)) return CMD_RC_INVPRM;
        switch (ternary) {
        case CMD_TERNARY_FALSE:
            fct = false;
            if (!rbuf_set_fct(cmd->rbuf, addr, no, fct)) return CMD_RC_NOCHANGE;
            break;
        case CMD_TERNARY_TRUE:
            fct = true;
            if (!rbuf_set_fct(cmd->rbuf, addr, no, fct)) return CMD_RC_NOCHANGE;
            break;
        case CMD_TERNARY_TOGGLE:
            if (!rbuf_toggle_fct(cmd->rbuf, addr, no, &fct)) return CMD_RC_NODATA;
            break;
        }
        break;
    }
    cmd_write_bool(fct);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_loco_cv_byte(cmd_t *cmd, int num_prm) {
    if (!cmd_check_num_prm(num_prm, 4, 4)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!cmd_parse_uint(cmd_prm(cmd, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    uint idx;
    if (!cmd_parse_uint(cmd_prm(cmd, 2), &idx)) return CMD_RC_INVPRM;
    if (!dcc_check_cv_idx(idx)) return CMD_RC_INVPRM;

    byte cv;
    if (!cmd_parse_byte(cmd_prm(cmd, 3), &cv)) return CMD_RC_INVPRM;
    if (!dcc_check_cv(cv)) return CMD_RC_INVPRM;

    // cv address is zero based -> cv index 1 is address 0
    uint cv_addr = idx - 1;

    channel_cv_byte(cmd->channel, MSB(addr), LSB(addr), MSB(cv_addr), LSB(cv_addr), cv);

    cmd_write_byte(cv);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_loco_cv_bit(cmd_t *cmd, int num_prm) {
    if (!cmd_check_num_prm(num_prm, 5, 5)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!cmd_parse_uint(cmd_prm(cmd, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    uint idx;
    if (!cmd_parse_uint(cmd_prm(cmd, 2), &idx)) return CMD_RC_INVPRM;
    if (!dcc_check_cv_idx(idx)) return CMD_RC_INVPRM;

    byte bit;
    if (!cmd_parse_byte(cmd_prm(cmd, 3), &bit)) return CMD_RC_INVPRM;
    if (!dcc_check_bit(bit)) return CMD_RC_INVPRM;

    bool flag;
    if (!cmd_parse_bool(cmd_prm(cmd, 4), &flag)) return CMD_RC_INVPRM;

    // cv address is zero based -> cv index 1 is address 0
    uint cv_addr = idx - 1;

    channel_cv_bit(cmd->channel, MSB(addr), LSB(addr), MSB(cv_addr), LSB(cv_addr), bit, flag);
    
    cmd_write_bool(flag);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_loco_cv29_bit5(cmd_t *cmd, int num_prm) {
    if (!cmd_check_num_prm(num_prm, 3, 3)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!cmd_parse_uint(cmd_prm(cmd, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    bool cv29_bit5;
    if (!cmd_parse_bool(cmd_prm(cmd, 2), &cv29_bit5)) return CMD_RC_INVPRM;

    channel_cv29_bit5(cmd->channel, MSB(addr), LSB(addr), cv29_bit5);

    cmd_write_bool(cv29_bit5);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_loco_laddr(cmd_t *cmd, int num_prm) {
    if (!cmd_check_num_prm(num_prm, 3, 3)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!cmd_parse_uint(cmd_prm(cmd, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    uint laddr;
    if (!cmd_parse_uint(cmd_prm(cmd, 2), &laddr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(laddr)) return CMD_RC_INVPRM;

    channel_laddr(cmd->channel, MSB(addr), LSB(addr), MSB(laddr), LSB(laddr));

    cmd_write_uint(laddr);
    return CMD_RC_OK;
}

static cmd_rc_t cmd_loco_cv1718(cmd_t *cmd, int num_prm) {
    if (!cmd_check_num_prm(num_prm, 2, 2)) return CMD_RC_INVNUMPRM;

    uint addr;
    if (!cmd_parse_uint(cmd_prm(cmd, 1), &addr)) return CMD_RC_INVPRM;
    if (!dcc_check_loco_addr(addr)) return CMD_RC_INVPRM;

    byte cv17 = 0xc0 | (byte) (addr>>8);
    byte cv18 = (byte) addr;

    cmd_write_cv1718(cv17, cv18);
    return CMD_RC_OK;
}

void cmd_dispatch(cmd_t *cmd) {

    if (!cmd_read_frame(cmd)) {
        cmd_write_error(CMD_RC_INVCMD);
        return;
    };

    int num_prm = cmd_num_prm(cmd);
    //printf("num parts %d pos %d\n", num_prm, cmd->pos);
    
    cmd_rc_t rc;
    
    switch (cmd_find_command(cmd_prm(cmd, 0))) {
    case CMD_COMMAND_NOP:            rc = CMD_RC_INVCMD;                    break;
    case CMD_COMMAND_HELP:           rc = cmd_help(cmd, num_prm);           break;
    case CMD_COMMAND_BOARD:          rc = cmd_board(cmd, num_prm);          break;
    case CMD_COMMAND_LED:            rc = cmd_led(cmd, num_prm);            break;
    case CMD_COMMAND_TEMP:           rc = cmd_temp(cmd, num_prm);           break;
    case CMD_COMMAND_DCC_SYNC_BITS:  rc = cmd_dcc_sync_bits(cmd, num_prm);  break;
    case CMD_COMMAND_ENABLED:        rc = cmd_enabled(cmd, num_prm);        break;
    case CMD_COMMAND_RBUF:           rc = cmd_rbuf(cmd, num_prm);           break;
    case CMD_COMMAND_DEL_LOCO:       rc = cmd_del_loco(cmd, num_prm);       break;
    case CMD_COMMAND_LOCO_DIR:       rc = cmd_loco_dir(cmd, num_prm);       break;
    case CMD_COMMAND_LOCO_SPEED128:  rc = cmd_loco_speed128(cmd, num_prm);  break;
    case CMD_COMMAND_LOCO_FCT:       rc = cmd_loco_fct(cmd, num_prm);       break;
    case CMD_COMMAND_LOCO_CV_BYTE:   rc = cmd_loco_cv_byte(cmd, num_prm);   break;
    case CMD_COMMAND_LOCO_CV_BIT:    rc = cmd_loco_cv_bit(cmd, num_prm);    break;
    case CMD_COMMAND_LOCO_CV29_BIT5: rc = cmd_loco_cv29_bit5(cmd, num_prm); break;
    case CMD_COMMAND_LOCO_LADDR:     rc = cmd_loco_laddr(cmd, num_prm);     break;
    case CMD_COMMAND_LOCO_CV1718:    rc = cmd_loco_cv1718(cmd, num_prm);    break;
    default:                         rc = CMD_RC_NOTIMPL;                   break;
    }

    if (rc != CMD_RC_OK) cmd_write_error(rc);
    return;
}    
