#include "pico/binary_info.h"
#include "hardware/clocks.h"

#include "dcc_tx.pio.h"

#include "dcc_tx.h"
#include "cfg.h"

static const word NUM_WORD_BIT = sizeof(word) * 8;
static const word ONES = 0xffffffff;

// main track: using three pins
// - first:  DCC signal
// - second: DCC signal inverted
// - third:  Power on / off (+BiDi cutout)
// - fourth: Power on / off (+BiDi cutout) inverted
static const uint DCC_TX_MT_DCC_PIN      =  2;
static const uint DCC_TX_MT_DCC_INV_PIN  =  DCC_TX_MT_DCC_PIN + 1;
static const uint DCC_TX_MT_PWR_PIN      =  DCC_TX_MT_DCC_PIN + 2;
static const uint DCC_TX_MT_PWR_INV_PIN  =  DCC_TX_MT_DCC_PIN + 3;
static const uint DCC_TX_MT_NUM_PIN      =  4;

static inline void dcc_tx_sm_send_bits(dcc_tx_sm_t *tx_sm, word bits, byte num_bit) {
    if (tx_sm->tx_num_free_bit >= num_bit) {
        tx_sm->tx_data = (tx_sm->tx_data << num_bit) | bits;
        tx_sm->tx_num_free_bit -= num_bit;
        return;
    }

    byte shift = num_bit - tx_sm->tx_num_free_bit;
    word msb = (bits >> shift); // most sig bits
    tx_sm->tx_data = (tx_sm->tx_data << tx_sm->tx_num_free_bit) | msb;

    pio_sm_put_blocking(tx_sm->pio, tx_sm->sm, tx_sm->tx_data);
    tx_sm->tx_data = bits;
    tx_sm->tx_num_free_bit = NUM_WORD_BIT - shift;
}

static inline void dcc_tx_sm_flush_bits(dcc_tx_sm_t *tx_sm) {
    if (tx_sm->tx_num_free_bit == NUM_WORD_BIT) return; // nothing to flush

    tx_sm->tx_data <<= tx_sm->tx_num_free_bit;
    pio_sm_put_blocking(tx_sm->pio, tx_sm->sm, tx_sm->tx_data);
    tx_sm->tx_num_free_bit = NUM_WORD_BIT;
}

static inline void dcc_tx_sm_send_def_bytes(dcc_tx_sm_t *tx_sm, byte b[], byte num_byte) {
    byte num_sync_bit = cfg_get_cv(CFG_MT_NUM_SYNC_BIT);
    dcc_tx_sm_send_bits(tx_sm, ~(ONES << num_sync_bit), num_sync_bit);    // sync bits
    byte cb = 0;                                                          // check byte
    for (byte i = 0; i < num_byte; i++) {
        dcc_tx_sm_send_bits(tx_sm, (word) b[i], 9);                       // start bit + data byte
        cb ^= b[i];                                                       // xor check byte
    }
    dcc_tx_sm_send_bits(tx_sm, (((word) cb) << 1) | 0x1, 10);             // start bit + check byte + stop bit
}

