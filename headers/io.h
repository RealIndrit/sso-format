#ifndef IO_H
#define IO_H

#include <stdio.h>
static inline int io_read_exact(FILE *f, void *buf, size_t n) {
    return fread(buf, 1, n, f) != n;
}

static inline int io_write_exact(FILE *f, const void *buf, size_t n) {
    return fwrite(buf, 1, n, f) != n;
}

#endif
