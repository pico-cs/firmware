#include "mt.h"

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
	{ 0,  1,  0}, // MT_CV_BIDI
	{26, 32, 26}, // MT_CV_BIDI_TS
	{10, 16, 12}, // MT_CV_BIDI_TE
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

static void mt_cmd_dir_speed(mt_t *mt, cmdch_in_t *in) { dcc_tx_sm_cmd_dir_speed(mt->tx_sm, in->msb, in->lsb, in->dir_speed); }
static void mt_cmd_f0_4(mt_t *mt, cmdch_in_t *in)      { dcc_tx_sm_cmd_f0_4(mt->tx_sm, in->msb, in->lsb, in->f0_4); }
static void mt_cmd_f5_8(mt_t *mt, cmdch_in_t *in)      { dcc_tx_sm_cmd_f5_8(mt->tx_sm, in->msb, in->lsb, in->f5_8); }
static void mt_cmd_f9_12(mt_t *mt, cmdch_in_t *in)     { dcc_tx_sm_cmd_f9_12(mt->tx_sm, in->msb, in->lsb, in->f9_12); }
static void mt_cmd_f13_20(mt_t *mt, cmdch_in_t *in)    { dcc_tx_sm_cmd_f13_20(mt->tx_sm, in->msb, in->lsb, in->f13_20); }
static void mt_cmd_f21_28(mt_t *mt, cmdch_in_t *in)    { dcc_tx_sm_cmd_f21_28(mt->tx_sm, in->msb, in->lsb, in->f21_28); }
static void mt_cmd_f29_36(mt_t *mt, cmdch_in_t *in)    { dcc_tx_sm_cmd_f29_36(mt->tx_sm, in->msb, in->lsb, in->f29_36); }
static void mt_cmd_f37_44(mt_t *mt, cmdch_in_t *in)    { dcc_tx_sm_cmd_f37_44(mt->tx_sm, in->msb, in->lsb, in->f37_44); }
static void mt_cmd_f45_52(mt_t *mt, cmdch_in_t *in)    { dcc_tx_sm_cmd_f45_52(mt->tx_sm, in->msb, in->lsb, in->f45_52); }
static void mt_cmd_f53_60(mt_t *mt, cmdch_in_t *in)    { dcc_tx_sm_cmd_f53_60(mt->tx_sm, in->msb, in->lsb, in->f53_60); }
static void mt_cmd_f61_68(mt_t *mt, cmdch_in_t *in)    { dcc_tx_sm_cmd_f61_68(mt->tx_sm, in->msb, in->lsb, in->f61_68); }
static void mt_cmd_cv_byte(mt_t *mt, cmdch_in_t *in)   { dcc_tx_sm_cmd_cv_byte(mt->tx_sm, in->msb, in->lsb, in->cv_msb, in->cv_lsb, in->cv); }
static void mt_cmd_cv_bit(mt_t *mt, cmdch_in_t *in)    { dcc_tx_sm_cmd_cv_bit(mt->tx_sm, in->msb, in->lsb, in->cv_msb, in->cv_lsb, in->cv_bit, in->cv_flag); }
static void mt_cmd_cv29_bit5(mt_t *mt, cmdch_in_t *in) { dcc_tx_sm_cmd_cv29_bit5(mt->tx_sm, in->msb, in->lsb, in->cv29_bit5); }
static void mt_cmd_laddr(mt_t *mt, cmdch_in_t *in)     { dcc_tx_sm_cmd_laddr(mt->tx_sm, in->msb, in->lsb, in->long_msb, in->long_lsb); }
static void mt_cmd_acc(mt_t *mt, cmdch_in_t *in)       { dcc_tx_sm_cmd_acc(mt->tx_sm, in->msb, in->lsb, in->acc_out, in->acc_flag); }
static void mt_cmd_acc_ext(mt_t *mt, cmdch_in_t *in)   { dcc_tx_sm_cmd_acc_ext(mt->tx_sm, in->msb, in->lsb, in->acc_status); }

typedef void (*mt_cmdcher)(mt_t *mt, cmdch_in_t *in);
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

static void mt_dcc(mt_t *mt, cmdch_in_t *in) {
    byte cmd = in->cmd;
    for (int i = 0; i < mt_repeat(mt, cmd); i++) {
        mt_cmdch_fn[cmd](mt, in);
    }
}

static void mt_refresh_dcc(mt_t *mt, bool one_entry, cmdch_in_t *in) {
    // looks like that the dcc refresh is not supported by all decoders
    // dcc_refresh3(mt->dcc, mt->entry.msb, mt->entry.lsb, mt->entry.dir_speed, rbuf_slice_fcts(mt->entry.fcts, 0, 8));
    // so let's refresh by sending commands being supported by any decoder
    // in a round-robin way (dir_speed, f0_4, ...), so that 
    // - queue commands are handled quickly and
    // - no idle commands (some decoder do not work without pausing between commands) are needed,
    //   in case there is more than one entry in refresh buffer
    if (one_entry) dcc_tx_sm_cmd_idle(mt->tx_sm); // only one entry in buffer -> idle
    mt_cmdch_fn[in->cmd](mt, in);
}

static void mt_cfg_get_enabled(mt_t *mt, cfgch_in_t *in, cfgch_out_t *out) {
    out->enabled = mt->enabled;
}

static void mt_cfg_set_enabled(mt_t *mt, cfgch_in_t *in, cfgch_out_t *out) {
    out->enabled = in->enabled;
    if (in->enabled == mt->enabled) return;
    mt->enabled = in->enabled;
    if (mt->enabled) { // when switching on set whether def or bidi depending on CV
        if (mt->cv[MT_CV_BIDI] == 0) {
            mt->tx_sm = &mt->tx_pio->tx_sm_def;
        } else {
            mt->tx_sm = &mt->tx_pio->tx_sm_bidi;
        }
    }
    dcc_tx_sm_set_enabled(mt->tx_sm, mt->enabled, mt->cv);
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

void mt_init(mt_t *mt, dcc_tx_pio_t *tx_pio, rbuf_t *rbuf, cfgch_t *cfgch, cmdch_t *cmdch) {
    mt->enabled = false;  // start not enabled

    mt->tx_pio   = tx_pio;
    mt->tx_sm    = &tx_pio->tx_sm_def;

    mt->rbuf  = rbuf;
    mt->cfgch = cfgch;
    mt->cmdch = cmdch;

    // configuration variables
    for (int i = 0; i < MT_NUM_CV; i++) mt->cv[i] = mt_cv[i].def; // init cv
            
    dcc_tx_sm_set_enabled(mt->tx_sm, mt->enabled, mt->cv);
}

void mt_dispatch(mt_t *mt) {
    rbuf_entry_t entry;
    cmdch_in_t in;

    while (true) {

        mt_cfg(mt); // config change ?

        bool hasCmd = cmdch_try_remove_qin(mt->cmdch, &in); // remove command from queue

        if (mt->enabled) {
            
            bool one_entry;

            if (hasCmd) {
                mt_dcc(mt, &in);
            } else if (rbuf_refresh(mt->rbuf, &one_entry, &in)) {
                mt_refresh_dcc(mt, one_entry, &in);
            } else {
                dcc_tx_sm_cmd_idle(mt->tx_sm);
            }
        }
    }
}
