#include "pico/multicore.h"
#include "pico/binary_info.h"
#include "hardware/adc.h"

#include "io.h"
#include "rbuf.h"
#include "cmd.h"
#include "exe.h"
#include "channel.h"

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

    bi_decl(bi_program_description("DCC command station."));
    bi_decl(bi_1pin_with_name(IO_LED_PIN, "On-board LED"));

    //bi_dect(bi)

    //bi_decl(bi_program_feature(0));
    //bi_decl(bi_program_feature_group(0 ,CMD_NOP, "nop"));

    io_init();
    io_set_led(true);

    stdio_init_all();

    // configure adc / enable temperature sensor
    adc_init();
    adc_set_temp_sensor_enabled(true);
    
    channel_init(&channel);
    rbuf_init(&rbuf, &channel); // init mutex
        
    cmd_t cmd;
    cmd_init(&cmd, &rbuf, &channel);
        
    multicore_launch_core1(core1_main);
    multicore_fifo_pop_blocking(); // wait for core1 to be started
    
    io_set_led(false); // end init

    while (true) {
        cmd_dispatch(&cmd);
    }
}
