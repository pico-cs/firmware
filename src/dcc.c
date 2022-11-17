#include "dcc.h"

static const word WORD_CAP = sizeof(word) * 8;
static const word ONES = 0xffffffff;

static const int DCC_MAX_LOCO_ADDR     = 10239;
static const int DCC_MAX_LOCO_SPEED128 =   127;
static const int DCC_MAX_CV_DIRECT     =  1024;

static const uint DCC_MIN_SYNC_BITS = 17;
static const uint DCC_MAX_SYNC_BITS = 32;

static const uint DCC_DEFAULT_SYNC_BITS = DCC_MIN_SYNC_BITS;

static const int DCC_REPEAT_NON_RBUF = 1; // number of repeats for commands not executed cyclic by refresh buffer 
static const int DCC_REPEAT_WRITE_CV = 2; // command needs to be repeated twice to be accepted by the decoder

static void inline ddc_init_internal(dcc_t *dcc) {
    dcc->bitfield = 0;
    dcc->cap = WORD_CAP;
}

static void inline dcc_put(dcc_t *dcc) {
    dcc->fct(dcc->pio, dcc->sm, dcc->bitfield);
    ddc_init_internal(dcc);
}

static void dcc_send_bits(dcc_t *dcc, word bits, int length) {
    assert(length <= WORD_CAP);          // 0 <= length <= WORD_CAP
    bits &= ONES >> (WORD_CAP - length); // mask bits

    if (dcc->cap >= length) {
        dcc->bitfield = (dcc->bitfield << length) | bits;
        dcc->cap -= length;
        return;
    }

    uint8_t shift = length - dcc->cap;
    uint8_t msb = bits >> shift;         // most sig bits
    dcc->bitfield = (dcc->bitfield << dcc->cap) | msb;
    dcc_put(dcc);
    dcc->bitfield = bits & ~(msb << shift);
    dcc->cap -= shift;
}

static void dcc_send2(dcc_t *dcc, byte b1, byte b2) {
    dcc_send_bits(dcc, ONES, dcc->sync_bits);           // sync bits
    dcc_send_bits(dcc, (word) b1, 9);                   // start bit + data byte 1
    dcc_send_bits(dcc, (word) b2, 9);                   // start bit + data byte 2
    uint8_t cb = b1 ^ b2;                               // check byte
    dcc_send_bits(dcc, (((word) cb) << 1) | 0x1, 10);   // start bit + check byte + stop bit
}

static void dcc_send3(dcc_t *dcc, byte b1, byte b2, byte b3) {
    dcc_send_bits(dcc, ONES, dcc->sync_bits);           // sync bits
    dcc_send_bits(dcc, (word) b1, 9);                   // start bit + data byte 1
    dcc_send_bits(dcc, (word) b2, 9);                   // start bit + data byte 2
    dcc_send_bits(dcc, (word) b3, 9);                   // start bit + data byte 3
    uint8_t cb = b1 ^ b2 ^ b3;                          // check byte
    dcc_send_bits(dcc, (((word) cb) << 1) | 0x1, 10);   // start bit + check byte + stop bit
}

static void dcc_send4(dcc_t *dcc, byte b1, byte b2, byte b3, byte b4) {
    dcc_send_bits(dcc, ONES, dcc->sync_bits);           // sync bits
    dcc_send_bits(dcc, (word) b1, 9);                   // start bit + data byte 1
    dcc_send_bits(dcc, (word) b2, 9);                   // start bit + data byte 2
    dcc_send_bits(dcc, (word) b3, 9);                   // start bit + data byte 3
    dcc_send_bits(dcc, (word) b4, 9);                   // start bit + data byte 4
    uint8_t cb = b1 ^ b2 ^ b3 ^ b4;                     // check byte
    dcc_send_bits(dcc, (((word) cb) << 1) | 0x1, 10);   // start bit + check byte + stop bit
}

