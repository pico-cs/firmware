#ifndef _CFG_H
#define _CFG_H

#include "common.h"

// configuration variables
static const byte CFG_MT_CFG            = 0; 
static const byte CFG_MT_NUM_SYNC_BIT   = 1;
static const byte CFG_MT_NUM_REPEAT     = 2;
static const byte CFG_MT_NUM_REPEAT_CV  = 3;
static const byte CFG_MT_NUM_REPEAT_ACC = 4;
static const byte CFG_MT_BIDI_TS        = 5; // cutout start time after end bit
static const byte CFG_MT_BIDI_TE        = 6; // cutout stop time  before 5th sync bit

static const byte CFG_MT_CFG_LED  = 0x01;
static const byte CFG_MT_CFG_BIDI = 0x02;

#define CFG_NUM_CV 7

// public interface
void cfg_load();
bool cfg_store();
bool cfg_store_format();

bool cfg_get_mt_enabled();
bool cfg_set_mt_enabled(bool enabled);

byte cfg_get_cv(byte idx);
byte cfg_set_cv(byte idx, byte cv);

#endif
