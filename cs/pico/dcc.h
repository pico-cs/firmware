#ifndef _DCC_H
#define _DCC_H

#include "pico/stdlib.h"
#include "hardware/pio.h"

#include "common.h"

typedef void (*putter)(PIO pio, uint sm, word w);

typedef struct {
    putter fct;
    word bitfield;
    byte cap; //TODO int?
    PIO pio;
    uint sm; 
    uint sync_bits;
} dcc_t;

// public interface
bool dcc_check_loco_addr(uint addr);
bool dcc_check_loco_speed128(uint speed128);
//bool dcc_check_loco_laddr(uint addr);
bool dcc_check_cv_idx(uint idx);
bool dcc_check_cv(byte cv);
bool dcc_check_bit(byte bit); 

void dcc_init(dcc_t *dcc, putter fct, PIO pio, uint sm);
uint dcc_set_sync_bits(dcc_t *dcc, uint sync_bits);
uint dcc_get_sync_bits(dcc_t *dcc);
void dcc_reset(dcc_t *dcc);
void dcc_idle(dcc_t *dcc);
void dcc_refresh3(dcc_t *dcc, byte msb, byte lsb, byte dir_speed, byte f0_7);
void dcc_refresh4(dcc_t *dcc, byte msb, byte lsb, byte dir_speed, byte f0_7, byte f8_15);
void dcc_refresh5(dcc_t *dcc, byte msb, byte lsb, byte dir_speed, byte f0_7, byte f8_15, byte f16_23);
void dcc_refresh6(dcc_t *dcc, byte msb, byte lsb, byte dir_speed, byte f0_7, byte f8_15, byte f16_23, byte f24_31);
void dcc_dir_speed(dcc_t *dcc, byte msb, byte lsb, byte dir_speed);
void dcc_f0_4(dcc_t *dcc, byte msb, byte lsb, byte f0_4);
void dcc_f5_8(dcc_t *dcc, byte msb, byte lsb, byte f5_8);
void dcc_f9_12(dcc_t *dcc, byte msb, byte lsb, byte f9_12);
void dcc_f13_20(dcc_t *dcc, byte msb, byte lsb, byte f13_20);
void dcc_f21_28(dcc_t *dcc, byte msb, byte lsb, byte f21_28);
void dcc_f29_36(dcc_t *dcc, byte msb, byte lsb, byte f29_36);
void dcc_f37_44(dcc_t *dcc, byte msb, byte lsb, byte f37_44);
void dcc_f45_52(dcc_t *dcc, byte msb, byte lsb, byte f45_52);
void dcc_f53_60(dcc_t *dcc, byte msb, byte lsb, byte f53_60);
void dcc_f61_68(dcc_t *dcc, byte msb, byte lsb, byte f61_68);
void dcc_cv_byte(dcc_t *dcc, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte val);
void dcc_cv_bit(dcc_t *dcc, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte bit, bool flag);
void dcc_cv29_bit5(dcc_t *dcc, byte msb, byte lsb, bool cv29_bit5);
void dcc_laddr(dcc_t *dcc, byte msb, byte lsb, byte new_msb, byte new_lsb);

#endif