static void dcc_send5(dcc_t *dcc, byte b1, byte b2, byte b3, byte b4, byte b5) {
    dcc_send_bits(dcc, ONES, dcc->sync_bits);           // sync bits
    dcc_send_bits(dcc, (word) b1, 9);                   // start bit + data byte 1
    dcc_send_bits(dcc, (word) b2, 9);                   // start bit + data byte 2
    dcc_send_bits(dcc, (word) b3, 9);                   // start bit + data byte 3
    dcc_send_bits(dcc, (word) b4, 9);                   // start bit + data byte 4
    dcc_send_bits(dcc, (word) b5, 9);                   // start bit + data byte 5
    uint8_t cb = b1 ^ b2 ^ b3 ^ b4 ^ b5;                // check byte
    dcc_send_bits(dcc, (((word) cb) << 1) | 0x1, 10);   // start bit + check byte + stop bit
}

static void dcc_send6(dcc_t *dcc, byte b1, byte b2, byte b3, byte b4, byte b5, byte b6) {
    dcc_send_bits(dcc, ONES, dcc->sync_bits);           // sync bits
    dcc_send_bits(dcc, (word) b1, 9);                   // start bit + data byte 1
    dcc_send_bits(dcc, (word) b2, 9);                   // start bit + data byte 2
    dcc_send_bits(dcc, (word) b3, 9);                   // start bit + data byte 3
    dcc_send_bits(dcc, (word) b4, 9);                   // start bit + data byte 4
    dcc_send_bits(dcc, (word) b5, 9);                   // start bit + data byte 5
    dcc_send_bits(dcc, (word) b6, 9);                   // start bit + data byte 6
    uint8_t cb = b1 ^ b2 ^ b3 ^ b4 ^ b5 ^ b6;           // check byte
    dcc_send_bits(dcc, (((word) cb) << 1) | 0x1, 10);   // start bit + check byte + stop bit
}

static void dcc_send7(dcc_t *dcc, byte b1, byte b2, byte b3, byte b4, byte b5, byte b6, byte b7) {
    dcc_send_bits(dcc, ONES, dcc->sync_bits);           // sync bits
    dcc_send_bits(dcc, (word) b1, 9);                   // start bit + data byte 1
    dcc_send_bits(dcc, (word) b2, 9);                   // start bit + data byte 2
    dcc_send_bits(dcc, (word) b3, 9);                   // start bit + data byte 3
    dcc_send_bits(dcc, (word) b4, 9);                   // start bit + data byte 4
    dcc_send_bits(dcc, (word) b5, 9);                   // start bit + data byte 5
    dcc_send_bits(dcc, (word) b6, 9);                   // start bit + data byte 6
    dcc_send_bits(dcc, (word) b7, 9);                   // start bit + data byte 7
    uint8_t cb = b1 ^ b2 ^ b3 ^ b4 ^ b5 ^ b6 ^ b7;      // check byte
    dcc_send_bits(dcc, (((word) cb) << 1) | 0x1, 10);   // start bit + check byte + stop bit
}

static void dcc_send8(dcc_t *dcc, byte b1, byte b2, byte b3, byte b4, byte b5, byte b6, byte b7, byte b8) {
    dcc_send_bits(dcc, ONES, dcc->sync_bits);           // sync bits
    dcc_send_bits(dcc, (word) b1, 9);                   // start bit + data byte 1
    dcc_send_bits(dcc, (word) b2, 9);                   // start bit + data byte 2
    dcc_send_bits(dcc, (word) b3, 9);                   // start bit + data byte 3
    dcc_send_bits(dcc, (word) b4, 9);                   // start bit + data byte 4
    dcc_send_bits(dcc, (word) b5, 9);                   // start bit + data byte 5
    dcc_send_bits(dcc, (word) b6, 9);                   // start bit + data byte 6
    dcc_send_bits(dcc, (word) b7, 9);                   // start bit + data byte 7
    dcc_send_bits(dcc, (word) b8, 9);                   // start bit + data byte 8
    uint8_t cb = b1 ^ b2 ^ b3 ^ b4 ^ b5 ^ b6 ^ b7 ^ b8; // check byte
    dcc_send_bits(dcc, (((word) cb) << 1) | 0x1, 10);   // start bit + check byte + stop bit
}

