#include "loop.h"

#include "tcp_server.h"
#include "pico/cyw43_arch.h"

void loop(cmd_t *cmd, reader_t *reader_usb, writer_t *writer_usb) {

    tcp_server_t server;
    tcp_server_init(&server, writer_usb);
    tcp_server_open(&server);

    writer_t tcp_writer;
    writer_init(&tcp_writer, &server, &tcp_server_write);
    reader_t tcp_reader;
    reader_init(&tcp_reader);

    byte usb_buf[PROT_BUFFER_SIZE];

    while (true) {

        cyw43_arch_poll();
        
        if (reader_read_frame(&tcp_reader, server.buffer_recv, server.recv_len)) {
            cmd_dispatch(cmd, &tcp_reader, &tcp_writer);
            reader_reset(&tcp_reader);
            server.recv_len = 0; //TODO : ugly
        }
        
        int n = usb_read(usb_buf, PROT_BUFFER_SIZE, 10);
        
        if (reader_read_frame(reader_usb, usb_buf, n)) {
            cmd_dispatch(cmd, reader_usb, writer_usb);
            reader_reset(reader_usb);
        } 

        //sleep_us(100);
    }
}
