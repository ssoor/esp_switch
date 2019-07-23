#ifndef _NET_H_
#define _NET_H_

#include "esp_common.h"

#define SOCKET_OK 0
#define SOCKET_ERROR -1
#define SOCKET_ADDRESS_INCORRECT -2

typedef void *net_conn_t;

net_conn_t net_dial_udp(const char *ip_addr, uint16 port);
net_conn_t net_dial_tcp(const char *ip_addr, uint16 port);

net_conn_t net_dial(const char *network, const char *address);
void net_close(net_conn_t conn);

int net_read(net_conn_t conn, void *data, uint32 lenght);
int net_write(net_conn_t conn, const void *data, uint32 lenght);

#endif