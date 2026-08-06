// File descriptor table and open/read/close operations (ADR-014).
// Component: fd
// Provides: fd_open, fd_close, fd_read
// Depends on: fs (fs_read), buddy (kfree), kernel.h
// Owns: kernel file descriptor table (16 slots)

#include "kernel.h"

struct file_desc {
    int in_use;
    uint32_t size;
    uint32_t offset;
    uint8_t *data;
};

static struct file_desc g_fd_table[16];

uint64_t fd_open(const char *path)
{
    if (!path) {
        return 0xFFFFFFFF;
    }
    // Skip leading slash if present to match fs_read expectation ("bin/...")
    if (path[0] == '/') {
        path++;
    }

    uint8_t *buf = NULL;
    uint32_t size = 0;
    if (fs_read(path, &buf, &size) < 0 || !buf) {
        return 0xFFFFFFFF;
    }

    for (int i = 0; i < 16; i++) {
        if (!g_fd_table[i].in_use) {
            g_fd_table[i].in_use = 1;
            g_fd_table[i].size = size;
            g_fd_table[i].offset = 0;
            g_fd_table[i].data = buf;
            return (uint64_t)(3 + i);
        }
    }

    // Table full, free allocated buffer
    kfree(buf);
    return 0xFFFFFFFF;
}

uint64_t fd_close(uint64_t fd)
{
    if (fd < 3 || fd >= 3 + 16) {
        return 0xFFFFFFFF;
    }
    int slot = (int)(fd - 3);
    if (!g_fd_table[slot].in_use) {
        return 0xFFFFFFFF;
    }
    if (g_fd_table[slot].data) {
        kfree(g_fd_table[slot].data);
    }
    g_fd_table[slot].in_use = 0;
    g_fd_table[slot].data = NULL;
    g_fd_table[slot].size = 0;
    g_fd_table[slot].offset = 0;
    return 0;
}

int64_t fd_read(uint64_t fd, void *buf, uint64_t len)
{
    if (fd < 3 || fd >= 3 + 16 || !buf) {
        return 0xFFFFFFFF;
    }
    int slot = (int)(fd - 3);
    if (!g_fd_table[slot].in_use) {
        return 0xFFFFFFFF;
    }
    struct file_desc *f = &g_fd_table[slot];
    if (f->offset >= f->size) {
        return 0;
    }
    uint64_t avail = (uint64_t)(f->size - f->offset);
    uint64_t to_read = len < avail ? len : avail;
    uint8_t *dest = (uint8_t *)buf;
    for (uint64_t i = 0; i < to_read; i++) {
        dest[i] = f->data[f->offset + i];
    }
    f->offset += (uint32_t)to_read;
    return (int64_t)to_read;
}
