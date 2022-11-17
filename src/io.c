#include "io.h"

const byte IO_PIN_INITIAL  = 0x00;
const byte IO_PIN_RESERVED = 0x01;
const byte IO_PIN_OUT      = 0x02;

static byte gp[IO_NUM] = {
    IO_PIN_RESERVED,              // GPIO0
    IO_PIN_RESERVED,              // GPIO1
    IO_PIN_RESERVED,              // GPIO2 (DCC output) // TODO configurable
    IO_PIN_RESERVED,              // GPIO3
    IO_PIN_RESERVED,              // GPIO4
    IO_PIN_RESERVED,              // GPIO5
    IO_PIN_INITIAL,               // GPIO6
    IO_PIN_INITIAL,               // GPIO7
    IO_PIN_INITIAL,               // GPIO8
    IO_PIN_INITIAL,               // GPIO9
    IO_PIN_INITIAL,               // GPIO10
    IO_PIN_INITIAL,               // GPIO11
    IO_PIN_INITIAL,               // GPIO12
    IO_PIN_INITIAL,               // GPIO13
    IO_PIN_INITIAL,               // GPIO14
    IO_PIN_INITIAL,               // GPIO15
    IO_PIN_RESERVED,              // GPIO16
    IO_PIN_RESERVED,              // GPIO17
    IO_PIN_RESERVED,              // GPIO18
    IO_PIN_RESERVED,              // GPIO19
    IO_PIN_RESERVED,              // GPIO20
    IO_PIN_RESERVED,              // GPIO21
    IO_PIN_RESERVED,              // GPIO22
    IO_PIN_RESERVED,              // GPIO23
    IO_PIN_RESERVED,              // GPIO24
    IO_PIN_RESERVED | IO_PIN_OUT, // GPIO25 (on board LED - pico only)
    IO_PIN_RESERVED,              // GPIO26
    IO_PIN_RESERVED,              // GPIO27
    IO_PIN_RESERVED,              // GPIO28
};

void io_init() {}

io_error_t io_init_out(uint no) {
    if (no >= IO_NUM) return IO_INV;
    if ((gp[no] & IO_PIN_RESERVED) != 0) return IO_RSRV;
    gp[no] = IO_PIN_OUT;
    gpio_init(no);
    gpio_set_dir(no, GPIO_OUT);
    return IO_OK;
}

io_error_t io_set_gp(uint no, bool v) {
    if (no >= IO_NUM) return IO_INV;
    if ((gp[no] & IO_PIN_OUT) == 0) return IO_NOOUT;
    gpio_put(no, v ? 1 : 0);
    return IO_OK;
}
