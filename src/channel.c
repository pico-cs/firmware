#include "channel.h"


// config
void cfgch_init(cfgch_t *cfgch) {
    // request response (only one entry per queue)
    queue_init(&cfgch->qin, sizeof(cfgch_in_t), 1);
    queue_init(&cfgch->qout, sizeof(cfgch_out_t), 1);
}

byte cfgch_get_cv(cfgch_t *cfgch, byte idx) {
    cfgch_in_t in;
    cfgch_out_t out;
    in.cmd = CFGCH_GET_CV;
    in.cv_idx = idx;
    queue_add_blocking(&cfgch->qin, &in);
    queue_remove_blocking(&cfgch->qout, &out);
    return out.cv;
}

byte cfgch_set_cv(cfgch_t *cfgch, byte idx, byte cv) {
    cfgch_in_t in;
    cfgch_out_t out;
    in.cmd = CFGCH_SET_CV;
    in.cv_idx = idx;
    in.cv = cv;
    queue_add_blocking(&cfgch->qin, &in);
    queue_remove_blocking(&cfgch->qout, &out);
    return out.cv;
}

bool cfgch_get_enabled(cfgch_t *cfgch) {
    cfgch_in_t in;
    cfgch_out_t out;
    in.cmd = CFGCH_GET_ENABLED;
    queue_add_blocking(&cfgch->qin, &in);
    queue_remove_blocking(&cfgch->qout, &out);
    return out.enabled;
}

bool cfgch_set_enabled(cfgch_t *cfgch, bool enabled) {
    cfgch_in_t in;
    cfgch_out_t out;
    in.cmd = CFGCH_SET_ENABLED;
    in.enabled = enabled;
    queue_add_blocking(&cfgch->qin, &in);
    queue_remove_blocking(&cfgch->qout, &out);
    return out.enabled;
}

bool cfgch_try_remove_qin(cfgch_t *cfgch, cfgch_in_t *in) {
    return queue_try_remove(&cfgch->qin, in);
}

void cfgch_add_blocking_qout(cfgch_t *cfgch, cfgch_out_t *out) {
    queue_add_blocking(&cfgch->qout, out);
}

// command
static const int cmdch_num_entry_in  = 8;

void cmdch_init(cmdch_t *cmdch) {
    queue_init(&cmdch->qin, sizeof(cmdch_in_t), cmdch_num_entry_in);
}

void cmdch_dir_speed(cmdch_t *cmdch, byte msb, byte lsb, byte dir_speed) {
    cmdch_in_t in;
    in.cmd = CMDCH_DIR_SPEED;
    in.msb = msb;
    in.lsb = lsb;
    in.dir_speed = dir_speed;
    queue_add_blocking(&cmdch->qin, &in);
};

void cmdch_f0_4(cmdch_t *cmdch, byte msb, byte lsb, byte f0_4) {
    cmdch_in_t in;
    in.cmd = CMDCH_F0_4;
    in.msb = msb;
    in.lsb = lsb;
    in.f0_4 = f0_4;
    queue_add_blocking(&cmdch->qin, &in);
};

void cmdch_f5_8(cmdch_t *cmdch, byte msb, byte lsb, byte f5_8) {
    cmdch_in_t in;
    in.cmd = CMDCH_F5_8;
    in.msb = msb;
    in.lsb = lsb;
    in.f5_8 = f5_8;
    queue_add_blocking(&cmdch->qin, &in);
};

void cmdch_f9_12(cmdch_t *cmdch, byte msb, byte lsb, byte f9_12) {
    cmdch_in_t in;
    in.cmd = CMDCH_F9_12;
    in.msb = msb;
    in.lsb = lsb;
    in.f9_12 = f9_12;
    queue_add_blocking(&cmdch->qin, &in);
};

void cmdch_f13_20(cmdch_t *cmdch, byte msb, byte lsb, byte f13_20) {
    cmdch_in_t in;
    in.cmd = CMDCH_F13_20;
    in.msb = msb;
    in.lsb = lsb;
    in.f13_20 = f13_20;
    queue_add_blocking(&cmdch->qin, &in);
};


void cmdch_f21_28(cmdch_t *cmdch, byte msb, byte lsb, byte f21_28) {
    cmdch_in_t in;
    in.cmd = CMDCH_F21_28;
    in.msb = msb;
    in.lsb = lsb;
    in.f21_28 = f21_28;
    queue_add_blocking(&cmdch->qin, &in);
};

