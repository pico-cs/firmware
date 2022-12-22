#ifndef _LOOP_H
#define _LOOP_H

#include "common.h"
#include "prot.h"
#include "cmd.h"

void loop(cmd_t *cmd, reader_t *usb_reader, writer_t *usb_writer);

#endif