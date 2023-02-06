#include "cmdq.h"

// command
static const int cmdq_num_entry_in  = 8;

void cmdq_init(cmdq_t *cmdq) {
    queue_init(&cmdq->qin, sizeof(cmdq_in_t), cmdq_num_entry_in);
}

void cmdq_dir_speed(cmdq_t *cmdq, byte msb, byte lsb, byte dir_speed) {
    cmdq_in_t in;
    in.cmd = CMDQ_DIR_SPEED;
    in.msb = msb;
    in.lsb = lsb;
    in.dir_speed = dir_speed;
    queue_add_blocking(&cmdq->qin, &in);
};

void cmdq_f0_4(cmdq_t *cmdq, byte msb, byte lsb, byte f0_4) {
    cmdq_in_t in;
    in.cmd = CMDQ_F0_4;
    in.msb = msb;
    in.lsb = lsb;
    in.f0_4 = f0_4;
    queue_add_blocking(&cmdq->qin, &in);
};

void cmdq_f5_8(cmdq_t *cmdq, byte msb, byte lsb, byte f5_8) {
    cmdq_in_t in;
    in.cmd = CMDQ_F5_8;
    in.msb = msb;
    in.lsb = lsb;
    in.f5_8 = f5_8;
    queue_add_blocking(&cmdq->qin, &in);
};

void cmdq_f9_12(cmdq_t *cmdq, byte msb, byte lsb, byte f9_12) {
    cmdq_in_t in;
    in.cmd = CMDQ_F9_12;
    in.msb = msb;
    in.lsb = lsb;
    in.f9_12 = f9_12;
    queue_add_blocking(&cmdq->qin, &in);
};

void cmdq_f13_20(cmdq_t *cmdq, byte msb, byte lsb, byte f13_20) {
    cmdq_in_t in;
    in.cmd = CMDQ_F13_20;
    in.msb = msb;
    in.lsb = lsb;
    in.f13_20 = f13_20;
    queue_add_blocking(&cmdq->qin, &in);
};


void cmdq_f21_28(cmdq_t *cmdq, byte msb, byte lsb, byte f21_28) {
    cmdq_in_t in;
    in.cmd = CMDQ_F21_28;
    in.msb = msb;
    in.lsb = lsb;
    in.f21_28 = f21_28;
    queue_add_blocking(&cmdq->qin, &in);
};

void cmdq_f29_36(cmdq_t *cmdq, byte msb, byte lsb, byte f29_36) {
    cmdq_in_t in;
    in.cmd = CMDQ_F29_36;
    in.msb = msb;
    in.lsb = lsb;
    in.f29_36 = f29_36;
    queue_add_blocking(&cmdq->qin, &in);
};

void cmdq_f37_44(cmdq_t *cmdq, byte msb, byte lsb, byte f37_44) {
    cmdq_in_t in;
    in.cmd = CMDQ_F37_44;
    in.msb = msb;
    in.lsb = lsb;
    in.f37_44 = f37_44;
    queue_add_blocking(&cmdq->qin, &in);
};

void cmdq_f45_52(cmdq_t *cmdq, byte msb, byte lsb, byte f45_52) {
    cmdq_in_t in;
    in.cmd = CMDQ_F45_52;
    in.msb = msb;
    in.lsb = lsb;
    in.f45_52 = f45_52;
    queue_add_blocking(&cmdq->qin, &in);
};

void cmdq_f53_60(cmdq_t *cmdq, byte msb, byte lsb, byte f53_60) {
    cmdq_in_t in;
    in.cmd = CMDQ_F53_60;
    in.msb = msb;
    in.lsb = lsb;
    in.f53_60 = f53_60;
    queue_add_blocking(&cmdq->qin, &in);
};

void cmdq_f61_68(cmdq_t *cmdq, byte msb, byte lsb, byte f61_68) {
    cmdq_in_t in;
    in.cmd = CMDQ_F61_68;
    in.msb = msb;
    in.lsb = lsb;
    in.f61_68 = f61_68;
    queue_add_blocking(&cmdq->qin, &in);
};

void cmdq_cv_byte(cmdq_t *cmdq, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte cv) {
    cmdq_in_t in;
    in.cmd = CMDQ_CV_BYTE;
    in.msb = msb;
    in.lsb = lsb;
    in.cv_msb = cv_msb;
    in.cv_lsb = cv_lsb;
    in.cv = cv;
    queue_add_blocking(&cmdq->qin, &in);
}

void cmdq_cv_bit(cmdq_t *cmdq, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte cv_bit, bool cv_flag) {
    cmdq_in_t in;
    in.cmd = CMDQ_CV_BIT;
    in.msb = msb;
    in.lsb = lsb;
    in.cv_msb = cv_msb;
    in.cv_lsb = cv_lsb;
    in.cv_bit = cv_bit;
    in.cv_flag = cv_flag;
    queue_add_blocking(&cmdq->qin, &in);
}

void cmdq_cv29_bit5(cmdq_t *cmdq, byte msb, byte lsb, bool cv29_bit5) {
    cmdq_in_t in;
    in.cmd = CMDQ_CV29_BIT5;
    in.msb = msb;
    in.lsb = lsb;
    in.cv29_bit5 = cv29_bit5;
    queue_add_blocking(&cmdq->qin, &in);
}

void cmdq_laddr(cmdq_t *cmdq, byte msb, byte lsb, byte long_msb, byte long_lsb) {
    cmdq_in_t in;
    in.cmd = CMDQ_LADDR;
    in.msb = msb;
    in.lsb = lsb;
    in.long_msb = long_msb;
    in.long_lsb = long_lsb;
    queue_add_blocking(&cmdq->qin, &in);
}

void cmdq_acc(cmdq_t *cmdq, byte msb, byte lsb, byte acc_out, bool acc_flag) {
    cmdq_in_t in;
    in.cmd = CMDQ_ACC;
    in.msb = msb;
    in.lsb = lsb;
    in.acc_out = acc_out;
    in.acc_flag = acc_flag;
    queue_add_blocking(&cmdq->qin, &in);
};

void cmdq_acc_ext(cmdq_t *cmdq, byte msb, byte lsb, byte acc_status) {
    cmdq_in_t in;
    in.cmd = CMDQ_ACC_EXT;
    in.msb = msb;
    in.lsb = lsb;
    in.acc_status = acc_status;
    queue_add_blocking(&cmdq->qin, &in);
};

bool cmdq_try_remove_qin(cmdq_t *cmdq, cmdq_in_t *in) {
    return queue_try_remove(&cmdq->qin, in);
}