static inline void dcc_tx_sm_send_bidi_bytes(dcc_tx_sm_t *tx_sm, byte b[], byte num_byte) {
    /*
        state machine generates the first 4 sync bits to support BiDi cutout, so
        we can reduce the number of sync bits by 4
    */
    byte num_sync_bit = cfg_get_cv(CFG_MT_NUM_SYNC_BIT) - 4;
    /*
        send number of dcc bits to state machine 'framing' messages
        -  number of sync bits                              - max:          +32 bits
        -  number of data bytes * (start bit + 8 data bits) - max: 8 * 9 =  +72 bits
        -  start bit + 8 check byte bits + end bit          - fix:          +10 bits
        -  subtract 1 as state machine loop executes n+1    - fix:          - 1
        -                                                   - sum:          113 bits (can be encoded in 7 bits)
    */
    word num_bit = num_sync_bit + num_byte * 9 + 10 - 1;
    dcc_tx_sm_send_bits(tx_sm, num_bit, 9);

    /*
        send cutout times
        - cutout period in total 464us (4 * sync bit (1 bit) time)
        - ts: time to start cutout (26 - 32us)
        - te: time to end   cutout (10 - 16us) before 5th sync bit starts
        - tc: time of effective cutout duration
        --> ts + cd + te = 464us
        - cs := ts - 2
        - ce := te - 10

        |   ts   |       tc      |   te  |
        |26-32us |   416-428us   |10-16us|
    */
    word cs = cfg_get_cv(CFG_MT_BIDI_TS) - 26;
    word ce = cfg_get_cv(CFG_MT_BIDI_TE) - 10;

    dcc_tx_sm_send_bits(tx_sm, cs, 3);
    dcc_tx_sm_send_bits(tx_sm, (428 - 3) - (cs + ce), 9);               // 3 instructions used by program
    dcc_tx_sm_send_bits(tx_sm, ce, 3);
    
    /*
        start sending sync bits + message
    */
    dcc_tx_sm_send_bits(tx_sm, ~(ONES << num_sync_bit), num_sync_bit);  // sync bits
    byte cb = 0;                                                        // check byte
    for (byte i = 0; i < num_byte; i++) {
        dcc_tx_sm_send_bits(tx_sm, (word) b[i], 9);                     // start bit + data byte
        cb ^= b[i];                                                     // xor check byte
    }
    dcc_tx_sm_send_bits(tx_sm, (((word) cb) << 1) | 0x1, 10);           // start bit + check byte + stop bit
    dcc_tx_sm_flush_bits(tx_sm);                                        // end frame 
}

inline static void dcc_tx_sm_send2(dcc_tx_sm_t *tx_sm, byte b1, byte b2) {
    byte b[] = {b1, b2};
    tx_sm->send_bytes(tx_sm, b, 2);
}

inline static void dcc_tx_sm_send3(dcc_tx_sm_t *tx_sm, byte b1, byte b2, byte b3) {
    byte b[] = {b1, b2, b3};
    tx_sm->send_bytes(tx_sm, b, 3);
}

inline static void dcc_tx_sm_send4(dcc_tx_sm_t *tx_sm, byte b1, byte b2, byte b3, byte b4) {
    byte b[] = {b1, b2, b3, b4};
    tx_sm->send_bytes(tx_sm, b, 4);
}

inline static void dcc_tx_sm_send5(dcc_tx_sm_t *tx_sm, byte b1, byte b2, byte b3, byte b4, byte b5) {
    byte b[] = {b1, b2, b3, b4, b5};
    tx_sm->send_bytes(tx_sm, b, 5);
}

inline static void dcc_tx_sm_send6(dcc_tx_sm_t *tx_sm, byte b1, byte b2, byte b3, byte b4, byte b5, byte b6) {
    byte b[] = {b1, b2, b3, b4, b5, b6};
    tx_sm->send_bytes(tx_sm, b, 6);
}

inline static void dcc_tx_sm_send7(dcc_tx_sm_t *tx_sm, byte b1, byte b2, byte b3, byte b4, byte b5, byte b6, byte b7) {
    byte b[] = {b1, b2, b3, b4, b5, b6, b7};
    tx_sm->send_bytes(tx_sm, b, 7);
}

inline static void dcc_tx_sm_send8(dcc_tx_sm_t *tx_sm, byte b1, byte b2, byte b3, byte b4, byte b5, byte b6, byte b7, byte b8) {
    byte b[] = {b1, b2, b3, b4, b5, b6, b7, b8};
    tx_sm->send_bytes(tx_sm, b, 8);
}

static void dcc_tx_sm_init(dcc_tx_sm_t *tx_sm, PIO pio, uint sm, uint offset, pio_sm_config *config, uint base_pin, uint num_pin, uint64_t freq, bytes_sender send_bytes) {
    tx_sm->pio = pio;
    tx_sm->sm  = sm;

    tx_sm->send_bytes = send_bytes;
          
    tx_sm->tx_data         = 0;
    tx_sm->tx_num_free_bit = NUM_WORD_BIT;

    pio_sm_set_consecutive_pindirs(pio, sm, base_pin, num_pin, true);
        
    // OUT shifts to left (msb first), autopull, threshold 32 bits 
    sm_config_set_out_shift(config, false, true, 32);
    // SET pins
    sm_config_set_set_pins(config, base_pin, num_pin);
    
    // No RX, so join RX fifo to TX fifo (in total 8 words)
    sm_config_set_fifo_join(config, PIO_FIFO_JOIN_TX);

    // Set clock divider
    uint64_t div = ((uint64_t)clock_get_hz(clk_sys) << 8) / freq;
    uint16_t div_int  = div >> 8;
    uint8_t  div_frac = div & 0xff;
    sm_config_set_clkdiv_int_frac(config, div_int, div_frac);
    
    // Finally init state machine 
    pio_sm_init(pio, sm, offset, config);
}