void cmdch_f29_36(cmdch_t *cmdch, byte msb, byte lsb, byte f29_36) {
    cmdch_in_t in;
    in.cmd = CMDCH_F29_36;
    in.msb = msb;
    in.lsb = lsb;
    in.f29_36 = f29_36;
    queue_add_blocking(&cmdch->qin, &in);
};

void cmdch_f37_44(cmdch_t *cmdch, byte msb, byte lsb, byte f37_44) {
    cmdch_in_t in;
    in.cmd = CMDCH_F37_44;
    in.msb = msb;
    in.lsb = lsb;
    in.f37_44 = f37_44;
    queue_add_blocking(&cmdch->qin, &in);
};

void cmdch_f45_52(cmdch_t *cmdch, byte msb, byte lsb, byte f45_52) {
    cmdch_in_t in;
    in.cmd = CMDCH_F45_52;
    in.msb = msb;
    in.lsb = lsb;
    in.f45_52 = f45_52;
    queue_add_blocking(&cmdch->qin, &in);
};

void cmdch_f53_60(cmdch_t *cmdch, byte msb, byte lsb, byte f53_60) {
    cmdch_in_t in;
    in.cmd = CMDCH_F53_60;
    in.msb = msb;
    in.lsb = lsb;
    in.f53_60 = f53_60;
    queue_add_blocking(&cmdch->qin, &in);
};

void cmdch_f61_68(cmdch_t *cmdch, byte msb, byte lsb, byte f61_68) {
    cmdch_in_t in;
    in.cmd = CMDCH_F61_68;
    in.msb = msb;
    in.lsb = lsb;
    in.f61_68 = f61_68;
    queue_add_blocking(&cmdch->qin, &in);
};

void cmdch_cv_byte(cmdch_t *cmdch, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte cv) {
    cmdch_in_t in;
    in.cmd = CMDCH_CV_BYTE;
    in.msb = msb;
    in.lsb = lsb;
    in.cv_msb = cv_msb;
    in.cv_lsb = cv_lsb;
    in.cv = cv;
    queue_add_blocking(&cmdch->qin, &in);
}

void cmdch_cv_bit(cmdch_t *cmdch, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte cv_bit, bool cv_flag) {
    cmdch_in_t in;
    in.cmd = CMDCH_CV_BIT;
    in.msb = msb;
    in.lsb = lsb;
    in.cv_msb = cv_msb;
    in.cv_lsb = cv_lsb;
    in.cv_bit = cv_bit;
    in.cv_flag = cv_flag;
    queue_add_blocking(&cmdch->qin, &in);
}

void cmdch_cv29_bit5(cmdch_t *cmdch, byte msb, byte lsb, bool cv29_bit5) {
    cmdch_in_t in;
    in.cmd = CMDCH_CV29_BIT5;
    in.msb = msb;
    in.lsb = lsb;
    in.cv29_bit5 = cv29_bit5;
    queue_add_blocking(&cmdch->qin, &in);
}

void cmdch_laddr(cmdch_t *cmdch, byte msb, byte lsb, byte long_msb, byte long_lsb) {
    cmdch_in_t in;
    in.cmd = CMDCH_LADDR;
    in.msb = msb;
    in.lsb = lsb;
    in.long_msb = long_msb;
    in.long_lsb = long_lsb;
    queue_add_blocking(&cmdch->qin, &in);
}

void cmdch_acc(cmdch_t *cmdch, byte msb, byte lsb, byte acc_out, bool acc_flag) {
    cmdch_in_t in;
    in.cmd = CMDCH_ACC;
    in.msb = msb;
    in.lsb = lsb;
    in.acc_out = acc_out;
    in.acc_flag = acc_flag;
    queue_add_blocking(&cmdch->qin, &in);
};

void cmdch_acc_ext(cmdch_t *cmdch, byte msb, byte lsb, byte acc_status) {
    cmdch_in_t in;
    in.cmd = CMDCH_ACC_EXT;
    in.msb = msb;
    in.lsb = lsb;
    in.acc_status = acc_status;
    queue_add_blocking(&cmdch->qin, &in);
};

bool cmdch_try_remove_qin(cmdch_t *cmdch, cmdch_in_t *in) {
    return queue_try_remove(&cmdch->qin, in);
}
