#include <stdio.h>
#include "pico/binary_info.h"

#include "rbuf.h"

enum {
    RBUF_ERROR_NONE        =  0,
    RBUF_ERROR_NOT_FOUND   = -1,
    RBUF_ERROR_BUFFER_FULL = -2,
};

static const int TIMEOUT_US = 100; // timeout in micro seconds

static const uint8_t  ONE_U8  = 1;
static const uint64_t ONE_U64 = 1;

typedef byte (*byte_getter)(rbuf_t*, uint);
typedef bool (*byte_setter)(rbuf_t*, uint, byte);
typedef bool (*bool_getter)(rbuf_t*, uint);
typedef bool (*bool_setter)(rbuf_t*, uint, bool);
typedef bool (*bool_idx_getter)(rbuf_t*, uint, byte);
typedef bool (*bool_idx_setter)(rbuf_t*, uint, byte, bool);

// rbuf_entry

static inline void rbuf_entry_clear_attr(volatile rbuf_entry_t *entry) {
    // some decoders cannot handle extended function formats, so
    // - the default is set to restrict the refresh cycle to functions 0-12
    // - if a function > 12 is selected, the refresh cycle is extended
    entry->max_refresh_cmd = CMDCH_F9_12;
    entry->refresh_cmd = CMDCH_DIR_SPEED;
    entry->dir_speed = 0;
    entry->f0_4 = 0;
    entry->f5_68.f5_68 = 0;
}

static inline void rbuf_entry_clear(volatile rbuf_entry_t *entry) {
    entry->msb = 0;
    entry->lsb = 0;
    rbuf_entry_clear_attr(entry);
    entry->prev = 0;
    entry->next = 0;
}

// rbuf

static inline bool rbuf_try_lock(rbuf_t *rbuf) {
    return mutex_enter_timeout_us(&rbuf->mu, TIMEOUT_US);
}

static inline void rbuf_lock(rbuf_t *rbuf) {
    //printf("enter lock \n");
    mutex_enter_blocking(&rbuf->mu);
    //printf("exit lock \n");
}

static inline void rbuf_unlock(rbuf_t *rbuf) {
    //printf("enter unlock \n");
    mutex_exit(&rbuf->mu);
    //printf("exit unlock \n");
}

static inline bool rbuf_get_dir_internal(rbuf_t *rbuf, uint idx) {
    return (rbuf->buf[idx].dir_speed & 0x80) != 0 ? true : false;
}

static inline bool rbuf_set_dir_internal(rbuf_t *rbuf, uint idx, bool dir) {
    if (rbuf_get_dir_internal(rbuf, idx) == dir) {
        return false; // not changed
    } 
    byte m = dir ? 0x80 : 0x00;
    rbuf->buf[idx].dir_speed = m | (rbuf->buf[idx].dir_speed & 0x7f);
    cmdch_dir_speed(rbuf->cmdch, rbuf->buf[idx].msb, rbuf->buf[idx].lsb, rbuf->buf[idx].dir_speed);
    return true;
}

static inline byte rbuf_get_speed128_internal(rbuf_t *rbuf, uint idx) {
    return rbuf->buf[idx].dir_speed & 0x7f;
}

static inline bool rbuf_set_speed128_internal(rbuf_t *rbuf, uint idx, byte speed) {
    if (rbuf_get_speed128_internal(rbuf, idx) == speed) {
        return false; // not changed
    }
    rbuf->buf[idx].dir_speed = (rbuf->buf[idx].dir_speed & 0x80) | (speed & 0x7f);
    cmdch_dir_speed(rbuf->cmdch, rbuf->buf[idx].msb, rbuf->buf[idx].lsb, rbuf->buf[idx].dir_speed);
    return true;
}

static inline bool rbuf_get_fct_internal(rbuf_t *rbuf, uint idx, byte no) {
    if (no == 0) return (rbuf->buf[idx].f0_4 >> 4) != 0;
    if (no < 5) {
        no -= 1; // zero based
        return (rbuf->buf[idx].f0_4 & (ONE_U8 << no)) != 0;
    }
    no -= 5; // zero based
    return (rbuf->buf[idx].f5_68.f5_68 & (ONE_U64 << no)) != 0;
}

