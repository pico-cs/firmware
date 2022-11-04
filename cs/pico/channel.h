#ifndef _CHANNEL_H
#define _CHANNEL_H

#include "pico/util/queue.h"

#include "common.h"

static const byte CHANNEL_CFG = 0x80; // config command
static const byte CHANNEL_DCC = 0x00; // dcc command

static const byte CHANNEL_CMD_GET_DCC_SYNC_BITS = CHANNEL_CFG | 0x01;
static const byte CHANNEL_CMD_SET_DCC_SYNC_BITS = CHANNEL_CFG | 0x02;
static const byte CHANNEL_CMD_GET_ENABLED       = CHANNEL_CFG | 0x03;
static const byte CHANNEL_CMD_SET_ENABLED       = CHANNEL_CFG | 0x04;

static const byte CHANNEL_CMD_DIR_SPEED         = CHANNEL_DCC | 0x01;
static const byte CHANNEL_CMD_F0_4              = CHANNEL_DCC | 0x02;
static const byte CHANNEL_CMD_F5_8              = CHANNEL_DCC | 0x03;
static const byte CHANNEL_CMD_F9_12             = CHANNEL_DCC | 0x04;
static const byte CHANNEL_CMD_F13_20            = CHANNEL_DCC | 0x05;
static const byte CHANNEL_CMD_F21_28            = CHANNEL_DCC | 0x06;
static const byte CHANNEL_CMD_F29_36            = CHANNEL_DCC | 0x07;
static const byte CHANNEL_CMD_F37_44            = CHANNEL_DCC | 0x08;
static const byte CHANNEL_CMD_F45_52            = CHANNEL_DCC | 0x09;
static const byte CHANNEL_CMD_F53_60            = CHANNEL_DCC | 0x0a;
static const byte CHANNEL_CMD_F61_68            = CHANNEL_DCC | 0x0b;
static const byte CHANNEL_CMD_CV_BYTE           = CHANNEL_DCC | 0x0c;
static const byte CHANNEL_CMD_CV_BIT            = CHANNEL_DCC | 0x0d;
static const byte CHANNEL_CMD_CV29_BIT5         = CHANNEL_DCC | 0x0e;
static const byte CHANNEL_CMD_LADDR             = CHANNEL_DCC | 0x0f;

typedef struct {
    byte cmd;
    union {
        uint dcc_sync_bits;       // config dcc sync bits
        bool enabled;             // config enable
        struct {                  // loco cmd
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
                            byte bit;
                            bool flag;
                        };
                    };
                };
                bool cv29_bit5;
                struct {
                    byte long_msb;
                    byte long_lsb;
                };
            };
        };
    };
} channel_in_t;

typedef struct {
    union {
        uint dcc_sync_bits;       // config dcc sync bits
        bool enabled;             // config enable
    };
} channel_out_t;

typedef struct {
    queue_t qin;
    queue_t qout;
    channel_in_t in;   // scratch buffer
    channel_out_t out; // scratch buffer
} channel_t;

// public interface

void channel_init(channel_t *channel);

uint channel_get_dcc_sync_bits(channel_t *channel);
uint channel_set_dcc_sync_bits(channel_t *channel, uint sync_bits);
bool channel_get_enabled(channel_t *channel);
void channel_set_enabled(channel_t *channel, bool enabled);

void channel_dir_speed(channel_t *channel, byte msb, byte lsb, byte dir_speed);
void channel_f0_4(channel_t *channel, byte msb, byte lsb, byte f0_4);
void channel_f5_8(channel_t *channel, byte msb, byte lsb, byte f5_8);
void channel_f9_12(channel_t *channel, byte msb, byte lsb, byte f9_12);
void channel_f13_20(channel_t *channel, byte msb, byte lsb, byte f13_20);
void channel_f21_28(channel_t *channel, byte msb, byte lsb, byte f21_28);
void channel_f29_36(channel_t *channel, byte msb, byte lsb, byte f29_36);
void channel_f37_44(channel_t *channel, byte msb, byte lsb, byte f37_44);
void channel_f45_52(channel_t *channel, byte msb, byte lsb, byte f45_52);
void channel_f53_60(channel_t *channel, byte msb, byte lsb, byte f53_60);
void channel_f61_68(channel_t *channel, byte msb, byte lsb, byte f61_68);
void channel_cv_byte(channel_t *channel, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte cv);
void channel_cv_bit(channel_t *channel, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte bit, bool flag);
void channel_cv29_bit5(channel_t *channel, byte msb, byte lsb, bool cv29_bit5);
void channel_laddr(channel_t *channel, byte msb, byte lsb, byte new_msb, byte new_lsb);

bool channel_try_remove_qin(channel_t *channel, channel_in_t *in);
void channel_remove_blocking_qin(channel_t *channel, channel_in_t *in);
void channel_add_blocking_qout(channel_t *channel, channel_out_t *out);

#endif
