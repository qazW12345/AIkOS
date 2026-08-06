// AIkFS driver — read-only filesystem (ADR-015).
// Component: fs (AIkFS driver)
// Provides: fs_init, fs_ls, fs_read, fs_info
// Depends on: buddy (kmalloc/kfree), kernel.h (stdint, kprintf)
// Owns: superblock state (magic, version, block_count, root_dir_block, bin_dir_block); initialized flag
// Single-CPU, read-only, RAM-backed at 0x400000 (boot copies partition there).

#include "kernel.h"

#define AIKFS_BASE_ADDR     0x400000ULL
#define AIKFS_BLOCK_SIZE    512
#define AIKFS_MAGIC         "AIkFS1"
#define AIKFS_MAGIC_LEN     6
#define AIKFS_VERSION       1

// Superblock offsets
#define SB_MAGIC_OFFSET     0
#define SB_VERSION_OFFSET   6
#define SB_BLOCK_COUNT_OFFSET 8
#define SB_BITMAP_START_OFFSET 12
#define SB_BITMAP_BLOCKS_OFFSET 16
#define SB_ROOT_DIR_BLOCK_OFFSET 20

// Directory entry layout (32 bytes)
#define DIR_ENTRY_SIZE      32
#define DIR_NAME_OFFSET     0
#define DIR_NAME_LEN        16
#define DIR_TYPE_OFFSET     16
#define DIR_SIZE_OFFSET     17
#define DIR_FIRST_BLOCK_OFFSET 21
#define DIR_BLOCK_COUNT_OFFSET 25

#define DIR_TYPE_FILE       1
#define DIR_TYPE_DIR        2

// Global state
static uint32_t g_block_count = 0;
static uint32_t g_root_dir_block = 0;
static uint32_t g_bin_dir_block = 0;
static int g_fs_initialized = 0;

// Read a u32 LE from memory at base + offset
static inline uint32_t read_u32_le(const uint8_t *base, uint32_t offset)
{
    return (uint32_t)base[offset] |
           ((uint32_t)base[offset + 1] << 8) |
           ((uint32_t)base[offset + 2] << 16) |
           ((uint32_t)base[offset + 3] << 24);
}

// Get pointer to a block in the ramdisk
static inline const uint8_t *block_ptr(uint32_t block_idx)
{
    return (const uint8_t *)(AIKFS_BASE_ADDR + block_idx * AIKFS_BLOCK_SIZE);
}

// Find a directory entry by name in a directory block
// Returns 1 on success, fills out type, size, first_block, block_count
// Returns 0 if not found
static int find_dir_entry(const uint8_t *dir_block, const char *name,
                          uint8_t *out_type, uint32_t *out_size,
                          uint32_t *out_first_block, uint32_t *out_block_count)
{
    const uint8_t *entry = dir_block;
    for (int i = 0; i < AIKFS_BLOCK_SIZE / DIR_ENTRY_SIZE; i++) {
        if (entry[DIR_NAME_OFFSET] == 0) {
            // Empty slot — end of used entries
            break;
        }
        // Check name match (NUL-padded, so compare up to first NUL or 16 chars)
        int match = 1;
        for (int j = 0; j < DIR_NAME_LEN; j++) {
            char c1 = entry[DIR_NAME_OFFSET + j];
            char c2 = name[j];
            if (c1 == 0 && c2 == 0) {
                break; // both NUL, match so far
            }
            if (c1 != c2) {
                match = 0;
                break;
            }
        }
        if (match) {
            *out_type = entry[DIR_TYPE_OFFSET];
            *out_size = read_u32_le(entry, DIR_SIZE_OFFSET);
            *out_first_block = read_u32_le(entry, DIR_FIRST_BLOCK_OFFSET);
            *out_block_count = read_u32_le(entry, DIR_BLOCK_COUNT_OFFSET);
            return 1;
        }
        entry += DIR_ENTRY_SIZE;
    }
    return 0;
}

// List all entries in a directory block with a prefix
static void list_dir(const uint8_t *dir_block, const char *prefix)
{
    const uint8_t *entry = dir_block;
    for (int i = 0; i < AIKFS_BLOCK_SIZE / DIR_ENTRY_SIZE; i++) {
        if (entry[DIR_NAME_OFFSET] == 0) {
            break;
        }
        // Extract name (NUL-terminated within 16 bytes)
        char name[DIR_NAME_LEN + 1];
        int name_len = 0;
        for (int j = 0; j < DIR_NAME_LEN; j++) {
            if (entry[DIR_NAME_OFFSET + j] == 0) break;
            name[name_len++] = entry[DIR_NAME_OFFSET + j];
        }
        name[name_len] = '\0';
        uint32_t size = read_u32_le(entry, DIR_SIZE_OFFSET);
        kprintf("%s%s %u\r\n", prefix, name, size);
        entry += DIR_ENTRY_SIZE;
    }
}