static inline bool rbuf_set_fct_internal(rbuf_t *rbuf, uint idx, byte no, bool v) {
    if (rbuf_get_fct_internal(rbuf, idx, no) == v) return false; // not changed

    byte msb = rbuf->buf[idx].msb;
    byte lsb = rbuf->buf[idx].lsb;
    byte i;

    if (no == 0) {
        byte f0_4 = rbuf->buf[idx].f0_4;
        f0_4 = v ? f0_4 | 0x10 : f0_4 & ~0x10;
        rbuf->buf[idx].f0_4 = f0_4;
        cmdch_f0_4(rbuf->cmdch, msb, lsb, f0_4);
    } else if (no < 5) {
        i = no - 1; // zero based
        byte f0_4 = rbuf->buf[idx].f0_4;
        f0_4 = v ? f0_4 | (ONE_U8 << i) : f0_4 & ~(ONE_U8 << i);
        rbuf->buf[idx].f0_4 = f0_4;
        cmdch_f0_4(rbuf->cmdch, msb, lsb, f0_4);
    } else {
        i = no - 5; // zero based
        f5_68_t f5_68;
        f5_68.f5_68 = rbuf->buf[idx].f5_68.f5_68;
        f5_68.f5_68 = v ? f5_68.f5_68 | (ONE_U64 << i) : f5_68.f5_68 & ~(ONE_U64 << i);
        rbuf->buf[idx].f5_68.f5_68 = f5_68.f5_68;

        switch (i / 8) {
        case 0:
            if ((i & 8) <= 4) {
                cmdch_f5_8(rbuf->cmdch, msb, lsb, f5_68.f5_8);
            } else {
                cmdch_f9_12(rbuf->cmdch, msb, lsb, f5_68.f9_12);
            }
            break;
        case 1: cmdch_f13_20(rbuf->cmdch, msb, lsb, f5_68.f13_20); rbuf->buf[idx].max_refresh_cmd = CMDCH_F13_20; break;
        case 2: cmdch_f21_28(rbuf->cmdch, msb, lsb, f5_68.f21_28); rbuf->buf[idx].max_refresh_cmd = CMDCH_F21_28; break;
        case 3: cmdch_f29_36(rbuf->cmdch, msb, lsb, f5_68.f29_36); rbuf->buf[idx].max_refresh_cmd = CMDCH_F29_36; break;
        case 4: cmdch_f37_44(rbuf->cmdch, msb, lsb, f5_68.f37_44); rbuf->buf[idx].max_refresh_cmd = CMDCH_F37_44; break;
        case 5: cmdch_f45_52(rbuf->cmdch, msb, lsb, f5_68.f45_52); rbuf->buf[idx].max_refresh_cmd = CMDCH_F45_52; break;
        case 6: cmdch_f53_60(rbuf->cmdch, msb, lsb, f5_68.f53_60); rbuf->buf[idx].max_refresh_cmd = CMDCH_F53_60; break;
        case 7: cmdch_f61_68(rbuf->cmdch, msb, lsb, f5_68.f61_68); rbuf->buf[idx].max_refresh_cmd = CMDCH_F61_68; break;
        }
    }
    return true;
}

static int rbuf_get_index(rbuf_t *rbuf, uint addr, uint *idx) {
    for (uint i = 0; i < RBUF_SIZE; i++) {
        *idx = (addr+i) % RBUF_SIZE;
        int cmpAddr = ADDR(rbuf->buf[*idx].msb, rbuf->buf[*idx].lsb);
        if (addr == cmpAddr) return RBUF_ERROR_NONE;
        if (cmpAddr == 0) return RBUF_ERROR_NOT_FOUND;
    }
    *idx = rbuf->buf[rbuf->first].prev; // last index (for replacement)
    return RBUF_ERROR_BUFFER_FULL;
}

