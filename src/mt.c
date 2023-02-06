#include "mt.h"
#include "cfg.h"

static inline byte mt_repeat(mt_t *mt, byte cmd) {
    // CV command ?
    if (cmd == CMDQ_CV_BYTE || cmd == CMDQ_CV_BIT || cmd == CMDQ_CV29_BIT5 || cmd == CMDQ_LADDR) return cfg_get_cv(CFG_MT_NUM_REPEAT_CV);
    // accessory command ?
    if (cmd == CMDQ_ACC || cmd == CMDQ_ACC_EXT) return cfg_get_cv(CFG_MT_NUM_REPEAT_ACC);
    // else
    return cfg_get_cv(CFG_MT_NUM_REPEAT);
}

static void mt_cmd_dir_speed(mt_t *mt, cmdq_in_t *in) { dcc_tx_sm_cmd_dir_speed(mt->tx_sm, in->msb, in->lsb, in->dir_speed); }
static void mt_cmd_f0_4(mt_t *mt, cmdq_in_t *in)      { dcc_tx_sm_cmd_f0_4(mt->tx_sm, in->msb, in->lsb, in->f0_4); }
static void mt_cmd_f5_8(mt_t *mt, cmdq_in_t *in)      { dcc_tx_sm_cmd_f5_8(mt->tx_sm, in->msb, in->lsb, in->f5_8); }
static void mt_cmd_f9_12(mt_t *mt, cmdq_in_t *in)     { dcc_tx_sm_cmd_f9_12(mt->tx_sm, in->msb, in->lsb, in->f9_12); }
static void mt_cmd_f13_20(mt_t *mt, cmdq_in_t *in)    { dcc_tx_sm_cmd_f13_20(mt->tx_sm, in->msb, in->lsb, in->f13_20); }
static void mt_cmd_f21_28(mt_t *mt, cmdq_in_t *in)    { dcc_tx_sm_cmd_f21_28(mt->tx_sm, in->msb, in->lsb, in->f21_28); }
static void mt_cmd_f29_36(mt_t *mt, cmdq_in_t *in)    { dcc_tx_sm_cmd_f29_36(mt->tx_sm, in->msb, in->lsb, in->f29_36); }
static void mt_cmd_f37_44(mt_t *mt, cmdq_in_t *in)    { dcc_tx_sm_cmd_f37_44(mt->tx_sm, in->msb, in->lsb, in->f37_44); }
static void mt_cmd_f45_52(mt_t *mt, cmdq_in_t *in)    { dcc_tx_sm_cmd_f45_52(mt->tx_sm, in->msb, in->lsb, in->f45_52); }
static void mt_cmd_f53_60(mt_t *mt, cmdq_in_t *in)    { dcc_tx_sm_cmd_f53_60(mt->tx_sm, in->msb, in->lsb, in->f53_60); }
static void mt_cmd_f61_68(mt_t *mt, cmdq_in_t *in)    { dcc_tx_sm_cmd_f61_68(mt->tx_sm, in->msb, in->lsb, in->f61_68); }
static void mt_cmd_cv_byte(mt_t *mt, cmdq_in_t *in)   { dcc_tx_sm_cmd_cv_byte(mt->tx_sm, in->msb, in->lsb, in->cv_msb, in->cv_lsb, in->cv); }
static void mt_cmd_cv_bit(mt_t *mt, cmdq_in_t *in)    { dcc_tx_sm_cmd_cv_bit(mt->tx_sm, in->msb, in->lsb, in->cv_msb, in->cv_lsb, in->cv_bit, in->cv_flag); }
static void mt_cmd_cv29_bit5(mt_t *mt, cmdq_in_t *in) { dcc_tx_sm_cmd_cv29_bit5(mt->tx_sm, in->msb, in->lsb, in->cv29_bit5); }
static void mt_cmd_laddr(mt_t *mt, cmdq_in_t *in)     { dcc_tx_sm_cmd_laddr(mt->tx_sm, in->msb, in->lsb, in->long_msb, in->long_lsb); }
static void mt_cmd_acc(mt_t *mt, cmdq_in_t *in)       { dcc_tx_sm_cmd_acc(mt->tx_sm, in->msb, in->lsb, in->acc_out, in->acc_flag); }
static void mt_cmd_acc_ext(mt_t *mt, cmdq_in_t *in)   { dcc_tx_sm_cmd_acc_ext(mt->tx_sm, in->msb, in->lsb, in->acc_status); }

typedef void (*mt_cmd_fn_t)(mt_t *mt, cmdq_in_t *in);
static const mt_cmd_fn_t mt_cmdq_fn[] = {
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

static void mt_dcc(mt_t *mt, cmdq_in_t *in) {
    byte cmd = in->cmd;
    for (int i = 0; i < mt_repeat(mt, cmd); i++) {
        mt_cmdq_fn[cmd](mt, in);
    }
}

static void mt_refresh_dcc(mt_t *mt, bool one_entry, cmdq_in_t *in) {
    // looks like that the dcc refresh is not supported by all decoders
    // dcc_refresh3(mt->dcc, mt->entry.msb, mt->entry.lsb, mt->entry.dir_speed, rbuf_slice_fcts(mt->entry.fcts, 0, 8));
    // so let's refresh by sending commands being supported by any decoder
    // in a round-robin way (dir_speed, f0_4, ...), so that 
    // - queue commands are handled quickly and
    // - no idle commands (some decoder do not work without pausing between commands) are needed,
    //   in case there is more than one entry in refresh buffer
    if (one_entry) dcc_tx_sm_cmd_idle(mt->tx_sm); // only one entry in buffer -> idle
    mt_cmdq_fn[in->cmd](mt, in);
}

static void mt_set_enabled(mt_t *mt, bool enabled) {
    mt->enabled = enabled;
    if (enabled) { 
        // switch led on if CV flag is set 
        if (cfg_get_cv(CFG_MT_CFG)&CFG_MT_CFG_LED) board_set_led(mt->board, true);
                
        // when switching on set whether def or bidi depending on CV
        if (cfg_get_cv(CFG_MT_CFG)&CFG_MT_CFG_BIDI) {
            mt->tx_sm = &mt->tx_pio->tx_sm_bidi;
        } else {
            mt->tx_sm = &mt->tx_pio->tx_sm_def;
        }
    } else {
        // switch led off if CV flag is set 
        if (cfg_get_cv(CFG_MT_CFG)&CFG_MT_CFG_LED) board_set_led(mt->board, false);
    }
    dcc_tx_sm_set_enabled(mt->tx_sm, enabled);
}

void mt_init(mt_t *mt, dcc_tx_pio_t *tx_pio, board_t *board, rbuf_t *rbuf, cmdq_t *cmdq) {
    mt->enabled = false;  // start not enabled

    mt->tx_pio   = tx_pio;
    mt->tx_sm    = &tx_pio->tx_sm_def;

    mt->board = board;
    mt->rbuf  = rbuf;
    mt->cmdq  = cmdq;

    dcc_tx_sm_set_enabled(mt->tx_sm, mt->enabled);
}

void mt_dispatch(mt_t *mt) {

    bool enabled = cfg_get_mt_enabled();
    if (enabled != mt->enabled) mt_set_enabled(mt, enabled);
    
    cmdq_in_t in;
    bool hasCmd = cmdq_try_remove_qin(mt->cmdq, &in); // remove command from queue

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
