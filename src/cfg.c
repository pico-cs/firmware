#include "pico/mutex.h"

#include "cfg.h"
#include "flash.h"
#include "prot.h"

typedef struct {
    const byte min;
    const byte max;
    const byte def;
} cfg_cv_t;
 
static const cfg_cv_t cfg_cv[CFG_NUM_CV] = {
	{  0,   3,  CFG_MT_CFG_LED}, // CFG_MT_CFG
	{ 17,  32,  17},             // CFG_MT_NUM_SYNC_BIT
	{  1,   5,   1},             // CFG_MT_NUM_REPEAT
	{  2,   5,   2},             // CFG_MT_NUM_REPEAT_CV - command needs to be repeated twice to be accepted by the decoder
	{  1,   5,   2},             // CFG_MT_NUM_REPEAT_ACC
	{ 26,  32,  26},             // CFG_MT_BIDI_TS
	{ 10,  16,  12},             // CFG_MT_BIDI_TE
};

static byte cfg_cv_check_range(byte cv, byte min, byte max) {
    if (cv < min) return min;
    if (cv > max) return max;
    return cv;
}

struct {
    writer_t *writer;
    mutex_t mu;
    volatile bool mt_enabled; // main track enabled
    volatile byte read_cv[CFG_NUM_CV];
    volatile byte write_cv[CFG_NUM_CV];
} cfg; // configuration singleton

static inline void cfg_lock()   { mutex_enter_blocking(&cfg.mu); }
static inline void cfg_unlock() { mutex_exit(&cfg.mu);           }

static void cfg_init_cv_def() {
    // init with default values
    for (byte i = 0; i < CFG_NUM_CV; i++) {
        cfg.read_cv[i]  = cfg_cv[i].def;
        cfg.write_cv[i] = cfg_cv[i].def;
    }
}

static bool cfg_load_cv() {
    byte idx;
    byte cv;
    while (flash_read_byte(&idx)) {
        if (idx >= CFG_NUM_CV) return false; // invalid cv index
        if (!flash_read_byte(&cv)) return false;
        cv = cfg_cv_check_range(cv, cfg_cv[idx].min, cfg_cv[idx].max);
        cfg.read_cv[idx]  = cv;
        cfg.write_cv[idx] = cv;
    }
}

static bool cfg_store_cv_delta_read() {
    for (byte i = 0; i < CFG_NUM_CV; i++) {
        if (cfg.write_cv[i] != cfg.read_cv[i]) {
            if (!flash_write_byte(i)) return false;
            if (!flash_write_byte(cfg.write_cv[i])) return false;
            cfg.read_cv[i] = cfg.write_cv[i];
        }
    }
    return flash_flush();
}

static bool cfg_store_cv_delta_default() {
    for (byte i = 0; i < CFG_NUM_CV; i++) {
        if (cfg.write_cv[i] != cfg_cv[i].def) {
            if (!flash_write_byte(i)) return false;
            if (!flash_write_byte(cfg.write_cv[i])) return false;
            cfg.read_cv[i] = cfg.write_cv[i];
        }
    }
    return flash_flush();
}

void cfg_load() {
    mutex_init(&cfg.mu);
    cfg_init_cv_def();
    flash_reset();
    cfg_load_cv();
    flash_start_append();
}

bool cfg_store_format() {
    if (!flash_format()) return false;
    return cfg_store_cv_delta_default();
}

bool cfg_store() {
    if (cfg_store_cv_delta_read()) return true;
    // no space
    return cfg_store_format();
}

inline bool cfg_get_mt_enabled() {
    cfg_lock();
    bool enabled = cfg.mt_enabled;
    cfg_unlock();
    return enabled;
}

inline bool cfg_set_mt_enabled(bool enabled) {
    cfg_lock();
    cfg.mt_enabled = enabled;
    cfg_unlock();
    return enabled;
}

inline byte cfg_get_cv(byte idx) {
    if (idx >= CFG_NUM_CV) return 0;
    cfg_lock();
    byte out_cv = cfg.write_cv[idx];
    cfg_unlock();
    return out_cv;
}

inline byte cfg_set_cv(byte idx, byte in_cv) {
    if (idx >= CFG_NUM_CV) return 0;
    cfg_lock();
    byte out_cv = cfg_cv_check_range(in_cv, cfg_cv[idx].min, cfg_cv[idx].max);
    cfg.write_cv[idx] = out_cv;
    cfg_unlock();
    return out_cv;
}
