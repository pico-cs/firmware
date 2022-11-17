#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

#include "common.h"
#include "prot.h"

typedef struct {
    uint16_t port;
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    byte buffer_recv[PROT_BUFFER_SIZE];
    int recv_len;
    writer_t *logger;
    bool open;
} tcp_server_t;

// public interface
void tcp_server_init(tcp_server_t *server, writer_t *logger);
bool tcp_server_open(tcp_server_t *server);
int tcp_server_write(void *obj, const byte buf[], int size);

#endif
