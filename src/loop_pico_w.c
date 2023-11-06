#include "tcp_server.h"
#include "pico/cyw43_arch.h"

#include "loop.h"
#include "io.h"

static bool is_wifi_connected(cmd_t *cmd, writer_t *logger, bool log_success) {
    switch (cyw43_wifi_link_status(&cyw43_state,CYW43_ITF_STA)) {
    case CYW43_LINK_DOWN:
        write_event(logger, "wifi: down"); return false;
    case CYW43_LINK_JOIN:
        if (log_success) write_event(logger, "wifi: connected");
        return true;
    case CYW43_LINK_FAIL:
        write_event(logger, "wifi: connection failed"); return false;
    case CYW43_LINK_NONET:
        write_event(logger, "wifi: no matching SSID found"); return false;
    case CYW43_LINK_BADAUTH:    
        write_event(logger, "wifi: authenticatation failure"); return false;
    default:
        write_event(logger, "wifi: unknown error"); return false;
    } 	
}

static const int num_connect_retry  = 5;
static const int connect_timeout_ms = 30000;    // 30 sec
static const int check_wifi_int_us  = 10000000; // 10 sec  

static bool wifi_connect(cmd_t *cmd, writer_t *logger) {
    write_eventf(logger, "wifi: connecting MAC: %s ...", cmd->board->mac);
    for (int i=0; i < num_connect_retry; i++) {
        cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, connect_timeout_ms);
        if (is_wifi_connected(cmd, logger, true)) return true;
    }
    return false;
}

void loop(cmd_t *cmd, reader_t *usb_reader, writer_t *usb_writer) {
    tcp_server_t server;
    tcp_server_init(&server, usb_writer);

    writer_t tcp_writer;
    writer_init(&tcp_writer, &server, &tcp_server_write);
    reader_t tcp_reader;
    reader_init(&tcp_reader);

    byte usb_buf[PROT_BUFFER_SIZE];

    if (wifi_connect(cmd, usb_writer)) tcp_server_open(&server);
    
    absolute_time_t time_from = get_absolute_time();
    
    while (true) {

        server.recv_len = 0;
        
        cyw43_arch_poll();

        if (absolute_time_diff_us(time_from, get_absolute_time()) >= check_wifi_int_us) {
            if (!is_wifi_connected(cmd, usb_writer, false)) {
                if (wifi_connect(cmd, usb_writer)) {
                    if (server.open) tcp_server_close(&server);
                    tcp_server_open(&server);
                };
            };
            time_from = get_absolute_time();
        }
        
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
