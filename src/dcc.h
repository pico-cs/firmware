#ifndef _DCC_H
#define _DCC_H

#include "common.h"

// public interface
bool dcc_check_loco_addr(uint addr);
bool dcc_check_loco_speed128(uint speed128);
bool dcc_check_cv_idx(uint idx);
bool dcc_check_cv(byte cv);
bool dcc_check_bit(byte bit); 

bool dcc_check_acc_addr(uint addr);
bool dcc_check_acc_out(byte out);
bool dcc_check_acc_time(byte time);

#endif
