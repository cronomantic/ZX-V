/*
 * serial.c - Un puerto serie (ver serial.h).
 */
#ifndef _WIN32
#define _DEFAULT_SOURCE                 /* cfmakeraw */
#endif
#include "serial.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

#include <windows.h>

struct serial {
    HANDLE h;
};

serial_t *serial_open(const char *name, unsigned baud, int *err, char *why)
{
    char path[64];
    snprintf(path, sizeof path, "\\\\.\\%s", name);  /* COM10 y siguientes lo necesitan */
    HANDLE h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        DWORD e = GetLastError();
        *err = e == ERROR_FILE_NOT_FOUND || e == ERROR_PATH_NOT_FOUND ? SERIAL_E_NONE
             : e == ERROR_ACCESS_DENIED || e == ERROR_SHARING_VIOLATION ? SERIAL_E_BUSY : SERIAL_E_OTHER;
        snprintf(why, 128, "error %lu", (unsigned long)e);
        return NULL;
    }
    DCB dcb;
    memset(&dcb, 0, sizeof dcb);
    dcb.DCBlength = sizeof dcb;
    GetCommState(h, &dcb);
    dcb.BaudRate = baud;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fOutX = dcb.fInX = FALSE;
    dcb.fNull = FALSE;
    dcb.fAbortOnError = FALSE;
    if (!SetCommState(h, &dcb)) {
        *err = SERIAL_E_OTHER;
        snprintf(why, 128, "SetCommState: error %lu", (unsigned long)GetLastError());
        CloseHandle(h);
        return NULL;
    }
    SetupComm(h, 1 << 16, 1 << 16);
    PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);
    serial_t *s = malloc(sizeof *s);
    s->h = h;
    return s;
}

void serial_close(serial_t *s)
{
    if (s) {
        CloseHandle(s->h);
        free(s);
    }
}

int serial_read(serial_t *s, uint8_t *buf, size_t n, unsigned timeout_ms)
{
    /* En cuanto llega algo, vuelve; si no, a los timeout_ms. */
    COMMTIMEOUTS t = { MAXDWORD, MAXDWORD, timeout_ms ? timeout_ms : 1, 0, 0 };
    SetCommTimeouts(s->h, &t);
    DWORD got = 0;
    if (!ReadFile(s->h, buf, (DWORD)n, &got, NULL))
        return -1;
    return (int)got;
}

int serial_write(serial_t *s, const uint8_t *buf, size_t n)
{
    COMMTIMEOUTS t = { MAXDWORD, MAXDWORD, 1, 0, 2000 };
    SetCommTimeouts(s->h, &t);
    DWORD put = 0;
    if (!WriteFile(s->h, buf, (DWORD)n, &put, NULL) || put != n)
        return -1;
    return (int)put;
}

int serial_list(char *out, size_t n)
{
    int count = 0;
    size_t o = 0;
    out[0] = 0;
    char target[256];
    for (int i = 1; i <= 256; i++) {
        char name[16];
        snprintf(name, sizeof name, "COM%d", i);
        if (QueryDosDeviceA(name, target, sizeof target) && o + strlen(name) + 2 < n) {
            o += (size_t)snprintf(out + o, n - o, "%s\n", name);
            count++;
        }
    }
    return count;
}

#else

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

struct serial {
    int fd;
};

static speed_t speed_of(unsigned baud)
{
    switch (baud) {
    case 115200: return B115200;
    case 230400: return B230400;
#ifdef B460800
    case 460800: return B460800;
#endif
#ifdef B921600
    case 921600: return B921600;
#endif
    default: return 0;
    }
}

serial_t *serial_open(const char *name, unsigned baud, int *err, char *why)
{
    *err = SERIAL_E_OTHER;
    int fd = open(name, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        *err = errno == ENOENT || errno == ENXIO || errno == ENODEV ? SERIAL_E_NONE
             : errno == EBUSY ? SERIAL_E_BUSY : errno == EACCES || errno == EPERM ? SERIAL_E_DENIED : SERIAL_E_OTHER;
        snprintf(why, 128, "%s", strerror(errno));
        return NULL;
    }
    struct termios t;
    if (tcgetattr(fd, &t) < 0) {
        snprintf(why, 128, "%s", strerror(errno));
        close(fd);
        return NULL;
    }
    cfmakeraw(&t);
    t.c_cflag |= CLOCAL | CREAD;
    t.c_cflag &= ~(CSTOPB | PARENB);
#ifdef CRTSCTS
    t.c_cflag &= ~CRTSCTS;
#endif
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 0;
    speed_t sp = speed_of(baud);
    if (!sp || cfsetispeed(&t, sp) < 0 || cfsetospeed(&t, sp) < 0 || tcsetattr(fd, TCSANOW, &t) < 0) {
        snprintf(why, 128, "%u baud: %s", baud, sp ? strerror(errno) : "?");
        close(fd);
        return NULL;
    }
    tcflush(fd, TCIOFLUSH);
    serial_t *s = malloc(sizeof *s);
    s->fd = fd;
    return s;
}

void serial_close(serial_t *s)
{
    if (s) {
        close(s->fd);
        free(s);
    }
}

int serial_read(serial_t *s, uint8_t *buf, size_t n, unsigned timeout_ms)
{
    struct pollfd p = { s->fd, POLLIN, 0 };
    int r = poll(&p, 1, (int)timeout_ms);
    if (r < 0)
        return errno == EINTR ? 0 : -1;
    if (r == 0)
        return 0;
    ssize_t got = read(s->fd, buf, n);
    if (got < 0)
        return errno == EAGAIN ? 0 : -1;
    return (int)got;
}

int serial_write(serial_t *s, const uint8_t *buf, size_t n)
{
    size_t done = 0;
    while (done < n) {
        ssize_t w = write(s->fd, buf + done, n - done);
        if (w < 0) {
            if (errno != EAGAIN)
                return -1;
            struct pollfd p = { s->fd, POLLOUT, 0 };
            if (poll(&p, 1, 2000) <= 0)
                return -1;
            continue;
        }
        done += (size_t)w;
    }
    return (int)n;
}

int serial_list(char *out, size_t n)
{
    int count = 0;
    size_t o = 0;
    out[0] = 0;
    DIR *d = opendir("/dev");
    if (!d)
        return 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        const char *m = e->d_name;
        if (strncmp(m, "ttyUSB", 6) && strncmp(m, "ttyACM", 6) && strncmp(m, "cu.usbserial", 12) &&
            strncmp(m, "cu.usbmodem", 11))
            continue;
        if (o + strlen(m) + 7 < n) {
            o += (size_t)snprintf(out + o, n - o, "/dev/%s\n", m);
            count++;
        }
    }
    closedir(d);
    return count;
}

#endif
