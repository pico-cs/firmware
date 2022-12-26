#include <stdio.h>
#include "pico/binary_info.h"

#include "mt.h"

#include "dcc_tx.pio.h"

static const uint MT_DEFAULT_TX_PIN      =  2;
static const uint MT_DEFAULT_TX_INV_PIN  =  MT_DEFAULT_TX_PIN + 1;
static const uint MT_DEFAULT_ENABLED_PIN = 22;
static const PIO MT_DEFAULT_PIO = pio0;
static const uint MT_DEFAULT_SM = 0;

// configuration variables
static const byte MT_CV_NUM_SYNC_BIT        = 0;
static const byte MT_CV_NUM_REPEAT          = 1;
static const byte MT_CV_NUM_REPEAT_CV       = 2;
static const byte MT_CV_NUM_REPEAT_ACC      = 3;

typedef struct {
    const byte min;
    const byte max;
    const byte def;
} mt_cv_t;
 
static const mt_cv_t mt_cv[MT_NUM_CV] = {
	{17, 32, 17}, // MT_CV_NUM_SYNC_BIT
	{ 1,  5,  1}, // MT_CV_NUM_REPEAT
	{ 2,  5,  2}, // MT_CV_NUM_REPEAT_CV - command needs to be repeated twice to be accepted by the decoder
	{ 1,  5,  2}, // MT_CV_NUM_REPEAT_ACC
};