inline bool dcc_check_loco_addr(uint addr) {
    return ((addr >=1) && (addr <= DCC_MAX_LOCO_ADDR));
};

inline bool dcc_check_loco_speed128(uint speed128) {
    return (speed128 <= DCC_MAX_LOCO_SPEED128);
};

inline bool dcc_check_cv_idx(uint idx) { // directly addressable cv
    return ((idx >=1) && (idx <= DCC_MAX_CV_DIRECT));
};

inline bool dcc_check_cv(byte cv) { // directly addressable cv byte value
    return ((cv >=0) && (cv < 256)); // byte
};

inline bool dcc_check_bit(byte bit) { // directly addressable cv bit position
    return ((bit >=0) && (bit < 8)); // bit position 0-7
};

void dcc_init(dcc_t *dcc, putter fct, PIO pio, uint sm) {
    ddc_init_internal(dcc);
    dcc->fct = fct;
    dcc->pio = pio;
    dcc->sm = sm;
    dcc->sync_bits = DCC_DEFAULT_SYNC_BITS;
}

uint dcc_get_sync_bits(dcc_t *dcc) {
    return dcc->sync_bits;
};

uint dcc_set_sync_bits(dcc_t *dcc, uint sync_bits) {
    if (sync_bits < DCC_MIN_SYNC_BITS) {
        dcc->sync_bits = DCC_MIN_SYNC_BITS;
    } else if (sync_bits > DCC_MAX_SYNC_BITS) {
        dcc->sync_bits = DCC_MAX_SYNC_BITS;
    } else {
        dcc->sync_bits = sync_bits;
    }
    return dcc->sync_bits;
}

void dcc_reset(dcc_t *dcc) {
    dcc_send2(dcc, 0x00, 0x00);
}

void dcc_idle(dcc_t *dcc) {
    dcc_send2(dcc, 0xff, 0x00);
}

void dcc_refresh3(dcc_t *dcc, byte msb, byte lsb, byte dir_speed, byte f0_7) {
    if (msb == 0) {
        dcc_send4(dcc, lsb, 0x3c, dir_speed, f0_7);
    } else {
        dcc_send5(dcc, msb | 0xc0, lsb, 0x3c, dir_speed, f0_7);
    }
}

void dcc_refresh4(dcc_t *dcc, byte msb, byte lsb, byte dir_speed, byte f0_7, byte f8_15) {
    if (msb == 0) {
        dcc_send5(dcc, lsb, 0x3c, dir_speed, f0_7, f8_15);
    } else {
        dcc_send6(dcc, msb | 0xc0, lsb, 0x3c, dir_speed, f0_7, f8_15);
    }
}

void dcc_refresh5(dcc_t *dcc, byte msb, byte lsb, byte dir_speed, byte f0_7, byte f8_15, byte f16_23) {
    if (msb == 0) {
        dcc_send6(dcc, lsb, 0x3c, dir_speed, f0_7, f8_15, f16_23);
    } else {
        dcc_send7(dcc, msb | 0xc0, lsb, 0x3c, dir_speed, f0_7, f8_15, f16_23);
    }
}

void dcc_refresh6(dcc_t *dcc, byte msb, byte lsb, byte dir_speed, byte f0_7, byte f8_15, byte f16_23, byte f24_31) {
    if (msb == 0) {
        dcc_send7(dcc, lsb, 0x3c, dir_speed, f0_7, f8_15, f16_23, f24_31);
    } else {
        dcc_send8(dcc, msb | 0xc0, lsb, 0x3c, dir_speed, f0_7, f8_15, f16_23, f24_31);
    }
}

void dcc_dir_speed(dcc_t *dcc, byte msb, byte lsb, byte dir_speed) {
    if (msb == 0) {
        dcc_send3(dcc, lsb, 0x3f, dir_speed);
    } else {
        dcc_send4(dcc, msb | 0xc0, lsb, 0x3f, dir_speed);
    }
}