static void rbuf_insert_loco(rbuf_t *rbuf, uint addr, uint idx) {
    //printf("enter insert loco %i ind %i \n", loco, idx);

    int first, last;

    if (rbuf->first == -1) {
        rbuf->first = idx;
        rbuf->next = idx;
        first = idx;
        last = idx;
    } else {
        first = rbuf->first;
        last = rbuf->buf[first].prev;
    }

    //printf("first %i last %i idx %i \n", first, last, idx);

    rbuf->buf[idx].msb = MSB(addr);
    rbuf->buf[idx].lsb = LSB(addr);
    
    rbuf->buf[idx].next = first;
    rbuf->buf[idx].prev = last;
    rbuf->buf[first].prev = idx;
    rbuf->buf[last].next = idx;

    // default values
    
    // DEBUG
    //rbuf->buf[idx].dir_speed = 0x80; // forward
    
    //printf("exit insert loco %i ind %i \n", loco, idx);
}

static void rbuf_replace_loco(rbuf_t *rbuf, uint addr, uint idx, bool clear) {
    //printf("enter replace loco %i ind %i \n", loco, idx);
    rbuf->buf[idx].msb = MSB(addr);
    rbuf->buf[idx].lsb = LSB(addr);
    if (clear) rbuf_entry_clear_attr(&rbuf->buf[idx]);
}

static void rbuf_remove_loco(rbuf_t *rbuf, uint idx) {
    int prev = rbuf->buf[idx].prev;
    int next = rbuf->buf[idx].next;

    if (idx == next) { // only one entry (idx == next && idx == first)
        rbuf->first = -1;
        rbuf->next  = -1;
    } else {
        rbuf->buf[prev].next = next;
        rbuf->buf[next].prev = prev;
        rbuf->first = next;
        if (idx == rbuf->next) rbuf->next = next;
    }
    rbuf_entry_clear(&rbuf->buf[idx]);
}

static bool rbuf_get_byte(rbuf_t *rbuf, uint addr, byte_getter f, byte *b) {
    uint idx;
    if (rbuf_get_index(rbuf, addr, &idx) != RBUF_ERROR_NONE) return false;
    *b = f(rbuf, idx);
    return true;
}

static bool rbuf_set_byte(rbuf_t *rbuf, uint addr, byte_setter f, byte b) {
    uint idx;
    int err = rbuf_get_index(rbuf, addr, &idx);
    rbuf_lock(rbuf);
    switch (err) {
    case RBUF_ERROR_NOT_FOUND:   rbuf_insert_loco(rbuf, addr, idx); break;
    case RBUF_ERROR_BUFFER_FULL: rbuf_replace_loco(rbuf, addr, idx, true); break;
    }
    bool rv = f(rbuf, idx, b);
    rbuf->first = idx;
    rbuf_unlock(rbuf);
    return rv;
}

static bool rbuf_get_bool(rbuf_t *rbuf, uint addr, bool_getter f, bool *b) {
    uint idx;
    if (rbuf_get_index(rbuf, addr, &idx) != RBUF_ERROR_NONE) return false;
    *b = f(rbuf, idx);
    return true;
}

static bool rbuf_set_bool(rbuf_t *rbuf, uint addr, bool_setter f, bool b) {
    uint idx;
    int err = rbuf_get_index(rbuf, addr, &idx);
    rbuf_lock(rbuf);
    switch (err) {
    case RBUF_ERROR_NOT_FOUND:   rbuf_insert_loco(rbuf, addr, idx); break;
    case RBUF_ERROR_BUFFER_FULL: rbuf_replace_loco(rbuf, addr, idx, true); break;
    }
    bool rv = f(rbuf, idx, b);
    rbuf->first = idx;
    rbuf_unlock(rbuf);
    return rv;
}

static bool rbuf_toggle_bool(rbuf_t *rbuf, uint addr, bool_getter g, bool_setter s, bool *b) {
    uint idx;
    if (rbuf_get_index(rbuf, addr, &idx) != RBUF_ERROR_NONE) return false;
    rbuf_lock(rbuf);
    *b = !g(rbuf, idx);
    bool rv = s(rbuf, idx, *b);
    rbuf->first = idx;
    rbuf_unlock(rbuf);
    return rv;
}

