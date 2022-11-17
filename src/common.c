#include "common.h"

inline char hex_digit(uint8_t b) { return (char)(b < 10 ? b + '0' : b + 'a' - 10); }

void uint8_hex_to_string(uint8_t arr[], int size, char str[], char delim) {
    for (int i = 0; i < size; i++) {
        str[i*3]   = hex_digit(arr[i] >> 4);
        str[i*3+1] = hex_digit(arr[i] & 0x0f);
        str[i*3+2] = delim;
    }
    str[size*3-1] = 0;
}

int usb_read(byte buf[], int size, uint32_t timeout_us) {
    for (int i = 0; i < size; i++) {
        int ch = getchar_timeout_us(timeout_us);
        if (ch == PICO_ERROR_TIMEOUT) return i;
        buf[i] = ch;
    }
    return size;
}

int usb_write(void *obj, const byte buf[], int size) {
    for (int i = 0; i < size; i++) {
        putchar_raw(buf[i]);
    }
    return size;
}
