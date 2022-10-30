#include "channel.h"

static const int channel_num_entry_in  = 8;
static const int channel_num_entry_out = 1;

void channel_init(channel_t *channel) {
    queue_init(&channel->qin, sizeof(channel_in_t), channel_num_entry_in);
    queue_init(&channel->qout, sizeof(channel_out_t), channel_num_entry_out);
}

uint channel_get_dcc_sync_bits(channel_t *channel) {
    channel->in.cmd = CHANNEL_CMD_GET_DCC_SYNC_BITS;
    queue_add_blocking(&channel->qin, &channel->in);
    queue_remove_blocking(&channel->qout, &channel->out);
    return channel->out.dcc_sync_bits;
};

void channel_set_dcc_sync_bits(channel_t *channel, uint sync_bits) {
    channel->in.cmd = CHANNEL_CMD_SET_DCC_SYNC_BITS;
    channel->in.dcc_sync_bits = sync_bits;
    queue_add_blocking(&channel->qin, &channel->in);
};

bool channel_get_enabled(channel_t *channel) {
    channel->in.cmd = CHANNEL_CMD_GET_ENABLED;
    queue_add_blocking(&channel->qin, &channel->in);
    queue_remove_blocking(&channel->qout, &channel->out);
    return channel->out.enabled;
};

void channel_set_enabled(channel_t *channel, bool enabled) {
    channel->in.cmd = CHANNEL_CMD_SET_ENABLED;
    channel->in.enabled = enabled;
    queue_add_blocking(&channel->qin, &channel->in);
};

void channel_dir_speed(channel_t *channel, byte msb, byte lsb, byte dir_speed) {
    channel->in.cmd = CHANNEL_CMD_DIR_SPEED;
    channel->in.msb = msb;
    channel->in.lsb = lsb;
    channel->in.dir_speed = dir_speed;
    queue_add_blocking(&channel->qin, &channel->in);
};

void channel_f0_4(channel_t *channel, byte msb, byte lsb, byte f0_4) {
    channel->in.cmd = CHANNEL_CMD_F0_4;
    channel->in.msb = msb;
    channel->in.lsb = lsb;
    channel->in.f0_4 = f0_4;
    queue_add_blocking(&channel->qin, &channel->in);
};

void channel_f5_8(channel_t *channel, byte msb, byte lsb, byte f5_8) {
    channel->in.cmd = CHANNEL_CMD_F5_8;
    channel->in.msb = msb;
    channel->in.lsb = lsb;
    channel->in.f5_8 = f5_8;
    queue_add_blocking(&channel->qin, &channel->in);
};

void channel_f9_12(channel_t *channel, byte msb, byte lsb, byte f9_12) {
    channel->in.cmd = CHANNEL_CMD_F9_12;
    channel->in.msb = msb;
    channel->in.lsb = lsb;
    channel->in.f9_12 = f9_12;
    queue_add_blocking(&channel->qin, &channel->in);
};

void channel_f13_20(channel_t *channel, byte msb, byte lsb, byte f13_20) {
    channel->in.cmd = CHANNEL_CMD_F13_20;
    channel->in.msb = msb;
    channel->in.lsb = lsb;
    channel->in.f13_20 = f13_20;
    queue_add_blocking(&channel->qin, &channel->in);
};

void channel_f21_28(channel_t *channel, byte msb, byte lsb, byte f21_28) {
    channel->in.cmd = CHANNEL_CMD_F21_28;
    channel->in.msb = msb;
    channel->in.lsb = lsb;
    channel->in.f21_28 = f21_28;
    queue_add_blocking(&channel->qin, &channel->in);
};

void channel_f29_36(channel_t *channel, byte msb, byte lsb, byte f29_36) {
    channel->in.cmd = CHANNEL_CMD_F29_36;
    channel->in.msb = msb;
    channel->in.lsb = lsb;
    channel->in.f29_36 = f29_36;
    queue_add_blocking(&channel->qin, &channel->in);
};

void channel_f37_44(channel_t *channel, byte msb, byte lsb, byte f37_44) {
    channel->in.cmd = CHANNEL_CMD_F37_44;
    channel->in.msb = msb;
    channel->in.lsb = lsb;
    channel->in.f37_44 = f37_44;
    queue_add_blocking(&channel->qin, &channel->in);
};

void channel_f45_52(channel_t *channel, byte msb, byte lsb, byte f45_52) {
    channel->in.cmd = CHANNEL_CMD_F45_52;
    channel->in.msb = msb;
    channel->in.lsb = lsb;
    channel->in.f45_52 = f45_52;
    queue_add_blocking(&channel->qin, &channel->in);
};

void channel_f53_60(channel_t *channel, byte msb, byte lsb, byte f53_60) {
    channel->in.cmd = CHANNEL_CMD_F53_60;
    channel->in.msb = msb;
    channel->in.lsb = lsb;
    channel->in.f53_60 = f53_60;
    queue_add_blocking(&channel->qin, &channel->in);
};

void channel_f61_68(channel_t *channel, byte msb, byte lsb, byte f61_68) {
    channel->in.cmd = CHANNEL_CMD_F61_68;
    channel->in.msb = msb;
    channel->in.lsb = lsb;
    channel->in.f61_68 = f61_68;
    queue_add_blocking(&channel->qin, &channel->in);
};

void channel_cv_byte(channel_t *channel, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte cv) {
    channel->in.cmd = CHANNEL_CMD_CV_BYTE;
    channel->in.msb = msb;
    channel->in.lsb = lsb;
    channel->in.cv_msb = cv_msb;
    channel->in.cv_lsb = cv_lsb;
    channel->in.cv = cv;
    queue_add_blocking(&channel->qin, &channel->in);
}

void channel_cv_bit(channel_t *channel, byte msb, byte lsb, byte cv_msb, byte cv_lsb, byte bit, bool flag) {
    channel->in.cmd = CHANNEL_CMD_CV_BIT;
    channel->in.msb = msb;
    channel->in.lsb = lsb;
    channel->in.cv_msb = cv_msb;
    channel->in.cv_lsb = cv_lsb;
    channel->in.bit = bit;
    channel->in.flag = flag;
    queue_add_blocking(&channel->qin, &channel->in);
}

void channel_cv29_bit5(channel_t *channel, byte msb, byte lsb, bool cv29_bit5) {
    channel->in.cmd = CHANNEL_CMD_CV29_BIT5;
    channel->in.msb = msb;
    channel->in.lsb = lsb;
    channel->in.cv29_bit5 = cv29_bit5;
    queue_add_blocking(&channel->qin, &channel->in);
}

void channel_laddr(channel_t *channel, byte msb, byte lsb, byte long_msb, byte long_lsb) {
    channel->in.cmd = CHANNEL_CMD_LADDR;
    channel->in.msb = msb;
    channel->in.lsb = lsb;
    channel->in.long_msb = long_msb;
    channel->in.long_lsb = long_lsb;
    queue_add_blocking(&channel->qin, &channel->in);
}

bool channel_try_remove_qin(channel_t *channel, channel_in_t *in) {
    return queue_try_remove(&channel->qin, in);
}

void channel_remove_blocking_qin(channel_t *channel, channel_in_t *in) {
    queue_remove_blocking(&channel->qin, in);
}

void channel_add_blocking_qout(channel_t *channel, channel_out_t *out) {
    queue_add_blocking(&channel->qout, out);
}