static bool rbuf_get_idx_bool(rbuf_t *rbuf, uint addr, bool_idx_getter f, byte no, bool *b) {
    uint idx;
    if (rbuf_get_index(rbuf, addr, &idx) != RBUF_ERROR_NONE) return false;
    *b = f(rbuf, idx, no);
    return true;
}

static bool rbuf_set_idx_bool(rbuf_t *rbuf, uint addr, bool_idx_setter f, byte no, bool b) {
    uint idx;
    int err = rbuf_get_index(rbuf, addr, &idx);
    rbuf_lock(rbuf);
    switch (err) {
    case RBUF_ERROR_NOT_FOUND:   rbuf_insert_loco(rbuf, addr, idx); break;
    case RBUF_ERROR_BUFFER_FULL: rbuf_replace_loco(rbuf, addr, idx, true); break;
    }
    bool rv = f(rbuf, idx, no, b);
    rbuf->first = idx;
    rbuf_unlock(rbuf);
    return rv;
}

static bool rbuf_toggle_idx_bool(rbuf_t *rbuf, uint addr, bool_idx_getter g, bool_idx_setter s, byte no, bool *b) {
    uint idx;
    if (rbuf_get_index(rbuf, addr, &idx) != RBUF_ERROR_NONE) return false;
    rbuf_lock(rbuf);
    *b = !g(rbuf, idx, no);
    bool rv = s(rbuf, idx, no, *b);
    rbuf->first = idx;
    rbuf_unlock(rbuf);
    return rv;
}

static void rbuf_reset_internal(rbuf_t *rbuf) {
    rbuf->first = -1;
    rbuf->next = -1;
    for (int i = 0; i < RBUF_SIZE; i++) {
        rbuf_entry_clear(&rbuf->buf[i]);
    }
}

#define RBUF_TEXT "Refresh buffer size " RBUF_SIZE_STRING

void rbuf_init(rbuf_t *rbuf, cmdch_t *cmdch) {
    bi_decl(bi_program_feature(RBUF_TEXT));

    mutex_init(&(rbuf->mu));
    rbuf->cmdch = cmdch;
    rbuf_reset_internal(rbuf);
}

void rbuf_reset(rbuf_t *rbuf) {
    rbuf_lock(rbuf);
    rbuf_reset_internal(rbuf);
    rbuf_unlock(rbuf);
}

bool rbuf_del(rbuf_t *rbuf, uint addr) {
    uint idx;
    if (rbuf_get_index(rbuf, addr, &idx) != RBUF_ERROR_NONE) return false;
    rbuf_lock(rbuf);
    rbuf_remove_loco(rbuf, idx);
    rbuf_unlock(rbuf);
    return true;
}

bool rbuf_get_dir(rbuf_t *rbuf, uint addr, bool *dir) {
    return rbuf_get_bool(rbuf, addr, rbuf_get_dir_internal, dir);
}

bool rbuf_set_dir(rbuf_t *rbuf, uint addr, bool dir) {
    return rbuf_set_bool(rbuf, addr, rbuf_set_dir_internal, dir);
}

bool rbuf_toggle_dir(rbuf_t *rbuf, uint addr, bool *dir) {
    return rbuf_toggle_bool(rbuf, addr, rbuf_get_dir_internal, rbuf_set_dir_internal, dir);
}

bool rbuf_get_speed128(rbuf_t *rbuf, uint addr, byte *speed) {
    return rbuf_get_byte(rbuf, addr, rbuf_get_speed128_internal, speed);
}

bool rbuf_set_speed128(rbuf_t *rbuf, uint addr, byte speed) {
    return rbuf_set_byte(rbuf, addr, rbuf_set_speed128_internal, speed);
}

bool rbuf_get_fct(rbuf_t *rbuf, uint addr, byte no, bool *fct) {
    return rbuf_get_idx_bool(rbuf, addr, rbuf_get_fct_internal, no, fct);
}

bool rbuf_set_fct(rbuf_t *rbuf, uint addr, byte no, bool fct) {
    return rbuf_set_idx_bool(rbuf, addr, rbuf_set_fct_internal, no, fct);
}

