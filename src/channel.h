#ifndef _CHANNEL_H
#define _CHANNEL_H

#include "pico/util/queue.h"

#include "common.h"

// config
#define CFGCH_NUM_CMD 4

static const byte CFGCH_GET_CV      = 0;
static const byte CFGCH_SET_CV      = 1;
static const byte CFGCH_GET_ENABLED = 2;
static const byte CFGCH_SET_ENABLED = 3;

typedef struct {
    byte cmd;
    union {
        bool enabled;
        struct {
            byte cv_idx;
            byte cv;
        };
    };
} cfgch_in_t;

typedef struct {
    union {
        bool enabled;
        byte cv;
    };
} cfgch_out_t;

typedef struct {
    queue_t qin;
    queue_t qout;
} cfgch_t;

void cfgch_init(cfgch_t *cfgch);

byte cfgch_get_cv(cfgch_t *cfgch, byte idx);
byte cfgch_set_cv(cfgch_t *cfgch, byte idx, byte cv);
bool cfgch_get_enabled(cfgch_t *cfgch);
bool cfgch_set_enabled(cfgch_t *cfgch, bool enabled);

bool cfgch_try_remove_qin(cfgch_t *cfgch, cfgch_in_t *in);
void cfgch_add_blocking_qout(cfgch_t *cfgch, cfgch_out_t *out);

// command
#define CMDCH_NUM_CMD 17

static const byte CMDCH_DIR_SPEED =  0;
static const byte CMDCH_F0_4      =  1;
static const byte CMDCH_F5_8      =  2;
static const byte CMDCH_F9_12     =  3;
static const byte CMDCH_F13_20    =  4;
static const byte CMDCH_F21_28    =  5;
static const byte CMDCH_F29_36    =  6;
static const byte CMDCH_F37_44    =  7;
static const byte CMDCH_F45_52    =  8;
static const byte CMDCH_F53_60    =  9;
static const byte CMDCH_F61_68    = 10;
static const byte CMDCH_CV_BYTE   = 11;
static const byte CMDCH_CV_BIT    = 12;
static const byte CMDCH_CV29_BIT5 = 13;
static const byte CMDCH_LADDR     = 14;
static const byte CMDCH_ACC       = 15;
static const byte CMDCH_ACC_EXT   = 16;

typedef struct {
    byte cmd;
    byte msb;
    byte lsb;
    union {
        byte dir_speed;
        byte f0_4;
        byte f5_8;
        byte f9_12;
        byte f13_20;
        byte f21_28;
        byte f29_36;
        byte f37_44;
        byte f45_52;
        byte f53_60;
        byte f61_68;
        struct {
            byte cv_msb;
            byte cv_lsb;
            union {
                byte cv;
                struct {
                    byte cv_bit;
                    bool cv_flag;
                };
            };
        };
        bool cv29_bit5;
        struct {
            byte long_msb;
            byte long_lsb;
        };
        struct {
            byte acc_out;
            bool acc_flag;
        };
        byte acc_status;
    };
} cmdch_in_t;

typedef struct {
    queue_t qin;
} cmdch_t;

// public interface

void cmdch_init(cmdch_t *cmdch);

void cmdch_dir_speed(cmdch_t *cmdch, byte msb, byte lsb, byte dir_speed);
void cmdch_f0_4(cmdch_t *cmdch, byte msb, byte lsb, byte f0_4);
void cmdch_f5_8(cmdch_t *cmdch, byte msb, byte lsb, byte f5_8);
void cmdch_f9_12(cmdch_t *cmdch, byte msb, byte lsb, byte f9_12);
void cmdch_f13_20(cmdch_t *cmdch, byte msb, byte lsb, byte f13_20);
void cmdch_f21_28(cmdch_t *cmdch, byte msb, byte lsb, byte f21_28);
void cmdch_f29_36(cmdch_t *cmdch, byte msb, byte lsb, byte f29_36);
void cmdch_f37_44(cmdch_t *cmdch, byte msb, byte lsb, byte f37_44);
void cmdch_f45_52(cmdch_t *cmdch, byte msb, byte lsb, byte f45_52);
void cmdch_f53_60(cmdch_t *cmdch, byte msb, byte lsb, byte f53_60);
void cmdch_f61_68(cmdch_t *cmdch, byte msb, byte lsb, byte f61_68);
void cmdch_cv_byte(cmdch_t *cmdch, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte cv);
void cmdch_cv_bit(cmdch_t *cmdch, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte cv_bit, bool cv_flag);
void cmdch_cv29_bit5(cmdch_t *cmdch, byte msb, byte lsb, bool cv29_bit5);
void cmdch_laddr(cmdch_t *cmdch, byte msb, byte lsb, byte new_msb, byte new_lsb);

void cmdch_acc(cmdch_t *cmdch, byte msb, byte lsb, byte acc_out, bool acc_flag);
void cmdch_acc_ext(cmdch_t *cmdch, byte msb, byte lsb, byte acc_status);

bool cmdch_try_remove_qin(cmdch_t *cmdch, cmdch_in_t *in);

#endif
