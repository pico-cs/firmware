#include "boards/pico.h"
#include "pico/multicore.h"
#include "pico/binary_info.h"

#include "prot.h"
#include "board.h"
#include "io.h"
#include "rbuf.h"
#include "cmd.h"
#include "exe.h"
#include "channel.h"
#include "loop.h"

#define PROGRAM_VERSION     "v0.1.10"
#define PROGRAM_DESCRIPTION "pico-cs DCC command station"
#define PROGRAM_URL         "https://github.com/pico-cs"

/*
    shared variables between cores
*/
rbuf_t rbuf;         // refresh buffer
channel_t channel;   // multicore channel

void core1_main() {
    exe_t exe;
    exe_init(&exe, &rbuf, &channel);

    multicore_fifo_push_blocking(0); // signal core0 that core1 is started
        
    exe_dispatch(&exe);
    
    // TODO wait for end
}

int main() {

    bi_decl(bi_program_version_string(PROGRAM_VERSION));
    bi_decl(bi_program_description(PROGRAM_DESCRIPTION));
    bi_decl(bi_program_url(PROGRAM_URL));
    
    // first guarantee that usb connection is up. 
    stdio_init_all();

    reader_t reader_usb;
    reader_init(&reader_usb);
    
    writer_t writer_usb;
    writer_init(&writer_usb, NULL, &usb_write);

    // then init board
    board_t board;
    if (!board_init(&board, &writer_usb)) {
        return -1;
    }
           
    io_init();
    
    board_set_led(&board, true); // start init

    channel_init(&channel);
    rbuf_init(&rbuf, &channel);
        
    cmd_t cmd;
    cmd_init(&cmd, &board, &rbuf, &channel);
        
    multicore_launch_core1(core1_main);
    multicore_fifo_pop_blocking(); // wait for core1 to be started
    
    board_set_led(&board, false); // end init

    loop(&cmd, &reader_usb, &writer_usb);

    board_deinit(&board);
}
