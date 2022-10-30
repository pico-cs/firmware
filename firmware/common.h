#ifndef _COMMON_H
#define _COMMON_H

#include "pico/stdlib.h"

typedef uint32_t word; // 32 bit words
typedef uint8_t  byte; // and let's have a byte as well 

// two byte addresses
#define MSB(addr) (addr >> 8) & 0x3f
#define LSB(addr) (addr & 0xff)
#define ADDR(msb, lsb) ((msb << 8) | lsb)

#endif
