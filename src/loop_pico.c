#include "loop.h"

void loop(cmd_t *cmd, reader_t *reader_usb, writer_t *writer_usb) {

    byte usb_buf[PROT_BUFFER_SIZE];
    
    while (true) {

        int n = usb_read(usb_buf, PROT_BUFFER_SIZE, 10);
        
        if (reader_read_frame(reader_usb, usb_buf, n)) {
            cmd_dispatch(cmd, reader_usb, writer_usb);
            reader_reset(reader_usb);
        } 

        sleep_us(100);
    }
}