void dcc_tx_sm_set_enabled(dcc_tx_sm_t *tx_sm, bool enabled) {
    if (enabled) {
        pio_sm_set_enabled(tx_sm->pio, tx_sm->sm, true);            // enable state machine
        uint instr = pio_encode_set(pio_pins, dcc_tx_high);         // 
        pio_sm_exec(tx_sm->pio, tx_sm->sm, instr);                  // send instruction to set pins high
    } else {
        uint instr = pio_encode_set(pio_pins, dcc_tx_off);          // 
        pio_sm_exec(tx_sm->pio, tx_sm->sm, instr);                  // send instruction to set pins off
        pio_sm_set_enabled(tx_sm->pio, tx_sm->sm, false);           // disable state machine
    }
}

void dcc_tx_sm_cmd_reset(dcc_tx_sm_t *tx_sm) {
    dcc_tx_sm_send2(tx_sm, 0x00, 0x00);
}

void dcc_tx_sm_cmd_idle(dcc_tx_sm_t *tx_sm) {
    dcc_tx_sm_send2(tx_sm, 0xff, 0x00);
}

void dcc_tx_sm_cmd_refresh3(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte dir_speed, byte f0_7) {
    if (msb == 0) {
        dcc_tx_sm_send4(tx_sm, lsb, 0x3c, dir_speed, f0_7);
    } else {
        dcc_tx_sm_send5(tx_sm, msb | 0xc0, lsb, 0x3c, dir_speed, f0_7);
    }
}

void dcc_tx_sm_cmd_refresh4(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte dir_speed, byte f0_7, byte f8_15) {
    if (msb == 0) {
        dcc_tx_sm_send5(tx_sm, lsb, 0x3c, dir_speed, f0_7, f8_15);
    } else {
        dcc_tx_sm_send6(tx_sm, msb | 0xc0, lsb, 0x3c, dir_speed, f0_7, f8_15);
    }
}

void dcc_tx_sm_cmd_refresh5(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte dir_speed, byte f0_7, byte f8_15, byte f16_23) {
    if (msb == 0) {
        dcc_tx_sm_send6(tx_sm, lsb, 0x3c, dir_speed, f0_7, f8_15, f16_23);
    } else {
        dcc_tx_sm_send7(tx_sm, msb | 0xc0, lsb, 0x3c, dir_speed, f0_7, f8_15, f16_23);
    }
}

void dcc_tx_sm_cmd_refresh6(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte dir_speed, byte f0_7, byte f8_15, byte f16_23, byte f24_31) {
    if (msb == 0) {
        dcc_tx_sm_send7(tx_sm, lsb, 0x3c, dir_speed, f0_7, f8_15, f16_23, f24_31);
    } else {
        dcc_tx_sm_send8(tx_sm, msb | 0xc0, lsb, 0x3c, dir_speed, f0_7, f8_15, f16_23, f24_31);
    }
}

void dcc_tx_sm_cmd_dir_speed(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte dir_speed) {
    if (msb == 0) {
        dcc_tx_sm_send3(tx_sm, lsb, 0x3f, dir_speed);
    } else {
        dcc_tx_sm_send4(tx_sm, msb | 0xc0, lsb, 0x3f, dir_speed);
    }
}

void dcc_tx_sm_cmd_f0_4(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f0_4) {
    // comand: 100D-DDDD
    byte cmd = 0x80 | (f0_4 & 0x1f);
    if (msb == 0) {
        dcc_tx_sm_send2(tx_sm, lsb, cmd);
    } else {
        dcc_tx_sm_send3(tx_sm, msb | 0xc0, lsb, cmd);
    }
};

void dcc_tx_sm_cmd_f5_8(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f5_8) {
    // comand: 1011-DDDD
    byte cmd = 0xB0 | (f5_8 & 0x0f);
    if (msb == 0) {
        dcc_tx_sm_send2(tx_sm, lsb, cmd);
    } else {
        dcc_tx_sm_send3(tx_sm, msb | 0xc0, lsb, cmd);
    }
};

