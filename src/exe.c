#include <stdio.h>
#include "pico/binary_info.h"

#include "exe.h"

#include "dcc_tx.pio.h"

/*
TODO
- exe -> main mt / service st track
- configurtion of dcc repeats
- separation of config and command queue commands
- replace switch by function array (see below)
- remove blocking cs enable wait in case of track is disabled
- ...
---
typedef void (*configer)(exe_t *exe, mt_cfg_t *cfg);
typedef void (*commander)(exe_t *exe, mt_cmd_t *cmd);

static const configer cfg_fn[] = {
    exe_get_num_sync_bit,
    exe_set_num_sync_bit,
    exe_get_enabled,
    exe_set_enabled,
};

bool mt_dispatch(channel_t *channel, exe_t *exe) {
    mt_cfg_t cfg;
    mt_cmd_t cmd;
    
    // configuration
    if queue_try_remove(&channel->cfg_queue, &cfg) {
        // TODO check boundaries
        cfg_fn[cfg.cmd](exe, &cfg);
    }
    
    // command
    bool rv = queue_try_remove(&channel->cmd_queue, &cmd) {
        // TODO check boundaries
        cmd_fn[cmd.cmd](exe, &cmd);
    }
    return rv;
}
*/

static const uint EXE_DEFAULT_TX_PIN      =  2;
static const uint EXE_DEFAULT_TX_INV_PIN  =  EXE_DEFAULT_TX_PIN + 1;
static const uint EXE_DEFAULT_ENABLED_PIN = 22;
static const PIO EXE_DEFAULT_PIO = pio0;
static const uint EXE_DEFAULT_SM = 0;

static void exe_dcc(exe_t *exe, channel_in_t *in) {
    switch (in->cmd) {
    case CHANNEL_CMD_DIR_SPEED: dcc_dir_speed(&exe->dcc, in->msb, in->lsb, in->dir_speed);                                break;
    case CHANNEL_CMD_F0_4:      dcc_f0_4(&exe->dcc, in->msb, in->lsb, in->f0_4);                                          break;
    case CHANNEL_CMD_F5_8:      dcc_f5_8(&exe->dcc, in->msb, in->lsb, in->f5_8);                                          break;
    case CHANNEL_CMD_F9_12:     dcc_f9_12(&exe->dcc, in->msb, in->lsb, in->f9_12);                                        break;
    case CHANNEL_CMD_F13_20:    dcc_f13_20(&exe->dcc, in->msb, in->lsb, in->f13_20);                                      break;
    case CHANNEL_CMD_F21_28:    dcc_f21_28(&exe->dcc, in->msb, in->lsb, in->f21_28);                                      break;
    case CHANNEL_CMD_F29_36:    dcc_f29_36(&exe->dcc, in->msb, in->lsb, in->f29_36);                                      break;
    case CHANNEL_CMD_F37_44:    dcc_f37_44(&exe->dcc, in->msb, in->lsb, in->f37_44);                                      break;
    case CHANNEL_CMD_F45_52:    dcc_f45_52(&exe->dcc, in->msb, in->lsb, in->f45_52);                                      break;
    case CHANNEL_CMD_F53_60:    dcc_f53_60(&exe->dcc, in->msb, in->lsb, in->f53_60);                                      break;
    case CHANNEL_CMD_F61_68:    dcc_f61_68(&exe->dcc, in->msb, in->lsb, in->f61_68);                                      break;
    case CHANNEL_CMD_CV_BYTE:   dcc_cv_byte(&exe->dcc, in->msb, in->lsb, in->cv_msb, in->cv_lsb, in->cv);                 break;
    case CHANNEL_CMD_CV_BIT:    dcc_cv_bit(&exe->dcc, in->msb, in->lsb, in->cv_msb, in->cv_lsb, in->cv_bit, in->cv_flag); break;
    case CHANNEL_CMD_CV29_BIT5: dcc_cv29_bit5(&exe->dcc, in->msb, in->lsb, in->cv29_bit5);                                break;
    case CHANNEL_CMD_LADDR:     dcc_laddr(&exe->dcc, in->msb, in->lsb, in->long_msb, in->long_lsb);                       break;
    case CHANNEL_CMD_ACC:       dcc_acc(&exe->dcc, in->msb, in->lsb, in->acc_out, in->acc_flag);                          break;
    case CHANNEL_CMD_ACC_EXT:   dcc_acc_ext(&exe->dcc, in->msb, in->lsb, in->acc_status);                                 break;
    }
}

