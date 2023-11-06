#include "pico/cyw43_arch.h"

#include "tcp_server.h"

#define POLL_TIME_S 5

static err_t tcp_client_close(tcp_server_t *server) {
    write_event(server->logger, "tcp: client close");
    
    err_t err = ERR_OK;
    if (server->client_pcb != NULL) {
        tcp_arg(server->client_pcb, NULL);
        tcp_poll(server->client_pcb, NULL, 0);
        tcp_sent(server->client_pcb, NULL);
        tcp_recv(server->client_pcb, NULL);
        tcp_err(server->client_pcb, NULL);
        err = tcp_close(server->client_pcb);
        if (err != ERR_OK) {
            write_eventf(server->logger, "close failed %d, calling abort", err);
            tcp_abort(server->client_pcb);
            err = ERR_ABRT;
        }
        server->client_pcb = NULL;
    }
    return err;
}

static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    tcp_server_t *server = (tcp_server_t*)arg;
    // debug
    // write_eventf(server->logger, "tcp server sent %d", len);
    return ERR_OK;
}

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    tcp_server_t *server = (tcp_server_t*)arg;
    if (!p) {
        write_event(server->logger, "tcp: connection has been closed");
        return tcp_client_close(arg);
    }
    if (err != ERR_OK) {
        write_eventf(server->logger, "tcp: receive error %d", err);
        return tcp_client_close(arg);
    }
    cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
        // debug
        // write_eventf(server->logger, "tcp: server_recv %d/%d err %d", p->tot_len, server->recv_len, err);

        // Receive the buffer
        const uint16_t buffer_left = PROT_BUFFER_SIZE - server->recv_len;
        server->recv_len += pbuf_copy_partial(p, server->buffer_recv + server->recv_len,
                                             p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);

        // debug
        // write_eventf(server->logger, "tcp buffer %s", server->buffer_recv);
    }
    pbuf_free(p); //??? really free in case tot_len cannot be consumed
    return ERR_OK;
}

static err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb) {
    tcp_server_t *server = (tcp_server_t*)arg;
    write_event(server->logger, "tcp: server poll");
    return ERR_OK;
}

static void tcp_server_err(void *arg, err_t err) {
    tcp_server_t *server = (tcp_server_t*)arg;
    write_eventf(server->logger, "tcp: server error %d", err);
    if (err != ERR_ABRT) {
        tcp_server_close(server);
    }
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    tcp_server_t *server = (tcp_server_t*)arg;
    if (err != ERR_OK) {
        write_eventf(server->logger, "tcp: accept error %d", err);
        return err;
    }
    if (client_pcb == NULL) {
        write_eventf(server->logger, "tcp: accept client pcb is NULL");
        return ERR_VAL;
    }
    write_eventf(server->logger, "tcp: client connected");

    server->client_pcb = client_pcb;
    tcp_arg(client_pcb, server);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    //tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, tcp_server_err);
    return ERR_OK;
}

void tcp_server_init(tcp_server_t *server, writer_t *logger) {
    server->client_pcb = NULL;
    server->recv_len = 0;
    server->port = TCP_PORT;
    server->logger = logger;
    server->recv_len = 0;
    server->open = false;
}

bool tcp_server_open(tcp_server_t *server) {
    
    write_eventf(server->logger, "tcp: starting server host %s port %d", ip4addr_ntoa(netif_ip4_addr(netif_list)), server->port);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        write_event(server->logger, "tcp: failed to create pcb");
        return false;
    }

    err_t err = tcp_bind(pcb, NULL, server->port);
    if (err) {
        write_eventf(server->logger, "tcp: failed to bind to port %d", server->port);
        return false;
    }

    server->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!server->server_pcb) {
        write_event(server->logger, "tcp: failed to listen");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(server->server_pcb, server);
    tcp_accept(server->server_pcb, tcp_server_accept);

    server->open = true;
    return true;
}

int tcp_server_write(void *obj, const byte buf[], int size) {
    tcp_server_t *server = (tcp_server_t*)obj;

    if (!server->client_pcb) {
        write_event(server->logger, "tcp: no client connected");
        return 0;
    }

    // debug
    // write_eventf(server->logger, "tcp: writing %d bytes to client", size);
    cyw43_arch_lwip_begin();
    err_t err = tcp_write(server->client_pcb, buf, size, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        write_eventf(server->logger, "tcp: failed to write data %d", err);
        // TODO: by when do we close the connection ????;
    }
    tcp_output(server->client_pcb);
    cyw43_arch_lwip_end();
    // TODO: error or n - return ERR_OK;
    return size;
}

err_t tcp_server_close(tcp_server_t *server) {
    err_t err = tcp_client_close(server);
    write_event(server->logger, "tcp: server close");
    
    if (server->server_pcb) {
        tcp_arg(server->server_pcb, NULL);
        tcp_close(server->server_pcb);
        server->server_pcb = NULL;
    }
    server->open = false;
    return err;
}