bool rbuf_toggle_fct(rbuf_t *rbuf, uint addr, byte no, bool *fct) {
    return rbuf_toggle_idx_bool(rbuf, addr, rbuf_get_fct_internal, rbuf_set_fct_internal, no, fct);
}

static void rbuf_refresh_dir_speed(rbuf_t *rbuf, byte idx, cmdch_in_t *in) { in->dir_speed = rbuf->buf[idx].dir_speed; }
static void rbuf_refresh_f0_4(rbuf_t *rbuf, byte idx, cmdch_in_t *in)      { in->f0_4 = rbuf->buf[idx].f0_4; }
static void rbuf_refresh_f5_8(rbuf_t *rbuf, byte idx, cmdch_in_t *in)      { in->f5_8 = rbuf->buf[idx].f5_68.f5_8; }
static void rbuf_refresh_f9_12(rbuf_t *rbuf, byte idx, cmdch_in_t *in)     { in->f9_12 = rbuf->buf[idx].f5_68.f9_12; }
static void rbuf_refresh_f13_20(rbuf_t *rbuf, byte idx, cmdch_in_t *in)    { in->f13_20 = rbuf->buf[idx].f5_68.f13_20; }
static void rbuf_refresh_f21_28(rbuf_t *rbuf, byte idx, cmdch_in_t *in)    { in->f21_28 = rbuf->buf[idx].f5_68.f21_28; }
static void rbuf_refresh_f29_36(rbuf_t *rbuf, byte idx, cmdch_in_t *in)    { in->f29_36 = rbuf->buf[idx].f5_68.f29_36; }
static void rbuf_refresh_f37_44(rbuf_t *rbuf, byte idx, cmdch_in_t *in)    { in->f37_44 = rbuf->buf[idx].f5_68.f37_44; }
static void rbuf_refresh_f45_52(rbuf_t *rbuf, byte idx, cmdch_in_t *in)    { in->f45_52 = rbuf->buf[idx].f5_68.f45_52; }
static void rbuf_refresh_f53_60(rbuf_t *rbuf, byte idx, cmdch_in_t *in)    { in->f53_60 = rbuf->buf[idx].f5_68.f53_60; }
static void rbuf_refresh_f61_68(rbuf_t *rbuf, byte idx, cmdch_in_t *in)    { in->f61_68 = rbuf->buf[idx].f5_68.f61_68; }

typedef void (*rbuf_refresher)(rbuf_t *rbuf, byte idx, cmdch_in_t *in);
static const rbuf_refresher rbuf_refresh_fn[] = {
    rbuf_refresh_dir_speed,
    rbuf_refresh_f0_4,
    rbuf_refresh_f5_8,
    rbuf_refresh_f9_12,
    rbuf_refresh_f13_20,
    rbuf_refresh_f21_28,
    rbuf_refresh_f29_36,
    rbuf_refresh_f37_44,
    rbuf_refresh_f45_52,
    rbuf_refresh_f53_60,
    rbuf_refresh_f61_68,
};

bool rbuf_refresh(rbuf_t *rbuf, bool *one_entry, cmdch_in_t *in) {
    if (!rbuf_try_lock(rbuf)) return false;

    if (rbuf->next == -1) { // empty
        rbuf_unlock(rbuf);
        return false;
    }

    byte idx = rbuf->next;

    *one_entry = (rbuf->buf[idx].prev == rbuf->buf[idx].next); // only one entry in buffer
        
    in->cmd = rbuf->buf[idx].refresh_cmd;
    in->msb = rbuf->buf[idx].msb;
    in->lsb = rbuf->buf[idx].lsb;
    rbuf_refresh_fn[in->cmd](rbuf, idx, in);

    rbuf->buf[idx].refresh_cmd = (rbuf->buf[idx].refresh_cmd + 1) % (rbuf->buf[idx].max_refresh_cmd + 1);
    rbuf->next = rbuf->buf[idx].next; // progress to next entry
        
    rbuf_unlock(rbuf);
    return true;
}
