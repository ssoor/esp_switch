#include "esp_common.h"
#include "net.h"
#include <lwip/sockets.h>

typedef struct __internal_net_conn_t
{
    int fd;
    bool is_udp;
    struct sockaddr_in addr;
} _internal_net_conn_t;

net_conn_t net_dial_udp(const char *ip_addr, uint16 port)
{
    int err = SOCKET_OK;
    _internal_net_conn_t *conn = (_internal_net_conn_t *)malloc(sizeof(_internal_net_conn_t));

    do
    {
        conn->fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (SOCKET_ERROR == conn->fd)
        {
            err = SOCKET_ERROR;
            break;
        }

        conn->is_udp = true;
        conn->addr.sin_family = AF_INET;
        conn->addr.sin_port = htons(port);
        conn->addr.sin_addr.s_addr = inet_addr(ip_addr);
        if (0 == conn->addr.sin_addr.s_addr)
        {
            err = SOCKET_ADDRESS_INCORRECT;
            break;
        }
    } while (false);

    if (SOCKET_OK != err)
    {
        free(conn);
        conn = NULL;
    }

    return conn;
}

net_conn_t net_dial_tcp(const char *ip_addr, uint16 port)
{
    int err = SOCKET_OK;
    _internal_net_conn_t *conn = (_internal_net_conn_t *)malloc(sizeof(_internal_net_conn_t));

    do
    {
        conn->fd = socket(AF_INET, SOCK_STREAM, 0);
        if (SOCKET_ERROR == conn->fd)
        {
            err = SOCKET_ERROR;
            break;
        }

        conn->is_udp = false;
        conn->addr.sin_family = AF_INET;
        conn->addr.sin_port = htons(port);
        conn->addr.sin_addr.s_addr = inet_addr(ip_addr);
        if (0 == conn->addr.sin_addr.s_addr)
        {
            err = SOCKET_ADDRESS_INCORRECT;
            break;
        }

        err = connect(conn->fd, (struct sockaddr *)&conn->addr, sizeof(conn->addr));
    } while (false);

    if (SOCKET_OK != err)
    {
        free(conn);
        conn = NULL;
    }

    return conn;
}

int atoi(const char *str)
{
    int acc = 0;

    while (str[0])
    {
        if (str[0] < '0' || str[0] > '9')
        {
            return acc;
        }

        acc *= 10;
        acc += str[0] - '0';

        ++str;
    }

    return acc;
}

net_conn_t net_dial(const char *network, const char *address)
{
    char ip_addr[16];
    const char *port_ptr = strrchr(address, ':');
    if (NULL == port_ptr || port_ptr == address)
    {
        return NULL;
    }

    ip_addr[port_ptr - address] = '\0';
    memcpy(ip_addr, address, port_ptr - address);

    if (strcmp(network, "tcp"))
    {
        return net_dial_tcp(ip_addr, atoi(++port_ptr));
    }
    else if (strcmp(network, "udp"))
    {
        return net_dial_udp(ip_addr, atoi(++port_ptr));
    }

    return NULL;
}

int net_write(net_conn_t conn, const void *data, uint32 lenght)
{
    _internal_net_conn_t *_conn = conn;
    if (_conn->is_udp)
    {
        return sendto(_conn->fd, data, lenght, 0, (struct sockaddr *)&_conn->addr, sizeof(_conn->addr));
    }

    return send(_conn->fd, data, lenght, 0);
}

int net_read(net_conn_t conn, void *data, uint32 lenght)
{
    _internal_net_conn_t *_conn = conn;

    if (_conn->is_udp)
    {
        int addr_len = sizeof(_conn->addr);
        return recvfrom(_conn->fd, data, lenght, 0, (struct sockaddr *)&_conn->addr, &addr_len);
    }

    return recv(_conn->fd, data, lenght, 0);
}

void net_close(net_conn_t conn)
{
    _internal_net_conn_t *_conn = conn;

    close(_conn->fd);
    free(_conn);
}