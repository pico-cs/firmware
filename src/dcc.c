#include "dcc.h"

static const int DCC_MAX_LOCO_ADDR     = 10239;
static const int DCC_MAX_ACC_ADDR      =  4048;
static const int DCC_MAX_LOCO_SPEED128 =   127;
static const int DCC_MAX_CV_DIRECT     =  1024;

inline bool dcc_check_loco_addr(uint addr) {
    return ((addr >=1) && (addr <= DCC_MAX_LOCO_ADDR));
};

inline bool dcc_check_loco_speed128(uint speed128) {
    return (speed128 <= DCC_MAX_LOCO_SPEED128);
};

inline bool dcc_check_cv_idx(uint idx) { // directly addressable cv
    return ((idx >=1) && (idx <= DCC_MAX_CV_DIRECT));
};

inline bool dcc_check_cv(byte cv) {  // directly addressable cv byte value
    return ((cv >=0) && (cv < 256)); // byte
};

inline bool dcc_check_bit(byte bit) { // directly addressable cv bit position
    return ((bit >=0) && (bit < 8)); // bit position 0-7
};

inline bool dcc_check_acc_addr(uint addr) {
    return ((addr >=0) && (addr < DCC_MAX_ACC_ADDR));
};

inline bool dcc_check_acc_out(byte out) {
    return ((out == 0) || (out == 1));
};

inline bool dcc_check_acc_time(byte time) {
    return (time <= 127);
};
