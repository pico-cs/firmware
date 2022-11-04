#ifndef _IO_H
#define _IO_H

#include "common.h"

#define IO_NUM 29  // number of general purpose io (gpio0 - gpio28)

typedef enum { 
    IO_OK    = 0, // ok
    IO_INV   = 1, // invalid gpio number
    IO_RSRV  = 2, // reserved gpio number
    IO_NOOUT = 3, // is not GPIO_OUT
} io_error_t;

// public interface
void io_init();
io_error_t io_init_out(uint no);
io_error_t io_set_gp(uint no, bool v);

#endif