void dcc_tx_sm_cmd_f9_12(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f9_12) {
    // command 1010-DDDD
    byte cmd = 0xA0 | (f9_12 & 0x0f);
    if (msb == 0) {
        dcc_tx_sm_send2(tx_sm, lsb, cmd);
    } else {
        dcc_tx_sm_send3(tx_sm, msb | 0xc0, lsb, cmd);
    }
}

void dcc_tx_sm_cmd_f13_20(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f13_20) {
    // command 1101-1110 DDDD-DDDD
    if (msb == 0) {
        dcc_tx_sm_send3(tx_sm, lsb, 0xde, f13_20);
    } else {
        dcc_tx_sm_send4(tx_sm, msb | 0xc0, lsb, 0xde, f13_20);
    }
};

void dcc_tx_sm_cmd_f21_28(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f21_28) {
    // command 1101-1111 DDDD-DDDD
    if (msb == 0) {
        dcc_tx_sm_send3(tx_sm, lsb, 0xdf, f21_28);
    } else {
        dcc_tx_sm_send4(tx_sm, msb | 0xc0, lsb, 0xdf, f21_28);
    }
}

void dcc_tx_sm_cmd_f29_36(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f29_36) {
    // command 1101-1000 DDDD-DDDD
    if (msb == 0) {
        dcc_tx_sm_send3(tx_sm, lsb, 0xd8, f29_36);
    } else {
        dcc_tx_sm_send4(tx_sm, msb | 0xc0, lsb, 0xd8, f29_36);
    }
}

void dcc_tx_sm_cmd_f37_44(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f37_44) {
    // command 1101-1001 DDDD-DDDD
    if (msb == 0) {
        dcc_tx_sm_send3(tx_sm, lsb, 0xd9, f37_44);
    } else {
        dcc_tx_sm_send4(tx_sm, msb | 0xc0, lsb, 0xd9, f37_44);
    }
}

void dcc_tx_sm_cmd_f45_52(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f45_52) {
    // command 1101-1010 DDDD-DDDD
    if (msb == 0) {
        dcc_tx_sm_send3(tx_sm, lsb, 0xda, f45_52);
    } else {
        dcc_tx_sm_send4(tx_sm, msb | 0xc0, lsb, 0xda, f45_52);
    }
}

void dcc_tx_sm_cmd_f53_60(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f53_60) {
    // command 1101-1011 DDDD-DDDD
    if (msb == 0) {
        dcc_tx_sm_send3(tx_sm, lsb, 0xdb, f53_60);
    } else {
        dcc_tx_sm_send4(tx_sm, msb | 0xc0, lsb, 0xdb, f53_60);
    }
}

void dcc_tx_sm_cmd_f61_68(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f61_68) {
    // command 1101-1100 DDDD-DDDD
    if (msb == 0) {
        dcc_tx_sm_send3(tx_sm, lsb, 0xdc, f61_68);
    } else {
        dcc_tx_sm_send4(tx_sm, msb | 0xc0, lsb, 0xdc, f61_68);
    }
}

void dcc_tx_sm_cmd_cv_byte(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte val) {
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
    if (msb == 0) {
        dcc_tx_sm_send4(tx_sm, lsb, 0xec | (cv_msb & 0x03) , cv_lsb, val);
    } else {
        dcc_tx_sm_send5(tx_sm, msb | 0xc0, lsb, 0xec | (cv_msb & 0x03), cv_lsb, val);
    }
}

void dcc_tx_sm_cmd_cv_bit(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte cv_bit, bool cv_flag) {
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
    if (msb == 0) {
        dcc_tx_sm_send4(tx_sm, lsb, 0xe8 | (cv_msb & 0x03) , cv_lsb, 0xf0 | (cv_flag ? 0x08 : 0x00) | (cv_bit & 0x07));
    } else {
        dcc_tx_sm_send5(tx_sm, msb | 0xc0, lsb, 0xe8 | (cv_msb & 0x03), cv_lsb, 0xf0 | (cv_flag ? 0x08 : 0x00) | (cv_bit & 0x07));
    }
}

