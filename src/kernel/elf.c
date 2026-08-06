// ELF64 static loader — parse ET_EXEC, load PT_LOAD segments, return e_entry (ADR-016).
// Component: elf (ELF loader)
// Provides: elf_load
// Depends on: kernel.h (stdint, kprintf), buddy (kmalloc/kfree via fs_read buffer)
// Owns: (stateless — no persistent state)

#include "kernel.h"

#define ELF_MAGIC0  0x7F
#define ELF_MAGIC1  'E'
#define ELF_MAGIC2  'L'
#define ELF_MAGIC3  'F'
#define ELF_CLASS_64        2
#define ELF_DATA_LE         1
#define ELF_TYPE_EXEC       2
#define ELF_MACHINE_X86_64  0x3E
#define ELF_PT_LOAD         1

#define USER_REGION_START   0x200000ULL
#define USER_REGION_END     0x400000ULL

// Read u16 LE from base + offset
static inline uint16_t read_u16_le(const uint8_t *base, uint64_t offset)
{
    return (uint16_t)base[offset] | ((uint16_t)base[offset + 1] << 8);
}

// Read u32 LE from base + offset
static inline uint32_t read_u32_le(const uint8_t *base, uint64_t offset)
{
    return (uint32_t)base[offset] |
           ((uint32_t)base[offset + 1] << 8) |
           ((uint32_t)base[offset + 2] << 16) |
           ((uint32_t)base[offset + 3] << 24);
}

// Read u64 LE from base + offset
static inline uint64_t read_u64_le(const uint8_t *base, uint64_t offset)
{
    return (uint64_t)base[offset] |
           ((uint64_t)base[offset + 1] << 8) |
           ((uint64_t)base[offset + 2] << 16) |
           ((uint64_t)base[offset + 3] << 24) |
           ((uint64_t)base[offset + 4] << 32) |
           ((uint64_t)base[offset + 5] << 40) |
           ((uint64_t)base[offset + 6] << 48) |
           ((uint64_t)base[offset + 7] << 56);
}

int elf_load(const uint8_t *image, uint64_t size, uint64_t *entry_out)
{
    // Minimum ELF header size is 64 bytes (e_ident[16] + e_type[2] + e_machine[2] +
    // e_version[4] + e_entry[8] + e_phoff[8] + e_shoff[8] + e_flags[4] +
    // e_ehsize[2] + e_phentsize[2] + e_phnum[2] + e_shentsize[2] + e_shnum[2] +
    // e_shstrndx[2] = 64)
    if (size < 64) {
        return -1;
    }

    // Validate e_ident magic: 0x7F 'E' 'L' 'F'
    if (image[0] != ELF_MAGIC0 || image[1] != ELF_MAGIC1 ||
        image[2] != ELF_MAGIC2 || image[3] != ELF_MAGIC3) {
        return -1;
    }

    // Validate EI_CLASS == 2 (64-bit) at byte 4
    if (image[4] != ELF_CLASS_64) {
        return -1;
    }

    // Validate EI_DATA == 1 (LE) at byte 5
    if (image[5] != ELF_DATA_LE) {
        return -1;
    }

    // e_type at offset 16 (u16)
    uint16_t e_type = read_u16_le(image, 16);
    if (e_type != ELF_TYPE_EXEC) {
        return -1;
    }

    // e_machine at offset 18 (u16)
    uint16_t e_machine = read_u16_le(image, 18);
    if (e_machine != ELF_MACHINE_X86_64) {
        return -1;
    }

    // Program header table: e_phoff (u64 @32), e_phentsize (u16 @54), e_phnum (u16 @56)
    uint64_t e_phoff = read_u64_le(image, 32);
    uint16_t e_phentsize = read_u16_le(image, 54);
    uint16_t e_phnum = read_u16_le(image, 56);

    // Validate program header table fits in image
    if (e_phentsize < 56) {  // minimum PT_LOAD entry size we parse
        return -1;
    }
    if (size < e_phoff + (uint64_t)e_phnum * e_phentsize) {
        return -1;
    }

    // Entry point must land inside the user region too.
    uint64_t e_entry = read_u64_le(image, 24);
    if (e_entry < USER_REGION_START || e_entry >= USER_REGION_END) {
        return -1;
    }

    // Walk program headers
    for (uint16_t i = 0; i < e_phnum; i++) {
        uint64_t phdr_off = e_phoff + (uint64_t)i * e_phentsize;

        // p_type at offset 0 (u32)
        uint32_t p_type = read_u32_le(image, phdr_off);
        if (p_type != ELF_PT_LOAD) {
            continue;  // ignore non-PT_LOAD in v1
        }

        // p_offset (u64 @8), p_vaddr (u64 @16), p_filesz (u64 @32), p_memsz (u64 @40)
        uint64_t p_offset = read_u64_le(image, phdr_off + 8);
        uint64_t p_vaddr = read_u64_le(image, phdr_off + 16);
        uint64_t p_filesz = read_u64_le(image, phdr_off + 32);
        uint64_t p_memsz = read_u64_le(image, phdr_off + 40);

        // Validate p_vaddr and p_vaddr + p_memsz are within [0x200000, 0x400000)
        // (overflow-safe form: no u64 addition that could wrap the check).
        if (p_vaddr < USER_REGION_START || p_vaddr >= USER_REGION_END) {
            return -1;
        }
        if (p_memsz > USER_REGION_END - p_vaddr) {
            return -1;
        }
        // A file segment larger than its memory image would overrun the region.
        if (p_filesz > p_memsz) {
            return -1;
        }

        // Validate segment read doesn't go past image buffer
        if (p_offset > size || p_filesz > size - p_offset) {
            return -1;
        }

        // Copy p_filesz bytes from image + p_offset to p_vaddr
        const uint8_t *src = image + p_offset;
        uint8_t *dst = (uint8_t *)p_vaddr;
        for (uint64_t j = 0; j < p_filesz; j++) {
            dst[j] = src[j];
        }

        // Zero-fill BSS: from p_vaddr + p_filesz to p_vaddr + p_memsz
        for (uint64_t j = p_filesz; j < p_memsz; j++) {
            dst[j] = 0;
        }
    }

    *entry_out = e_entry;
    return 0;
}