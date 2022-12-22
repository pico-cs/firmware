#include "loop.h"
#include "io.h"

void loop(cmd_t *cmd, reader_t *usb_reader, writer_t *usb_writer) {

    byte usb_buf[PROT_BUFFER_SIZE];
    
    while (true) {

        int n = usb_read(usb_buf, PROT_BUFFER_SIZE, 10);
        
        if (reader_read_frame(usb_reader, usb_buf, n)) {
            cmd_dispatch(cmd, usb_reader, usb_writer);
            reader_reset(usb_reader);
        }

        uint flags = io_get_gpio_flags();
        if (flags) {
            io_write_gpio_input_event(usb_writer, flags);
        }

        sleep_us(100);
    }
}
