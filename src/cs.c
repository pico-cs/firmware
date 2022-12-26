#include "boards/pico.h"
#include "pico/multicore.h"
#include "pico/binary_info.h"

#include "prot.h"
#include "board.h"
#include "io.h"
#include "rbuf.h"
#include "cmd.h"
#include "mt.h"
#include "channel.h"
#include "loop.h"

#define PROGRAM_VERSION     "v0.6.0"
#define PROGRAM_DESCRIPTION "pico-cs DCC command station"
#define PROGRAM_URL         "https://github.com/pico-cs"

/*
    shared variables between cores
*/
rbuf_t rbuf;         // refresh buffer
cfgch_t cfgch;       // config  channel (multicore)
cmdch_t cmdch;       // command channel (multicore)

void core1_main() {
    mt_t mt;
    mt_init(&mt, &rbuf, &cfgch, &cmdch);

    multicore_fifo_push_blocking(0); // signal core0 that core1 is started
        
    mt_dispatch(&mt);
    
    // TODO wait for end
}

int main() {

    bi_decl(bi_program_version_string(PROGRAM_VERSION));
    bi_decl(bi_program_description(PROGRAM_DESCRIPTION));
    bi_decl(bi_program_url(PROGRAM_URL));
    
    // first guarantee that usb connection is up. 
    stdio_init_all();

    reader_t usb_reader;
    reader_init(&usb_reader);
    
    writer_t usb_writer;
    writer_init(&usb_writer, NULL, &usb_write);

    // init io before board
    io_init();

    // then init board
    board_t board;
    if (!board_init(&board, &usb_writer)) {
        return -1;
    }

    cfgch_init(&cfgch);
    cmdch_init(&cmdch);
    
    rbuf_init(&rbuf, &cmdch);
        
    cmd_t cmd;
    cmd_init(&cmd, &board, &rbuf, &cfgch, &cmdch);
        
    multicore_launch_core1(core1_main);
    multicore_fifo_pop_blocking(); // wait for core1 to be started
    
    // after bootstrap set board led to right value. 
    bool enabled = cfgch_get_enabled(&cfgch);
    board_set_led(&board, enabled);
    
    loop(&cmd, &usb_reader, &usb_writer);

    board_deinit(&board);
}
