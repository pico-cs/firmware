#ifndef _DCC_TX_H
#define _DCC_TX_H

#include "hardware/pio.h"

#include "common.h"

typedef struct dcc_tx_sm dcc_tx_sm_t;

typedef void (*bytes_sender)(dcc_tx_sm_t *dcc_tx_sm, byte b[], byte num_byte);

struct dcc_tx_sm {
    PIO pio;
    uint sm;

    bytes_sender send_bytes;    // sender function

    word tx_data;               // data word to write to state machine's TX FIFO 
    byte tx_num_free_bit;       // number of free tx_data bits

    byte *cv_ptr;               // pointer to config byte array
};

typedef struct {
    dcc_tx_sm_t tx_sm_def;
    dcc_tx_sm_t tx_sm_bidi;
} dcc_tx_pio_t;

void dcc_tx_pio_init(dcc_tx_pio_t *tx_pio);

void dcc_tx_sm_set_enabled(dcc_tx_sm_t *tx_sm, bool enabled, byte cv_ptr[]);

void dcc_tx_sm_cmd_reset(dcc_tx_sm_t *tx_sm);
void dcc_tx_sm_cmd_idle(dcc_tx_sm_t *tx_sm);
void dcc_tx_sm_cmd_refresh3(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte dir_speed, byte f0_7);
void dcc_tx_sm_cmd_refresh4(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte dir_speed, byte f0_7, byte f8_15);
void dcc_tx_sm_cmd_refresh5(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte dir_speed, byte f0_7, byte f8_15, byte f16_23);
void dcc_tx_sm_cmd_refresh6(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte dir_speed, byte f0_7, byte f8_15, byte f16_23, byte f24_31);
void dcc_tx_sm_cmd_dir_speed(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte dir_speed);
void dcc_tx_sm_cmd_f0_4(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f0_4);
void dcc_tx_sm_cmd_f5_8(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f5_8);
void dcc_tx_sm_cmd_f9_12(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f9_12);
void dcc_tx_sm_cmd_f13_20(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f13_20);
void dcc_tx_sm_cmd_f21_28(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f21_28);
void dcc_tx_sm_cmd_f29_36(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f29_36);
void dcc_tx_sm_cmd_f37_44(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f37_44);
void dcc_tx_sm_cmd_f45_52(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f45_52);
void dcc_tx_sm_cmd_f53_60(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f53_60);
void dcc_tx_sm_cmd_f61_68(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte f61_68);
void dcc_tx_sm_cmd_cv_byte(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte val);
void dcc_tx_sm_cmd_cv_bit(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte cv_bit, bool cv_flag);
void dcc_tx_sm_cmd_cv29_bit5(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, bool cv29_bit5);
void dcc_tx_sm_cmd_laddr(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte new_msb, byte new_lsb);
void dcc_tx_sm_cmd_acc(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte acc_out, bool acc_flag);
void dcc_tx_sm_cmd_acc_ext(dcc_tx_sm_t *tx_sm, byte msb, byte lsb, byte acc_status);

#endif