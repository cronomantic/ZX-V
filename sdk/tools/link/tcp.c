/*
 * tcp.c - Lo justo de TCP para zxv-link serve (ver tcp.h).
 */
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET sock_t;
#define BAD_SOCK INVALID_SOCKET
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int sock_t;
#define BAD_SOCK (-1)
#define closesocket close
#endif

#include <string.h>

#include "tcp.h"

static int ready(sock_t s, unsigned timeout_ms)
{
    fd_set f;
    FD_ZERO(&f);
    FD_SET(s, &f);
    struct timeval t = { (long)(timeout_ms / 1000), (long)(timeout_ms % 1000) * 1000 };
    return select((int)s + 1, &f, NULL, NULL, &t);
}

intptr_t tcp_listen(unsigned port)
{
#ifdef _WIN32
    static int started;
    if (!started) {
        WSADATA w;
        if (WSAStartup(MAKEWORD(2, 2), &w))
            return -1;
        started = 1;
    }
#endif
    sock_t s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == BAD_SOCK)
        return -1;
    int one = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof one);
    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_port = htons((unsigned short)port);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(s, (struct sockaddr *)&a, sizeof a) || listen(s, 1)) {
        closesocket(s);
        return -1;
    }
    return (intptr_t)s;
}

intptr_t tcp_accept(intptr_t listener, unsigned timeout_ms)
{
    if (ready((sock_t)listener, timeout_ms) <= 0)
        return -1;
    sock_t c = accept((sock_t)listener, NULL, NULL);
    if (c == BAD_SOCK)
        return -1;
    int one = 1;
    setsockopt(c, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof one);
    return (intptr_t)c;
}

int tcp_recv(intptr_t s, char *buf, size_t n, unsigned timeout_ms)
{
    int r = ready((sock_t)s, timeout_ms);
    if (r < 0)
        return -1;
    if (r == 0)
        return 0;
    int got = (int)recv((sock_t)s, buf, (int)n, 0);
    return got > 0 ? got : -1;
}

int tcp_send(intptr_t s, const char *buf, size_t n)
{
    while (n) {
        int w = (int)send((sock_t)s, buf, (int)n, 0);
        if (w <= 0)
            return -1;
        buf += w;
        n -= (size_t)w;
    }
    return 0;
}

void tcp_close(intptr_t s)
{
    if (s >= 0)
        closesocket((sock_t)s);
}
