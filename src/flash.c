#include <string.h>

#include "pico/multicore.h"

#include "flash.h"

static const size_t FLASH_NUM_PAGE = FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE;
static const int FLASH_WRITE_RETRY = 2;

flash_t flash;

static inline bool flash_is_formatted() {
    return memcmp(FLASH_FORMAT_VERSION, FLASH_TARGET_CONTENTS, FLASH_FORMAT_VERSION_SIZE) == 0;
}

inline static bool flash_verify_page(size_t page_no) {
    return memcmp(flash.data, &FLASH_TARGET_CONTENTS[page_no * FLASH_PAGE_SIZE], FLASH_PAGE_SIZE) == 0;
}

inline static void flash_read_page(size_t page_no) {
    memcpy(flash.data, &FLASH_TARGET_CONTENTS[page_no * FLASH_PAGE_SIZE], FLASH_PAGE_SIZE);
    flash.page_no = page_no;
}

inline static uint32_t flash_range_start() {
    multicore_lockout_start_blocking();     // suspend core1
    return save_and_disable_interrupts();   // disable interrupts
}

inline static void flash_range_end(uint32_t ints) {
    restore_interrupts(ints);               // restore interrupts
    multicore_lockout_end_blocking();       // restart core1
}

inline static void flash_erase() {
    uint32_t ints = flash_range_start();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_end(ints);
}

inline static void flash_program(uint32_t offset) {
    uint32_t ints = flash_range_start();
    flash_range_program(offset, flash.data, FLASH_PAGE_SIZE);
    flash_range_end(ints);
}

static bool flash_write_page(size_t page_no) {
    for (int i = 0; i < FLASH_WRITE_RETRY; i++) {
        flash_program(FLASH_TARGET_OFFSET + (page_no * FLASH_PAGE_SIZE));
        if (flash_verify_page(page_no)) return true;
    }
    return false;
}

#include <stdio.h>

bool flash_format() {
    flash_erase();
    flash_read_page(0);
    memcpy(flash.data, FLASH_FORMAT_VERSION, FLASH_FORMAT_VERSION_SIZE);
    flash.read_idx = FLASH_FORMAT_VERSION_SIZE;
    flash.write_idx = FLASH_FORMAT_VERSION_SIZE;
    return flash_write_page(0);
}

static bool flash_check_space() {
    if (flash.write_idx < FLASH_PAGE_SIZE) return true;
    flash.write_idx = 0;
    if (!flash_write_page(flash.page_no)) return false;
    if (++flash.page_no > FLASH_NUM_PAGE) return false;
    flash_read_page(flash.page_no);
    return true;
}

/*
byte stuffing:
-  initial flash byte value: 0xff
-> replace 0xff by byte sequence 0xfe, 0xfe
-> replace 0xfe by byte sequence 0xfe, 0xfd
add 0xfe as stuffing byte and decrease orginal value by 1
*/

bool flash_read_byte(byte *b) {
    if (flash.read_idx >= FLASH_SECTOR_SIZE) return false;
    *b = FLASH_TARGET_CONTENTS[flash.read_idx];
    if (*b == 0xff) return false;
    flash.read_idx++;
    if (*b != 0xfe) return true;
    if (flash.read_idx >= FLASH_SECTOR_SIZE) return false;
    *b = FLASH_TARGET_CONTENTS[flash.read_idx++] + 1; // add one (see byte stuffing)
    return true;
}

void flash_init_read() {
    if (!flash_is_formatted()) flash_format();
    flash.read_idx = FLASH_FORMAT_VERSION_SIZE;
}

bool flash_write_byte(byte b) {
    if (!flash_check_space()) return false;
    if (b < 0xfe) {
        flash.data[flash.write_idx++] = b;
        return true;
    }
    flash.data[flash.write_idx++] = 0xfe;
    if (!flash_check_space()) return false;
    flash.data[flash.write_idx++] = b - 1; // subtract one (see byte stuffing)
}

bool flash_flush() {
    if (flash_verify_page(flash.page_no)) return true; // nothing to do
    return flash_write_page(flash.page_no);
}

void flash_start_append() { // start apend mode
    flash.page_no   = flash.read_idx / FLASH_PAGE_SIZE;
    flash.write_idx = flash.read_idx % FLASH_PAGE_SIZE;
    flash_read_page(flash.page_no);
}

void flash_reset() {
    if (!flash_is_formatted()) flash_format();
    flash.read_idx = FLASH_FORMAT_VERSION_SIZE;
}

flash_t *flash_object() {
    return &flash;
}
