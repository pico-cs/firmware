#ifndef _COMMON_H
#define _COMMON_H

#include "pico/stdlib.h"

typedef uint32_t word; // 32 bit words
typedef uint8_t  byte; // and let's have a byte as well 

// two byte addresses
#define MSB(addr) (addr >> 8) & 0x3f
#define LSB(addr) (addr & 0xff)
#define ADDR(msb, lsb) ((msb << 8) | lsb)

// public interface
void uint8_hex_to_string(uint8_t arr[], int size, char str[], char delim);
int read_char_from_usb(uint32_t timeout_us); 
int write_string_to_usb(const char *s);
int write_char_to_usb(int ch);

#endif
