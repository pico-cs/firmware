#include <stdio.h>

#include "rbuf.h"
#include "exe.h"

enum {
    RBUF_ERROR_NONE        =  0,
    RBUF_ERROR_NOT_FOUND   = -1,
    RBUF_ERROR_BUFFER_FULL = -2,
};

// some decoders cannot handle extended function formats, so
// - the default is set to restrict the refresh cycle to functions 0-12
// - if a function > 12 is selected, the refresh cycle is extended
enum {
    NUM_REFRESH_CYCLE_F0_12   =  4, // dir_speed, f0_4, f5_8, f9_12
    NUM_REFRESH_CYCLE_F13_20  =  5,
    NUM_REFRESH_CYCLE_F21_28  =  6,
    NUM_REFRESH_CYCLE_F29_36  =  7,
    NUM_REFRESH_CYCLE_F37_44  =  8,
    NUM_REFRESH_CYCLE_F45_52  =  9,
    NUM_REFRESH_CYCLE_F53_60  = 10,
    NUM_REFRESH_CYCLE_F61_68  = 11,
};

static const byte REFRESH_CYCLES[7] = {
    NUM_REFRESH_CYCLE_F13_20,
    NUM_REFRESH_CYCLE_F21_28, 
    NUM_REFRESH_CYCLE_F29_36, 
    NUM_REFRESH_CYCLE_F37_44, 
    NUM_REFRESH_CYCLE_F45_52,
    NUM_REFRESH_CYCLE_F53_60,
    NUM_REFRESH_CYCLE_F61_68,
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
    entry->num_refresh_cycle = NUM_REFRESH_CYCLE_F0_12;
    entry->refresh_cycle = 0;
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

static inline void rbuf_entry_copy(volatile rbuf_entry_t *dest, const volatile rbuf_entry_t *source) {
    dest->msb = source->msb;
    dest->lsb = source->lsb;
    dest->num_refresh_cycle = source->num_refresh_cycle;
    dest->refresh_cycle = source->refresh_cycle;
    dest->dir_speed = source->dir_speed;
    dest->f0_4 = source->f0_4;
    dest->f5_68.f5_68 = source->f5_68.f5_68;
    dest->prev = source->prev;
    dest->next = source->next;
}

// rbuf

static inline bool rbuf_try_lock(rbuf_t *rbuf) {
    //return true;
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

static inline void rbuf_clear_entry(rbuf_t *rbuf, uint idx) {
    rbuf_entry_clear(&rbuf->buf[idx]);
}

static inline void rbuf_clear_entry_attr(rbuf_t *rbuf, uint idx) {
    rbuf_entry_clear_attr(&rbuf->buf[idx]);
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
    channel_dir_speed(rbuf->channel, rbuf->buf[idx].msb, rbuf->buf[idx].lsb, rbuf->buf[idx].dir_speed);
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
    channel_dir_speed(rbuf->channel, rbuf->buf[idx].msb, rbuf->buf[idx].lsb, rbuf->buf[idx].dir_speed);
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
    // update num_refresh_cycle
    if (no >= 13) {
        byte i = (no - 13) / 8;
        if (rbuf->buf[idx].num_refresh_cycle < REFRESH_CYCLES[i]) rbuf->buf[idx].num_refresh_cycle = REFRESH_CYCLES[i];
    }
        
    if (rbuf_get_fct_internal(rbuf, idx, no) == v) {
        return false; // not changed
    }

    byte msb = rbuf->buf[idx].msb;
    byte lsb = rbuf->buf[idx].lsb;
    byte i;

    if (no == 0) {
        byte f0_4 = rbuf->buf[idx].f0_4;
        f0_4 = v ? f0_4 | 0x10 : f0_4 & ~0x10;
        rbuf->buf[idx].f0_4 = f0_4;
        channel_f0_4(rbuf->channel, msb, lsb, f0_4);
    } else if (no < 5) {
        i = no - 1; // zero based
        byte f0_4 = rbuf->buf[idx].f0_4;
        f0_4 = v ? f0_4 | (ONE_U8 << i) : f0_4 & ~(ONE_U8 << i);
        rbuf->buf[idx].f0_4 = f0_4;
        channel_f0_4(rbuf->channel, msb, lsb, f0_4);
    } else {
        i = no - 5; // zero based
        f5_68_t f5_68;
        f5_68.f5_68 = rbuf->buf[idx].f5_68.f5_68;
        f5_68.f5_68 = v ? f5_68.f5_68 | (ONE_U64 << i) : f5_68.f5_68 & ~(ONE_U64 << i);
        rbuf->buf[idx].f5_68.f5_68 = f5_68.f5_68;

        switch (i / 8) {
        case 0:
            if ((i & 8) <= 4) {
                channel_f5_8(rbuf->channel, msb, lsb, f5_68.f5_8);
            } else {
                channel_f9_12(rbuf->channel, msb, lsb, f5_68.f9_12);
            }
            break;
        case 1: channel_f13_20(rbuf->channel, msb, lsb, f5_68.f13_20); break;
        case 2: channel_f21_28(rbuf->channel, msb, lsb, f5_68.f21_28); break;
        case 3: channel_f29_36(rbuf->channel, msb, lsb, f5_68.f29_36); break;
        case 4: channel_f37_44(rbuf->channel, msb, lsb, f5_68.f37_44); break;
        case 5: channel_f45_52(rbuf->channel, msb, lsb, f5_68.f45_52); break;
        case 6: channel_f53_60(rbuf->channel, msb, lsb, f5_68.f53_60); break;
        case 7: channel_f61_68(rbuf->channel, msb, lsb, f5_68.f61_68); break;
        }
    }
    return true;
}

static int rbuf_get_index(rbuf_t *rbuf, uint addr, uint *idx) {
    for (int i = 0; i < RBUF_NUM_ENTRY; i++) {
        *idx = (addr+i) % RBUF_NUM_ENTRY;
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
    if (clear) rbuf_clear_entry_attr(rbuf, idx);
}

static void rbuf_remove_loco(rbuf_t *rbuf, uint idx) {
    int prev = rbuf->buf[idx].prev;
    int next = rbuf->buf[idx].next;

    if (prev == next) { // only one entry
        rbuf->first = -1;
        rbuf->next  = -1;
    } else {
        rbuf->buf[prev].next = next;
        rbuf->buf[next].prev = prev;
        rbuf->first = next;
        if (idx == rbuf->next) rbuf->next = next;
    }
    rbuf_clear_entry(rbuf, idx);
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

void rbuf_init(rbuf_t *rbuf, channel_t *channel) {
    mutex_init(&(rbuf->mu));
    rbuf->channel = channel;
    rbuf->first = -1;
    rbuf->next = -1;
    for (int i = 0; i < RBUF_NUM_ENTRY; i++) {
        rbuf_entry_clear(&rbuf->buf[i]);
    }
}

bool rbuf_deregister(rbuf_t *rbuf, uint addr) {
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

bool rbuf_try_get_next(rbuf_t *rbuf, rbuf_entry_t *entry) {
    if (!rbuf_try_lock(rbuf)) return false;

    if (rbuf->next == -1) { // empty
        rbuf_unlock(rbuf);
        return false;
    }

    byte idx = rbuf->next;
    rbuf_entry_copy(entry, &rbuf->buf[idx]);
    rbuf->buf[idx].refresh_cycle = (rbuf->buf[idx].refresh_cycle + 1) % rbuf->buf[idx].num_refresh_cycle;
    rbuf->next = rbuf->buf[idx].next; // progress to next entry
        
    rbuf_unlock(rbuf);
    return true;
}
