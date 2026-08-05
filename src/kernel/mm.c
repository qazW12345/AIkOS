// Physical memory manager (ADR-012): E820 discovery + bitmap page allocator.
// One bit per 4 KiB page; static 4 KiB bitmap covers 128 MiB. Low memory
// [0, 1 MiB) reserved wholesale; kernel, page tables, and user blobs
// reserved explicitly. Returns 4 KiB-aligned physical addresses (identity
// mapped, so directly writable).

#include "kernel.h"

#define PMM_PAGE_SIZE   4096
#define PMM_BITMAP_PAGES 32768          /* 128 MiB / 4 KiB */

static uint8_t pmm_bitmap[PMM_BITMAP_PAGES / 8] __attribute__((aligned(4096)));
static uint32_t pmm_free_count;

struct e820_entry {                     /* int 15h AX=E820, ACPI 3.0 */
    uint64_t base, len;
    uint32_t type, attrs;
};

#define E820_COUNT (*(volatile uint16_t *)0x4FFC)
#define E820_TABLE ((struct e820_entry *)0x5000)
#define E820_TYPE_USABLE 1

static void pmm_mark(uint64_t page, int used)
{
    if (page >= PMM_BITMAP_PAGES)
        return;
    uint32_t idx = (uint32_t)(page / 8);
    uint8_t bit = (uint8_t)(1 << (page % 8));
    if (used)
        pmm_bitmap[idx] |= bit;
    else
        pmm_bitmap[idx] &= (uint8_t)~bit;
}

static int pmm_test(uint64_t page)
{
    return (pmm_bitmap[page / 8] >> (page % 8)) & 1;
}

void pmm_init(void)
{
    extern char _kernel_start[], _kernel_end[];
    int i;

    /* everything used, then free the E820 usable regions */
    for (i = 0; i < (int)sizeof(pmm_bitmap); i++)
        pmm_bitmap[i] = 0xFF;
    for (i = 0; i < E820_COUNT && i < 64; i++) {
        struct e820_entry *e = &E820_TABLE[i];
        uint64_t start, end, p;
        if (e->type != E820_TYPE_USABLE)
            continue;
        start = (e->base + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
        end = (e->base + e->len) / PMM_PAGE_SIZE;
        for (p = start; p < end; p++)
            pmm_mark(p, 0);
    }

    /* reserve: low memory, kernel, Phase 0 page tables + bitmap, user blobs */
    for (i = 0; i < 0x100000 / PMM_PAGE_SIZE; i++)
        pmm_mark((uint64_t)i, 1);
    for (uint64_t p = (uint64_t)_kernel_start / PMM_PAGE_SIZE;
         p <= (uint64_t)_kernel_end / PMM_PAGE_SIZE; p++)
        pmm_mark(p, 1);
    for (uint64_t p = 0x9000 / PMM_PAGE_SIZE; p < 0xD000 / PMM_PAGE_SIZE; p++)
        pmm_mark(p, 1);                 /* PML4/PDPT/PD + bitmap */
    for (uint64_t p = 0x200000 / PMM_PAGE_SIZE; p < 0x240000 / PMM_PAGE_SIZE; p++)
        pmm_mark(p, 1);                 /* user blobs */

    pmm_free_count = 0;
    for (i = 0; i < PMM_BITMAP_PAGES; i++)
        if (!pmm_test((uint64_t)i))
            pmm_free_count++;
}

void *pmm_alloc_page(void)
{
    int i;
    for (i = 0; i < PMM_BITMAP_PAGES; i++) {
        if (!pmm_test((uint64_t)i)) {
            pmm_mark((uint64_t)i, 1);
            pmm_free_count--;
            return (void *)((uint64_t)i * PMM_PAGE_SIZE);
        }
    }
    return 0;                           /* out of memory */
}

void pmm_free_page(void *page)
{
    uint64_t p = (uint64_t)page / PMM_PAGE_SIZE;
    if (page && pmm_test(p)) {
        pmm_mark(p, 0);
        pmm_free_count++;
    }
}