static byte mt_check_range(byte value, byte min, byte max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static inline byte mt_repeat(mt_t *mt, byte cmd) {
    // CV command ?
    if (cmd == CMDCH_CV_BYTE || cmd == CMDCH_CV_BIT || cmd == CMDCH_CV29_BIT5 || cmd == CMDCH_LADDR) return mt->cv[MT_CV_NUM_REPEAT_CV];
    // accessory command ?
    if (cmd == CMDCH_ACC || cmd == CMDCH_ACC_EXT) return mt->cv[MT_CV_NUM_REPEAT_ACC];
    // else
    return mt->cv[MT_CV_NUM_REPEAT];
}

static void mt_cmd_dir_speed(mt_t *mt, byte num_sync_bit, cmdch_in_t *in) { dcc_dir_speed(&mt->dcc, num_sync_bit, in->msb, in->lsb, in->dir_speed); }
static void mt_cmd_f0_4(mt_t *mt, byte num_sync_bit, cmdch_in_t *in)      { dcc_f0_4(&mt->dcc, num_sync_bit, in->msb, in->lsb, in->f0_4); }
static void mt_cmd_f5_8(mt_t *mt, byte num_sync_bit, cmdch_in_t *in)      { dcc_f5_8(&mt->dcc, num_sync_bit, in->msb, in->lsb, in->f5_8); }
static void mt_cmd_f9_12(mt_t *mt, byte num_sync_bit, cmdch_in_t *in)     { dcc_f9_12(&mt->dcc, num_sync_bit, in->msb, in->lsb, in->f9_12); }
static void mt_cmd_f13_20(mt_t *mt, byte num_sync_bit, cmdch_in_t *in)    { dcc_f13_20(&mt->dcc, num_sync_bit, in->msb, in->lsb, in->f13_20); }
static void mt_cmd_f21_28(mt_t *mt, byte num_sync_bit, cmdch_in_t *in)    { dcc_f21_28(&mt->dcc, num_sync_bit, in->msb, in->lsb, in->f21_28); }
static void mt_cmd_f29_36(mt_t *mt, byte num_sync_bit, cmdch_in_t *in)    { dcc_f29_36(&mt->dcc, num_sync_bit, in->msb, in->lsb, in->f29_36); }
static void mt_cmd_f37_44(mt_t *mt, byte num_sync_bit, cmdch_in_t *in)    { dcc_f37_44(&mt->dcc, num_sync_bit, in->msb, in->lsb, in->f37_44); }
static void mt_cmd_f45_52(mt_t *mt, byte num_sync_bit, cmdch_in_t *in)    { dcc_f45_52(&mt->dcc, num_sync_bit, in->msb, in->lsb, in->f45_52); }
static void mt_cmd_f53_60(mt_t *mt, byte num_sync_bit, cmdch_in_t *in)    { dcc_f53_60(&mt->dcc, num_sync_bit, in->msb, in->lsb, in->f53_60); }
static void mt_cmd_f61_68(mt_t *mt, byte num_sync_bit, cmdch_in_t *in)    { dcc_f61_68(&mt->dcc, num_sync_bit, in->msb, in->lsb, in->f61_68); }
static void mt_cmd_cv_byte(mt_t *mt, byte num_sync_bit, cmdch_in_t *in)   { dcc_cv_byte(&mt->dcc, num_sync_bit, in->msb, in->lsb, in->cv_msb, in->cv_lsb, in->cv); }
static void mt_cmd_cv_bit(mt_t *mt, byte num_sync_bit, cmdch_in_t *in)    { dcc_cv_bit(&mt->dcc, num_sync_bit, in->msb, in->lsb, in->cv_msb, in->cv_lsb, in->cv_bit, in->cv_flag); }
static void mt_cmd_cv29_bit5(mt_t *mt, byte num_sync_bit, cmdch_in_t *in) { dcc_cv29_bit5(&mt->dcc, num_sync_bit, in->msb, in->lsb, in->cv29_bit5); }
static void mt_cmd_laddr(mt_t *mt, byte num_sync_bit, cmdch_in_t *in)     { dcc_laddr(&mt->dcc, num_sync_bit, in->msb, in->lsb, in->long_msb, in->long_lsb); }
static void mt_cmd_acc(mt_t *mt, byte num_sync_bit, cmdch_in_t *in)       { dcc_acc(&mt->dcc, num_sync_bit, in->msb, in->lsb, in->acc_out, in->acc_flag); }
static void mt_cmd_acc_ext(mt_t *mt, byte num_sync_bit, cmdch_in_t *in)   { dcc_acc_ext(&mt->dcc, num_sync_bit, in->msb, in->lsb, in->acc_status); }

typedef void (*mt_cmdcher)(mt_t *mt, byte num_sync_bit, cmdch_in_t *in);
static const mt_cmdcher mt_cmdch_fn[CMDCH_NUM_CMD] = {
    mt_cmd_dir_speed,
    mt_cmd_f0_4,
    mt_cmd_f5_8,
    mt_cmd_f9_12,
    mt_cmd_f13_20,
    mt_cmd_f21_28,
    mt_cmd_f29_36,
    mt_cmd_f37_44,
    mt_cmd_f45_52,
    mt_cmd_f53_60,
    mt_cmd_f61_68,
    mt_cmd_cv_byte,
    mt_cmd_cv_bit,
    mt_cmd_cv29_bit5,
    mt_cmd_laddr,
    mt_cmd_acc,
    mt_cmd_acc_ext,
};

static void mt_dcc(mt_t *mt, byte num_sync_bit, cmdch_in_t *in) {
    byte cmd = in->cmd;
    for (int i = 0; i < mt_repeat(mt, cmd); i++) {
        mt_cmdch_fn[cmd](mt, num_sync_bit, in);
    }
}

static void mt_refresh_dcc(mt_t *mt, byte num_sync_bit, bool one_entry, cmdch_in_t *in) {
    // looks like that the dcc refresh is not supported by all decoders
    // dcc_refresh3(mt->dcc, mt->entry.msb, mt->entry.lsb, mt->entry.dir_speed, rbuf_slice_fcts(mt->entry.fcts, 0, 8));
    // so let's refresh by sending commands being supported by any decoder
    // in a round-robin way (dir_speed, f0_4, ...), so that 
    // - queue commands are handled quickly and
    // - no idle commands (some decoder do not work without pausing between commands) are needed,
    //   in case there is more than one entry in refresh buffer
    if (one_entry) dcc_idle(&mt->dcc, num_sync_bit); // only one entry in buffer -> idle
    mt_cmdch_fn[in->cmd](mt, num_sync_bit, in);
}

static void mt_put(PIO pio, uint sm, word w) {
    dcc_tx_program_put(pio, sm, w);
}

static bool mt_set_enabled(mt_t *mt, bool on) {
     mt->enabled = on;
     gpio_put(MT_DEFAULT_ENABLED_PIN, on ? 1 : 0);
     return on;
}

static void mt_cfg_get_enabled(mt_t *mt, cfgch_in_t *in, cfgch_out_t *out) {
    out->enabled = mt->enabled;
}

static void mt_cfg_set_enabled(mt_t *mt, cfgch_in_t *in, cfgch_out_t *out) {
    out->enabled = mt_set_enabled(mt, in->enabled);
}

static void mt_cfg_get_cv(mt_t *mt, cfgch_in_t *in, cfgch_out_t *out) {
    out->cv = (in->cv_idx < MT_NUM_CV) ? mt->cv[in->cv_idx] : 0;
}

static void mt_cfg_set_cv(mt_t *mt, cfgch_in_t *in, cfgch_out_t *out) {
    out->cv = 0;
    if (in->cv_idx >= MT_NUM_CV) return;
    
    mt->cv[in->cv_idx] = mt_check_range(in->cv, mt_cv[in->cv_idx].min, mt_cv[in->cv_idx].max);
    out->cv = mt->cv[in->cv_idx];
}

typedef void (*mt_cfgcher)(mt_t *mt, cfgch_in_t *in, cfgch_out_t *out);
static const mt_cfgcher mt_cfgch_fn[CFGCH_NUM_CMD] = {
    mt_cfg_get_cv,
    mt_cfg_set_cv,
    mt_cfg_get_enabled,
    mt_cfg_set_enabled,
};

static void mt_cfg(mt_t *mt) {
    cfgch_in_t in;
    cfgch_out_t out;
    
    if (!cfgch_try_remove_qin(mt->cfgch, &in)) return;
    mt_cfgch_fn[in.cmd](mt, &in, &out);
    cfgch_add_blocking_qout(mt->cfgch, &out);
}

void mt_init(mt_t *mt, rbuf_t *rbuf, cfgch_t *cfgch, cmdch_t *cmdch) {
    bi_decl(bi_1pin_with_name(MT_DEFAULT_TX_PIN, "Main track: DCC signal output"));
    bi_decl(bi_1pin_with_name(MT_DEFAULT_TX_INV_PIN, "Main track: DCC signal output (inverted)"));
    bi_decl(bi_1pin_with_name(MT_DEFAULT_ENABLED_PIN, "Main track: DCC signal enabled"));

    gpio_init(MT_DEFAULT_ENABLED_PIN);
    gpio_set_dir(MT_DEFAULT_ENABLED_PIN, GPIO_OUT);

    mt_set_enabled(mt, false); // start not enabled
    mt->rbuf = rbuf;
    mt->cfgch = cfgch;
    mt->cmdch = cmdch;

    // configuration variables
    for (int i = 0; i < MT_NUM_CV; i++) mt->cv[i] = mt_cv[i].def; // init cv
            
    uint offset = pio_add_program(MT_DEFAULT_PIO, &dcc_tx_program);
    dcc_tx_program_init(MT_DEFAULT_PIO, MT_DEFAULT_SM, offset, MT_DEFAULT_TX_PIN);
    dcc_tx_program_set_enabled(MT_DEFAULT_PIO, MT_DEFAULT_SM, true);

    dcc_init(&mt->dcc, mt_put, MT_DEFAULT_PIO, MT_DEFAULT_SM);
}

void mt_dispatch(mt_t *mt) {
    rbuf_entry_t entry;
    cmdch_in_t in;

    while (true) {

        mt_cfg(mt); // config change ?

        bool hasCmd = cmdch_try_remove_qin(mt->cmdch, &in); // remove command from queue

        if (mt->enabled) {
            
            byte num_sync_bit = mt->cv[MT_CV_NUM_SYNC_BIT];
            bool one_entry;

            if (hasCmd) {
                mt_dcc(mt, num_sync_bit, &in);
            } else if (rbuf_refresh(mt->rbuf, &one_entry, &in)) {
                mt_refresh_dcc(mt, num_sync_bit, one_entry, &in);
            } else {
                dcc_idle(&mt->dcc, num_sync_bit);
            }
        }
    }
}