static void exe_refresh_dcc(exe_t *exe, rbuf_entry_t *entry) {
    // looks like that the dcc refresh is not supported by all decoders
    // dcc_refresh3(exe->dcc, exe->entry.msb, exe->entry.lsb, exe->entry.dir_speed, rbuf_slice_fcts(exe->entry.fcts, 0, 8));
    // so let's refresh by sending commands being supported by any decoder
    // in a round-robin way (dir_speed, f0_4, ...), so that 
    // - queue commands are handled quickly and
    // - no idle commands (some decoder do not work without pausing between commands) are needed,
    //   in case there is more than one entry in refresh buffer
    //
    if (entry->prev == entry->next) dcc_idle(&exe->dcc); // only one entry in buffer -> idle
    switch (entry->refresh_cycle) {
    case  0: dcc_dir_speed(&exe->dcc, entry->msb, entry->lsb, entry->dir_speed); break;
    case  1: dcc_f0_4(&exe->dcc, entry->msb, entry->lsb, entry->f0_4);           break;
    case  2: dcc_f5_8(&exe->dcc, entry->msb, entry->lsb, entry->f5_68.f5_8);     break;
    case  3: dcc_f9_12(&exe->dcc, entry->msb, entry->lsb, entry->f5_68.f9_12);   break;
    case  4: dcc_f13_20(&exe->dcc, entry->msb, entry->lsb, entry->f5_68.f13_20); break;
    case  5: dcc_f21_28(&exe->dcc, entry->msb, entry->lsb, entry->f5_68.f21_28); break;
    case  6: dcc_f29_36(&exe->dcc, entry->msb, entry->lsb, entry->f5_68.f29_36); break;
    case  7: dcc_f37_44(&exe->dcc, entry->msb, entry->lsb, entry->f5_68.f37_44); break;
    case  8: dcc_f45_52(&exe->dcc, entry->msb, entry->lsb, entry->f5_68.f45_52); break;
    case  9: dcc_f53_60(&exe->dcc, entry->msb, entry->lsb, entry->f5_68.f53_60); break;
    case 10: dcc_f61_68(&exe->dcc, entry->msb, entry->lsb, entry->f5_68.f61_68); break;
    default: dcc_idle(&exe->dcc);                                                break; // just in case - keep the dcc signal going
    }
}

static void exe_put(PIO pio, uint sm, word w) {
    dcc_tx_program_put(pio, sm, w);
}

static void exe_set_enabled(exe_t *exe, bool on) {
     exe->enabled = on;
     gpio_put(EXE_DEFAULT_ENABLED_PIN, on ? 1 : 0);
}

static void exe_config(exe_t *exe, channel_in_t *in) {
    channel_out_t out;
    
    switch (in->cmd) {

    case CHANNEL_CMD_GET_DCC_SYNC_BITS:
        out.dcc_sync_bits = dcc_get_sync_bits(&exe->dcc);
        channel_add_blocking_qout(exe->channel, &out);
        break;

    case CHANNEL_CMD_SET_DCC_SYNC_BITS:
        out.dcc_sync_bits = dcc_set_sync_bits(&exe->dcc, in->dcc_sync_bits);
        channel_add_blocking_qout(exe->channel, &out);
        break;

    case CHANNEL_CMD_GET_ENABLED:
        out.enabled = exe->enabled;
        channel_add_blocking_qout(exe->channel, &out);
        break;
    
    case CHANNEL_CMD_SET_ENABLED:
        exe_set_enabled(exe, in->enabled);
        break;
    
    }
}

static void exe_dispatch_disabled(exe_t *exe) {
    channel_in_t in;

    channel_remove_blocking_qin(exe->channel, &in);

    if ((in.cmd & CHANNEL_CFG) != 0) { // configuration command
        exe_config(exe, &in);
    }                                  // else: ignore dcc command
}

static void exe_dispatch_enabled(exe_t *exe) {
    rbuf_entry_t entry;
    channel_in_t in;

    bool dcc_command = false;
    if (channel_try_remove_qin(exe->channel, &in)) {
        if ((in.cmd & CHANNEL_CFG) != 0) { // configuration command
            exe_config(exe, &in);
        } else {                           // dcc command
            dcc_command = true;
        }
    }

    if (dcc_command) {
        exe_dcc(exe, &in);
    } else if (rbuf_try_get_next(exe->rbuf, &entry)) {
        exe_refresh_dcc(exe, &entry);
    } else {
        dcc_idle(&exe->dcc);
    }
}

void exe_init(exe_t *exe, rbuf_t *rbuf, channel_t *channel) {
    bi_decl(bi_1pin_with_name(EXE_DEFAULT_TX_PIN, "DCC signal output"));
    bi_decl(bi_1pin_with_name(EXE_DEFAULT_TX_INV_PIN, "DCC signal output (inverted)"));
    bi_decl(bi_1pin_with_name(EXE_DEFAULT_ENABLED_PIN, "DCC signal enabled"));

    gpio_init(EXE_DEFAULT_ENABLED_PIN);
    gpio_set_dir(EXE_DEFAULT_ENABLED_PIN, GPIO_OUT);

    exe_set_enabled(exe, false); // start not enabled
    exe->rbuf = rbuf;
    exe->channel = channel;

    uint offset = pio_add_program(EXE_DEFAULT_PIO, &dcc_tx_program);
    dcc_tx_program_init(EXE_DEFAULT_PIO, EXE_DEFAULT_SM, offset, EXE_DEFAULT_TX_PIN);
    dcc_tx_program_set_enabled(EXE_DEFAULT_PIO, EXE_DEFAULT_SM, true);

    dcc_init(&exe->dcc, exe_put, EXE_DEFAULT_PIO, EXE_DEFAULT_SM);
}

void exe_dispatch(exe_t *exe) {
    while (true) {
        if (exe->enabled) {
            exe_dispatch_enabled(exe);
         } else {
            //TODO: service track
            //printf("tx queue entries %d\n", pio_sm_get_tx_fifo_level(pio0, 0));
            exe_dispatch_disabled(exe);
        }
    }
}
