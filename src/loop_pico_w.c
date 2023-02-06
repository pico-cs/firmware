#include "tcp_server.h"
#include "pico/cyw43_arch.h"

#include "loop.h"
#include "io.h"

void loop(cmd_t *cmd, reader_t *usb_reader, writer_t *usb_writer) {

    tcp_server_t server;
    tcp_server_init(&server, usb_writer);
    tcp_server_open(&server);

    writer_t tcp_writer;
    writer_init(&tcp_writer, &server, &tcp_server_write);
    reader_t tcp_reader;
    reader_init(&tcp_reader);

    byte usb_buf[PROT_BUFFER_SIZE];

    while (true) {

        server.recv_len = 0;
        
        cyw43_arch_poll();
        
        if (reader_read_frame(&tcp_reader, server.buffer_recv, server.recv_len)) {
            if (!cmd_dispatch(cmd, &tcp_reader, &tcp_writer)) break;
            reader_reset(&tcp_reader);
        }
        
        int n = usb_read(usb_buf, PROT_BUFFER_SIZE, 10);
        
        if (reader_read_frame(usb_reader, usb_buf, n)) {
            if (!cmd_dispatch(cmd, usb_reader, usb_writer)) break;
            reader_reset(usb_reader);
        }

        uint flags = io_get_gpio_flags();
        if (flags) {
            io_write_gpio_input_event(usb_writer, flags);
            if (server.client_pcb) io_write_gpio_input_event(&tcp_writer, flags);
        }

        //sleep_us(100);
    }
    tcp_server_close(&server);
}
