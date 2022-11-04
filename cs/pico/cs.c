#include "pico/multicore.h"
#include "pico/binary_info.h"
#include "hardware/adc.h"

#include "board.h"
#include "io.h"
#include "rbuf.h"
#include "cmd.h"
#include "exe.h"
#include "channel.h"

/*
    shared variables between cores
*/
board_t board;       // board
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

    bi_decl(bi_program_description("DCC command station."));
    bi_decl(bi_1pin_with_name(BOARD_GPIO_LED_PIN, "On-board LED")); // TODO: Pico W

    //bi_dect(bi)

    //bi_decl(bi_program_feature(0));
    //bi_decl(bi_program_feature_group(0 ,CMD_NOP, "nop"));

    // configure adc / enable temperature sensor
    adc_init();
    adc_set_temp_sensor_enabled(true);

    // first: init board
    if (!board_init(&board)) { // needs adc_init()
        return -1;
    }
           
    io_init();
    
    board_set_led(&board, true);

    stdio_init_all();

    channel_init(&channel);
    rbuf_init(&rbuf, &channel); // init mutex
        
    cmd_t cmd;
    cmd_init(&cmd, &board, &rbuf, &channel);
        
    multicore_launch_core1(core1_main);
    multicore_fifo_pop_blocking(); // wait for core1 to be started
    
    board_set_led(&board, false); // end init

    while (true) {
        cmd_dispatch(&cmd);
    }
}