void dcc_f0_4(dcc_t *dcc, byte msb, byte lsb, byte f0_4) {
    // comand: 100D-DDDD
    byte cmd = 0x80 | (f0_4 & 0x1f);
    if (msb == 0) {
        dcc_send2(dcc, lsb, cmd);
    } else {
        dcc_send3(dcc, msb | 0xc0, lsb, cmd);
    }
};

void dcc_f5_8(dcc_t *dcc, byte msb, byte lsb, byte f5_8) {
    // comand: 1011-DDDD
    byte cmd = 0xB0 | (f5_8 & 0x0f);
    if (msb == 0) {
        dcc_send2(dcc, lsb, cmd);
    } else {
        dcc_send3(dcc, msb | 0xc0, lsb, cmd);
    }
};

void dcc_f9_12(dcc_t *dcc, byte msb, byte lsb, byte f9_12) {
    // command 1010-DDDD
    byte cmd = 0xA0 | (f9_12 & 0x0f);
    if (msb == 0) {
        dcc_send2(dcc, lsb, cmd);
    } else {
        dcc_send3(dcc, msb | 0xc0, lsb, cmd);
    }
}

void dcc_f13_20(dcc_t *dcc, byte msb, byte lsb, byte f13_20) {
    // command 1101-1110 DDDD-DDDD
    if (msb == 0) {
        dcc_send3(dcc, lsb, 0xde, f13_20);
    } else {
        dcc_send4(dcc, msb | 0xc0, lsb, 0xde, f13_20);
    }
};

void dcc_f21_28(dcc_t *dcc, byte msb, byte lsb, byte f21_28) {
    // command 1101-1111 DDDD-DDDD
    if (msb == 0) {
        dcc_send3(dcc, lsb, 0xdf, f21_28);
    } else {
        dcc_send4(dcc, msb | 0xc0, lsb, 0xdf, f21_28);
    }
}

void dcc_f29_36(dcc_t *dcc, byte msb, byte lsb, byte f29_36) {
    // command 1101-1000 DDDD-DDDD
    if (msb == 0) {
        dcc_send3(dcc, lsb, 0xd8, f29_36);
    } else {
        dcc_send4(dcc, msb | 0xc0, lsb, 0xd8, f29_36);
    }
}

void dcc_f37_44(dcc_t *dcc, byte msb, byte lsb, byte f37_44) {
    // command 1101-1001 DDDD-DDDD
    if (msb == 0) {
        dcc_send3(dcc, lsb, 0xd9, f37_44);
    } else {
        dcc_send4(dcc, msb | 0xc0, lsb, 0xd9, f37_44);
    }
}

void dcc_f45_52(dcc_t *dcc, byte msb, byte lsb, byte f45_52) {
    // command 1101-1010 DDDD-DDDD
    if (msb == 0) {
        dcc_send3(dcc, lsb, 0xda, f45_52);
    } else {
        dcc_send4(dcc, msb | 0xc0, lsb, 0xda, f45_52);
    }
}

void dcc_f53_60(dcc_t *dcc, byte msb, byte lsb, byte f53_60) {
    // command 1101-1011 DDDD-DDDD
    if (msb == 0) {
        dcc_send3(dcc, lsb, 0xdb, f53_60);
    } else {
        dcc_send4(dcc, msb | 0xc0, lsb, 0xdb, f53_60);
    }
}

void dcc_f61_68(dcc_t *dcc, byte msb, byte lsb, byte f61_68) {
    // command  1101-1100 DDDD-DDDD
    if (msb == 0) {
        dcc_send3(dcc, lsb, 0xdc, f61_68);
    } else {
        dcc_send4(dcc, msb | 0xc0, lsb, 0xdc, f61_68);
    }
}

