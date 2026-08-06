// Binary buddy allocator + kernel heap (ADR-017).
// Component: buddy (kernel heap, ADR-017)
// Provides: buddy_init, kmalloc, kfree, heap_stats
// Depends on: mm (pmm_alloc_contiguous), kernel.h (stdint, kprintf)
// Owns: free_lists[11] (orders 0..10); allocated_blocks counter; pool base tracking
// Single-CPU, no locks (interrupt handlers never allocate).

#include "kernel.h"

#define BUDDY_MAGIC_FREE   0x42756479  // "BudY"
#define BUDDY_MAGIC_ALLOC  0x414C4F43  // "ALOC"
#define BUDDY_HEADER_SIZE  16
#define BUDDY_MAX_ORDER    10
#define BUDDY_PAGE_SIZE    4096

struct buddy_header {
    uint64_t free_next;  // valid only when free (BudY), else 0
    uint32_t magic;      // BUDDY_MAGIC_FREE or BUDDY_MAGIC_ALLOC
    uint32_t order;      // 0..10
};

static void *free_lists[BUDDY_MAX_ORDER + 1];
static uint64_t allocated_blocks;
static void *pool_base;
static void *pool_limit;  // one past the last byte of buddy-managed pool

static void buddy_list_push(int order, void *block)
{
    struct buddy_header *h = (struct buddy_header *)block;
    h->free_next = (uint64_t)free_lists[order];
    h->magic = BUDDY_MAGIC_FREE;
    h->order = (uint32_t)order;
    free_lists[order] = block;
}

static void *buddy_list_pop(int order)
{
    void *block = free_lists[order];
    if (block) {
        struct buddy_header *h = (struct buddy_header *)block;
        free_lists[order] = (void *)h->free_next;
        h->free_next = 0;
    }
    return block;
}

static int buddy_list_unlink(int order, void *block)
{
    void **prev = &free_lists[order];
    void *cur = free_lists[order];
    while (cur) {
        if (cur == block) {
            struct buddy_header *h = (struct buddy_header *)cur;
            *prev = (void *)h->free_next;
            h->free_next = 0;
            return 1;
        }
        struct buddy_header *h = (struct buddy_header *)cur;
        prev = (void **)&h->free_next;
        cur = (void *)h->free_next;
    }
    return 0;
}

void buddy_init(void)
{
    for (int i = 0; i <= BUDDY_MAX_ORDER; i++)
        free_lists[i] = 0;
    allocated_blocks = 0;
    pool_base = 0;
    pool_limit = 0;
}

static void *buddy_get_block(int order)
{
    // Try to find a free block at this order or higher
    for (int j = order; j <= BUDDY_MAX_ORDER; j++) {
        void *block = buddy_list_pop(j);
        if (block) {
            // Split down to the requested order
            while (j > order) {
                j--;
                uint64_t buddy_addr = (uint64_t)block + (1ULL << (j + 12));  // 2^j pages = 2^(j+12) bytes
                buddy_list_push(j, (void *)buddy_addr);
                // The original block stays as the lower half
            }
            return block;
        }
    }

    // No block available, allocate from pmm.
    // CRITICAL: the pulled run must be aligned to its order, or split-partners
    // (block + 2^j) and XOR-buddies disagree and coalescing never merges them
    // (silent permanent fragmentation). Pull slack pages, align the base up,
    // and decompose the head/tail slack into properly-aligned free blocks.
    uint32_t pages = 1U << order;
    uint64_t align_bytes = (uint64_t)pages << 12;         // order block size
    uint32_t slack_pages = (uint32_t)((align_bytes >> 12) - 1);  // < pages
    void *block = pmm_alloc_contiguous(pages + slack_pages);
    if (!block)
        return 0;

    uint64_t base = ((uint64_t)block + align_bytes - 1) & ~(align_bytes - 1);
    uint64_t head_pages = (base - (uint64_t)block) >> 12;
    uint64_t tail_pages = slack_pages - head_pages;
    uint64_t rem;
    uint64_t p;

    // Head slack: [block, base). Lay pieces out from the aligned base DOWN,
    // highest bit first, so every piece is aligned to its own order.
    rem = head_pages;
    p = base;
    for (int bit = BUDDY_MAX_ORDER - 1; bit >= 0; bit--) {
        if (rem & (1ULL << bit)) {
            p -= (1ULL << bit) << 12;
            buddy_list_push(bit, (void *)p);
        }
    }
    // Tail slack: [base + order_size, end of pull) — from the aligned end UP,
    // highest bit first.
    rem = tail_pages;
    p = base + align_bytes;
    for (int bit = BUDDY_MAX_ORDER - 1; bit >= 0; bit--) {
        if (rem & (1ULL << bit)) {
            buddy_list_push(bit, (void *)p);
            p += (1ULL << bit) << 12;
        }
    }
    block = (void *)base;

    // Track pool extents for coalescing boundary checks
    if (!pool_base || block < pool_base)
        pool_base = block;
    uint64_t block_end = (uint64_t)block + (pages * BUDDY_PAGE_SIZE);
    if (!pool_limit || block_end > (uint64_t)pool_limit)
        pool_limit = (void *)block_end;

    buddy_list_push(order, block);
    return buddy_list_pop(order);
}