void fs_init(void)
{
    const uint8_t *superblock = block_ptr(0);

    // Validate magic
    int magic_ok = 1;
    for (int i = 0; i < AIKFS_MAGIC_LEN; i++) {
        if (superblock[SB_MAGIC_OFFSET + i] != AIKFS_MAGIC[i]) {
            magic_ok = 0;
            break;
        }
    }

    // Validate version
    uint8_t version = superblock[SB_VERSION_OFFSET];
    if (!magic_ok || version != AIKFS_VERSION) {
        kprintf("fs_init: invalid superblock (magic=%.*s version=%u)\r\n",
                AIKFS_MAGIC_LEN, (const char *)superblock, version);
        g_fs_initialized = 0;
        return;
    }

    g_block_count = read_u32_le(superblock, SB_BLOCK_COUNT_OFFSET);
    // bitmap_start = read_u32_le(superblock, SB_BITMAP_START_OFFSET); // always 1
    // bitmap_blocks = read_u32_le(superblock, SB_BITMAP_BLOCKS_OFFSET); // always 1
    g_root_dir_block = read_u32_le(superblock, SB_ROOT_DIR_BLOCK_OFFSET);
    g_bin_dir_block = 3; // Fixed per format: block 3 is the bin directory

    g_fs_initialized = 1;
    kprintf("fs_init: AIkFS1 v%u blocks=%u root_dir_block=%u bin_dir_block=%u\r\n",
            version, g_block_count, g_root_dir_block, g_bin_dir_block);
}

void fs_info(void)
{
    if (!g_fs_initialized) {
        kprintf("fs not initialized\r\n");
        return;
    }

    // Count files in bin directory
    const uint8_t *bin_dir = block_ptr(g_bin_dir_block);
    int file_count = 0;
    const uint8_t *entry = bin_dir;
    for (int i = 0; i < AIKFS_BLOCK_SIZE / DIR_ENTRY_SIZE; i++) {
        if (entry[DIR_NAME_OFFSET] == 0) break;
        if (entry[DIR_TYPE_OFFSET] == DIR_TYPE_FILE) file_count++;
        entry += DIR_ENTRY_SIZE;
    }

    kprintf("AIkFS1 v%u blocks=%u files=%d\r\n", AIKFS_VERSION, g_block_count, file_count);
}

void fs_ls(void)
{
    if (!g_fs_initialized) {
        kprintf("fs not initialized\r\n");
        return;
    }

    const uint8_t *root_dir = block_ptr(g_root_dir_block);
    const uint8_t *bin_dir = block_ptr(g_bin_dir_block);

    kprintf("root:\r\n");
    list_dir(root_dir, "");

    kprintf("bin:\r\n");
    list_dir(bin_dir, "bin/");
}

int fs_read(const char *name, uint8_t **out, uint32_t *out_size)
{
    if (!g_fs_initialized) {
        return -1;
    }

    // Parse path: must be "bin/xxx"
    if (name[0] != 'b' || name[1] != 'i' || name[2] != 'n' || name[3] != '/') {
        return -1;
    }
    const char *filename = name + 4; // Skip "bin/"

    if (filename[0] == '\0') {
        return -1;
    }

    const uint8_t *bin_dir = block_ptr(g_bin_dir_block);

    uint8_t type;
    uint32_t size, first_block, block_count;

    if (!find_dir_entry(bin_dir, filename, &type, &size, &first_block, &block_count)) {
        return -1;
    }

    if (type != DIR_TYPE_FILE) {
        return -1;
    }

    // Allocate buffer
    uint8_t *buf = (uint8_t *)kmalloc(size);
    if (!buf) {
        return -1;
    }

    // Copy data from contiguous blocks
    uint32_t bytes_copied = 0;
    for (uint32_t b = 0; b < block_count; b++) {
        const uint8_t *src = block_ptr(first_block + b);
        uint32_t to_copy = AIKFS_BLOCK_SIZE;
        if (bytes_copied + to_copy > size) {
            to_copy = size - bytes_copied;
        }
        for (uint32_t i = 0; i < to_copy; i++) {
            buf[bytes_copied + i] = src[i];
        }
        bytes_copied += to_copy;
        if (bytes_copied >= size) break;
    }

    *out = buf;
    *out_size = size;
    return 0;
}