/*
 * tcp.h - Lo justo de TCP para zxv-link serve: escuchar en 127.0.0.1 y un
 * cliente, en Windows y en Linux o macOS.
 *
 * MIT, como el resto del SDK.
 */
#ifndef ZXV_TCP_H
#define ZXV_TCP_H

#include <stddef.h>
#include <stdint.h>

/* Escuchar en 127.0.0.1:port: el socket, o -1. */
intptr_t tcp_listen(unsigned port);

/* Un cliente que llega en timeout_ms: su socket, o -1. */
intptr_t tcp_accept(intptr_t listener, unsigned timeout_ms);

/* Lo que llegue en timeout_ms: cuántos bytes, 0 si nada, -1 si se ha ido. */
int tcp_recv(intptr_t s, char *buf, size_t n, unsigned timeout_ms);

/* Todo, o -1. */
int tcp_send(intptr_t s, const char *buf, size_t n);

void tcp_close(intptr_t s);

#endif /* ZXV_TCP_H */
