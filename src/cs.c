#include "pico/multicore.h"
#include "pico/binary_info.h"
#include "hardware/watchdog.h"

#include "core.h"
#include "cfg.h"
#include "board.h"
#include "io.h"
#include "rbuf.h"
#include "mt.h"
#include "loop.h"

#define PROGRAM_VERSION     "v0.8.0"
#define PROGRAM_DESCRIPTION "pico-cs DCC command station"
#define PROGRAM_URL         "https://github.com/pico-cs"

/*
    shared variables between cores
*/
core_t  core;           // core (inter core communication)
board_t board;          // board (shared for led)
rbuf_t  rbuf;           // refresh buffer
cmdq_t  cmdq;           // command queue (multicore)
mt_t    mt;             // main track

void core1_main() {
    multicore_lockout_victim_init();    // prepare lockout (suspend) of core 1

    dcc_tx_pio_t tx_pio;
    dcc_tx_pio_init(&tx_pio);
    
    mt_init(&mt, &tx_pio, &board, &rbuf, &cmdq);

    core_signal_start1(&core);          // signal core0 that core1 is started

    while (!core_stop1(&core)) {        // loop until core0 sends stop notification
        //printf("loop core1 \n");
        mt_dispatch(&mt);
    }
}

static const uint32_t wait_ms = 200;

int main() {

    bi_decl(bi_program_version_string(PROGRAM_VERSION));
    bi_decl(bi_program_description(PROGRAM_DESCRIPTION));
    bi_decl(bi_program_url(PROGRAM_URL));
    
    // first guarantee that usb connection is up. 
    stdio_init_all();

    core_init(&core);

    reader_t usb_reader;
    reader_init(&usb_reader);
    writer_t usb_writer;
    writer_init(&usb_writer, NULL, &usb_write);

    // init io before board
    io_init();

    // then init board
    if (!board_init(&board, &usb_writer)) {
        return -1;
    }

    cmdq_init(&cmdq);
    
    rbuf_init(&rbuf, &cmdq);
        
    cmd_t cmd;
    cmd_init(&cmd, &board, &rbuf, &cmdq);
        
    multicore_launch_core1(core1_main);
    core_wait_start1(&core); // wait for core1 to be started

    // load configuration data from flash (after core1 is started)
    cfg_load();

    // after bootstrap set board led off. 
    board_set_led(&board, false);
    
    loop(&cmd, &usb_reader, &usb_writer); // finally loop until reboot

    // reboot
    core_wait_stop1(&core);     // wait for core1 to be stopped
    board_deinit(&board);
    sleep_ms(wait_ms);          // wait for write output
    watchdog_enable(0, true);   // reboot immediately
}
