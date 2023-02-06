#ifndef _FLASH_H
#define _FLASH_H

#include "common.h"

#include "hardware/flash.h"

static const uint32_t FLASH_TARGET_OFFSET   = PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;
static const uint8_t *FLASH_TARGET_CONTENTS = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
static const byte FLASH_FORMAT_VERSION[] = {0x00, 0x00, 0x00, 0x01};
static const size_t FLASH_FORMAT_VERSION_SIZE = sizeof(FLASH_FORMAT_VERSION);

typedef struct {
    size_t read_idx;
    size_t write_idx;
    size_t page_no;
    uint8_t data[FLASH_PAGE_SIZE];
} flash_t; // command

// public interface
bool flash_format();
bool flash_flush();
void flash_start_append();
void flash_reset();
flash_t *flash_object();

bool flash_read_byte(byte *b);
bool flash_write_byte(byte b);

#endif