void dcc_cv_byte(dcc_t *dcc, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte val) {
    // operation mode (main track)
    // long config variable command
    //
    // 1110-KKVV VVVV-VVVV DDDD-DDDD
    // KK = 00 – reserved
    // KK = 01 – check byte
    // KK = 11 – write byte
    // KK = 10 – bit mode
    //
    // here: 11 - write byte
    for (int i = 0; i < DCC_REPEAT_WRITE_CV; i++) {
        if (msb == 0) {
            dcc_send4(dcc, lsb, 0xec | (cv_msb & 0x03) , cv_lsb, val);
        } else {
            dcc_send5(dcc, msb | 0xc0, lsb, 0xec | (cv_msb & 0x03), cv_lsb, val);
        }
    }
}

void dcc_cv_bit(dcc_t *dcc, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte bit, bool flag) {
    // operation mode (main track)
    // long config variable command
    //
    // 1110-KKVV VVVV-VVVV DDDD-DDDD
    // KK = 00 – reserved
    // KK = 01 – check byte
    // KK = 11 – write byte
    // KK = 10 – bit mode
    //
    // here: 10 - bit manipulation
    //
    // DDDD-DDDD is 111K-DBBB in bit manipulaton mode
    // K = 1 – write bit
    // K = 0 – check bit
    //
    // here 1 - write bit
    //
    // BBB - bit position (0-7)
    // D   - bit value    (0|1)
    for (int i = 0; i < DCC_REPEAT_WRITE_CV; i++) {
        if (msb == 0) {
            dcc_send4(dcc, lsb, 0xe8 | (cv_msb & 0x03) , cv_lsb, 0xf0 | (flag ? 0x08 : 0x00) | (bit & 0x07));
        } else {
            dcc_send5(dcc, msb | 0xc0, lsb, 0xe8 | (cv_msb & 0x03), cv_lsb, 0xf0 | (flag ? 0x08 : 0x00) | (bit & 0x07));
        }
    }
}

void dcc_cv29_bit5(dcc_t *dcc, byte msb, byte lsb, bool cv29_bit5) {
    // set cv29 bit 5 (switch short / long address)
    // val == false (bit == 0): use short address in cv1
    // val == true  (bit == 1): use logn address in cv17 (msb) and cv18 (lsb)
    //
    // command  0000-101D (d sets bit 5 of cv29)

    for (int i = 0; i < DCC_REPEAT_NON_RBUF; i++) {
        if (msb == 0) {
            dcc_send2(dcc, lsb, cv29_bit5 ? 0x0b : 0x0a);
        } else {
            dcc_send3(dcc, msb | 0xc0, lsb, cv29_bit5 ? 0x0b : 0x0a);
        }
    }
}

void dcc_laddr(dcc_t *dcc, byte msb, byte lsb, byte long_msb, byte long_lsb) {
    // set 'long' address (extended address)
    // sets cv17 + cv18 + cv29 bit 5 (please double check on decoder manual if supported)
    // uses short config variable command
    //
    // 1111-KKKK DDDD-DDDD
    // 1111-KKKK DDDD-DDDD DDDD-DDDD (used here)
    //
    // KKKK = 0000 – must not be used
    // KKKK = 0010 – consist acceleration value (cv23) (only one paket needed for change)
    // KKKK = 0011 – consist delereration value (cv24) (only one paket needed for change)
    // KKKK = 0100 – switch to long / extended address - write cv17, cv18 and set bit 5 in cv29 (used here)
    //               first data byte cv17 (extended address msb), second data byte cv18 (extended address lsb) 
    // KKKK = xxxx - reserved for future usage
    //
    // note: msb extended address range starts with 0x0c (decimal 192) - equivalent to long address addressing schema in DCC commands

    for (int i = 0; i < DCC_REPEAT_WRITE_CV; i++) {
        if (msb == 0) {
            dcc_send4(dcc, lsb, 0xf4, long_msb | 0xc0, long_lsb);
        } else {
            dcc_send5(dcc, msb | 0xc0, lsb, 0xf4, long_msb | 0xc0, long_lsb);
        }
    }
}
