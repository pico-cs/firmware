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

int read_char_from_usb(uint32_t timeout_us) {
    return getchar_timeout_us(timeout_us);
}

int write_string_to_usb(const char *s) { 
    // return puts_raw(s); // cannot use - adds nl
    int cnt = 0;
    while (*s != 0) {
        cnt += putchar_raw(*s);
        s++;
    }
    return cnt;
};
int write_char_to_usb(int ch) { return putchar_raw(ch); }