void *kmalloc(size_t size)
{
    if (size == 0)
        return 0;

    size_t total = size + BUDDY_HEADER_SIZE;
    uint32_t pages = (total + BUDDY_PAGE_SIZE - 1) / BUDDY_PAGE_SIZE;

    if (pages > 1024)  // 1024 pages = 4 MiB = order 10
        return 0;

    // order = bit_length(pages - 1), i.e., smallest k with 2^k >= pages
    int order = 0;
    if (pages > 1) {
        uint32_t v = pages - 1;
        while (v) {
            v >>= 1;
            order++;
        }
    }
    if (order > BUDDY_MAX_ORDER)
        order = BUDDY_MAX_ORDER;

    void *block = buddy_get_block(order);
    if (!block)
        return 0;

    struct buddy_header *h = (struct buddy_header *)block;
    h->magic = BUDDY_MAGIC_ALLOC;
    h->order = (uint32_t)order;
    h->free_next = 0;

    allocated_blocks++;
    return (char *)block + BUDDY_HEADER_SIZE;
}

void kfree(void *ptr)
{
    if (!ptr)
        return;

    struct buddy_header *h = (struct buddy_header *)((char *)ptr - BUDDY_HEADER_SIZE);

    if (h->magic != BUDDY_MAGIC_ALLOC) {
        kprintf("kfree: bad magic\r\n");
        return;
    }

    int order = (int)h->order;
    if (order < 0 || order > BUDDY_MAX_ORDER) {
        kprintf("kfree: bad order\r\n");
        return;
    }

    allocated_blocks--;

    // Coalesce first (h is not in any list yet); then push the merged block
    // once, at its final order. Pushing before the loop would leak or
    // mis-list merged blocks.
    while (order < BUDDY_MAX_ORDER) {
        uint64_t block_addr = (uint64_t)h;
        uint64_t buddy_addr = block_addr ^ (1ULL << (order + 12));

        // Buddy must be inside the buddy-managed pool...
        if (buddy_addr < (uint64_t)pool_base || buddy_addr >= (uint64_t)pool_limit)
            break;

        struct buddy_header *buddy_h = (struct buddy_header *)buddy_addr;
        // ...free, and at the same order.
        if (buddy_h->magic != BUDDY_MAGIC_FREE || buddy_h->order != (uint32_t)order)
            break;

        if (!buddy_list_unlink(order, buddy_h))
            break;  // buddy not found in list (shouldn't happen, but safety)

        // The merged block is the lower-addressed one.
        if (buddy_addr < block_addr)
            h = buddy_h;
        order++;
    }

    h->magic = BUDDY_MAGIC_FREE;
    h->order = (uint32_t)order;
    h->free_next = 0;
    buddy_list_push(order, h);
}

void heap_stats(uint64_t *free_pages, int *largest_order, uint64_t *allocated_blocks_out)
{
    uint64_t free = 0;
    int largest = -1;

    for (int i = 0; i <= BUDDY_MAX_ORDER; i++) {
        int count = 0;
        void *cur = free_lists[i];
        while (cur) {
            count++;
            struct buddy_header *h = (struct buddy_header *)cur;
            cur = (void *)h->free_next;
        }
        if (count > 0) {
            free += (uint64_t)count * (1ULL << i);
            largest = i;
        }
    }

    if (free_pages)
        *free_pages = free;
    if (largest_order)
        *largest_order = largest;
    if (allocated_blocks_out)
        *allocated_blocks_out = allocated_blocks;
}