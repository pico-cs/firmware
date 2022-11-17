#ifndef _LOOP_H
#define _LOOP_H

#include "common.h"
#include "prot.h"
#include "cmd.h"

void loop(cmd_t *cmd, reader_t *reader_usb, writer_t *writer_usb);

#endif