void dcc_tx_sm_cmd_cv29_bit5(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, bool cv29_bit5) {
    // set cv29 bit 5 (switch short / long address)
    // val == false (bit == 0): use short address in cv1
    // val == true  (bit == 1): use long address in cv17 (msb) and cv18 (lsb)
    //
    // command  0000-101D (d sets bit 5 of cv29)
    if (msb == 0) {
        dcc_tx_sm_send2(tx_sm, lsb, cv29_bit5 ? 0x0b : 0x0a);
    } else {
        dcc_tx_sm_send3(tx_sm, msb | 0xc0, lsb, cv29_bit5 ? 0x0b : 0x0a);
    }
}

void dcc_tx_sm_cmd_laddr(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte long_msb, byte long_lsb) {
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
    if (msb == 0) {
        dcc_tx_sm_send4(tx_sm, lsb, 0xf4, long_msb | 0xc0, long_lsb);
    } else {
        dcc_tx_sm_send5(tx_sm, msb | 0xc0, lsb, 0xf4, long_msb | 0xc0, long_lsb);
    }
}

void dcc_tx_sm_cmd_acc(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte acc_out, bool acc_flag) {
    // command 10AA-AAAA 1AAA-CAAD
    // C: flag on / off
    // D: output (0 or 1)
    // 11-bit address:
    //   1 0 A A - A A A A  1 A A A - C A A D
    //       7 6   5 4 3 2   10 9 8     1 0
    // with A8, A9, A10 inverted by convention
    dcc_tx_sm_send2(
        tx_sm,
        0x80 | (lsb >> 2),
        0x80 | (acc_flag ? 0x08 : 0x00) | (((msb & 0x07) ^ 0x07) << 4) | ((lsb & 0x03) << 1) | (acc_out & 0x01)
    );
}

void dcc_tx_sm_cmd_acc_ext(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte acc_status) {
    // command 10AA-AAAA 0AAA-0AA1 DDDD-DDDD
    // 11-bit address:
    //   1 0 A A - A A A A  0 A A A - 0 A A 1
    //       7 6   5 4 3 2   10 9 8     1 0
    // with A8, A9, A10 inverted by convention
    dcc_tx_sm_send3(
        tx_sm,
        0x80 | (lsb >> 2),
        0x80 | (((msb & 0x07) ^ 0x07) << 4) | ((lsb & 0x03) << 1) | 0x01,
        acc_status
    );
}

void dcc_tx_pio_init(dcc_tx_pio_t *tx_pio) {
    bi_decl(bi_1pin_with_name(DCC_TX_MT_DCC_PIN,     "Main track: DCC signal output"));
    bi_decl(bi_1pin_with_name(DCC_TX_MT_DCC_INV_PIN, "Main track: DCC signal output (inverted)"));
    bi_decl(bi_1pin_with_name(DCC_TX_MT_PWR_PIN,     "Main track: DCC signal power"));
    bi_decl(bi_1pin_with_name(DCC_TX_MT_PWR_INV_PIN, "Main track: DCC signal power  (inverted)"));

    // init gpio 
    pio_gpio_init(pio0, DCC_TX_MT_DCC_PIN);
    pio_gpio_init(pio0, DCC_TX_MT_DCC_INV_PIN);
    pio_gpio_init(pio0, DCC_TX_MT_PWR_PIN);
    pio_gpio_init(pio0, DCC_TX_MT_PWR_INV_PIN);

    uint tx_def_offset  = pio_add_program(pio0, &dcc_tx_def_program);
    uint tx_bidi_offset = pio_add_program(pio0, &dcc_tx_bidi_program);

    pio_sm_config tx_def_config  = dcc_tx_def_program_get_default_config(tx_def_offset);
    pio_sm_config tx_bidi_config = dcc_tx_bidi_program_get_default_config(tx_bidi_offset);

    dcc_tx_sm_init(&tx_pio->tx_sm_def,
        pio0,
        0,
        tx_def_offset,
        &tx_def_config,
        DCC_TX_MT_DCC_PIN,
        DCC_TX_MT_NUM_PIN,
        500000, // state machine frequency: 500 000 HZ (2us per instruction)
        dcc_tx_sm_send_def_bytes
    );

    dcc_tx_sm_init(&tx_pio->tx_sm_bidi,
        pio0,
        1,
        tx_bidi_offset,
        &tx_bidi_config,
        DCC_TX_MT_DCC_PIN,
        DCC_TX_MT_NUM_PIN,
        1000000, // state machine frequency: 1 000 000 HZ (1us per instruction)
        dcc_tx_sm_send_bidi_bytes
    );
}
