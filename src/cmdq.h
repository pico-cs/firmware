#ifndef _CMDQ_H
#define _CMDQ_H

#include "pico/util/queue.h"

#include "common.h"

// command

static const byte CMDQ_DIR_SPEED =  0;
static const byte CMDQ_F0_4      =  1;
static const byte CMDQ_F5_8      =  2;
static const byte CMDQ_F9_12     =  3;
static const byte CMDQ_F13_20    =  4;
static const byte CMDQ_F21_28    =  5;
static const byte CMDQ_F29_36    =  6;
static const byte CMDQ_F37_44    =  7;
static const byte CMDQ_F45_52    =  8;
static const byte CMDQ_F53_60    =  9;
static const byte CMDQ_F61_68    = 10;
static const byte CMDQ_CV_BYTE   = 11;
static const byte CMDQ_CV_BIT    = 12;
static const byte CMDQ_CV29_BIT5 = 13;
static const byte CMDQ_LADDR     = 14;
static const byte CMDQ_ACC       = 15;
static const byte CMDQ_ACC_EXT   = 16;

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
} cmdq_in_t;

typedef struct {
    queue_t qin;
} cmdq_t;

// public interface

void cmdq_init(cmdq_t *cmdq);

void cmdq_dir_speed(cmdq_t *cmdq, byte msb, byte lsb, byte dir_speed);
void cmdq_f0_4(cmdq_t *cmdq, byte msb, byte lsb, byte f0_4);
void cmdq_f5_8(cmdq_t *cmdq, byte msb, byte lsb, byte f5_8);
void cmdq_f9_12(cmdq_t *cmdq, byte msb, byte lsb, byte f9_12);
void cmdq_f13_20(cmdq_t *cmdq, byte msb, byte lsb, byte f13_20);
void cmdq_f21_28(cmdq_t *cmdq, byte msb, byte lsb, byte f21_28);
void cmdq_f29_36(cmdq_t *cmdq, byte msb, byte lsb, byte f29_36);
void cmdq_f37_44(cmdq_t *cmdq, byte msb, byte lsb, byte f37_44);
void cmdq_f45_52(cmdq_t *cmdq, byte msb, byte lsb, byte f45_52);
void cmdq_f53_60(cmdq_t *cmdq, byte msb, byte lsb, byte f53_60);
void cmdq_f61_68(cmdq_t *cmdq, byte msb, byte lsb, byte f61_68);
void cmdq_cv_byte(cmdq_t *cmdq, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte cv);
void cmdq_cv_bit(cmdq_t *cmdq, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte cv_bit, bool cv_flag);
void cmdq_cv29_bit5(cmdq_t *cmdq, byte msb, byte lsb, bool cv29_bit5);
void cmdq_laddr(cmdq_t *cmdq, byte msb, byte lsb, byte new_msb, byte new_lsb);

void cmdq_acc(cmdq_t *cmdq, byte msb, byte lsb, byte acc_out, bool acc_flag);
void cmdq_acc_ext(cmdq_t *cmdq, byte msb, byte lsb, byte acc_status);

bool cmdq_try_remove_qin(cmdq_t *cmdq, cmdq_in_t *in);

#endif
