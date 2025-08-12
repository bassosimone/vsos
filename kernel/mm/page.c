// File: kernel/mm/page.c
// Purpose: Physical pages allocator.
// SPDX-License-Identifier: MIT

#include <kernel/boot/boot.h>     // for __free_ram_start
#include <kernel/core/assert.h>   // for KERNEL_ASSERT
#include <kernel/core/printk.h>   // for printk
#include <kernel/core/spinlock.h> // for struct spinlock
#include <kernel/mm/page.h>       // for page_alloc
#include <kernel/sched/sched.h>   // for sched_thread_yield

#include <sys/errno.h> // for EAGAIN
#include <sys/param.h> // for PAGE_SIZE
#include <sys/types.h> // for uintptr_t

/*-
  Memory Layout
  -------------

  We have 0x4000000 byte of free RAM starting at __free_ram_start
  and ending at __free_ram_end. This memory should be aligned.

  Memory addresses relative to __free_ram_start are in the range:

      [0x0000000, 0x4000000)

  Each page spans 4 KiB, so pages are in this range

      [0x0000000, 0x4000000)

  By shifting right by 12 bits, we obtain page indexes in this range:

      [0x0000, 0x4000)

  We use a bitmask composed of 64 bit entries. So, each entry in
  the bitmask manages 64 potentially-allocated pages.

  To convert from a page index to a bitmask index we use:

      slot_idx := page_idx / 64; // or >> 6
      bit_idx := page_idx & 63;  // or &0x3f

  This gives us the following binary layout for physical memory:

     MSB                                                                     LSB
      +-+-+ +-+-+-+-+ +-+-+    +-+-+ +-+-+-+-+    +-+-+-+-+ +-+-+-+-+ +-+-+-+-+
      |0|0| |0|0|0|0| |0|0|    |0|0| |0|0|0|0|    |0|0|0|0| |0|0|0|0| |0|0|0|0|
      +-+-+ +-+-+-+-+ +-+-+    +-+-+ +-+-+-+-+    +-+-+-+-+ +-+-+-+-+ +-+-+-+-+
     `---------------------'  `---------------'  `-----------------------------'
           slot_idx (8)          bit_idx (6)             within_page (12)

     `----------------------------------------'
                  page_idx (14)

  As such, allocating a physical page means this:

  1. walk through all the slots

  2. finding a zero bit

  3. doing slot_idx << 6 | bit_idx to obtain a page index

  4. doing a further << 12 to obtain the page relative address

  5. adding __free_ram_start to obtain a physical address

  Conversely, deallocating a page means:

  1. ensuring we're in the __free_ram_start, __free_ram_end bounds
     as well as that we're aligned to a page boundary

  2. subtracting __free_ram_start

  3. ensuring we're still page aligned

  4. doing >> 12 to get the page ID

  5. extracting slot_idx and bit_idx to modify the bitmask

  6. panicking if the page was not allocated
*/
#define RAM_SIZE 0x4000000
#define PAGE_SHIFT 12
#define PAGE_MASK (PAGE_SIZE - 1)
#define MAX_PAGES 0x4000
#define PAGES_PER_SLOT 64
#define SLOT_SHIFT 6
#define NUM_SLOTS 256

// Bitmask for managing the free pages.
static uint64_t bitmask[NUM_SLOTS];

void page_init_early(void) {
	// Runtime assertions on the loaded memory map
	KERNEL_ASSERT((uintptr_t)__free_ram_end - (uintptr_t)__free_ram_start == RAM_SIZE);
	KERNEL_ASSERT(((uintptr_t)__free_ram_start & PAGE_MASK) == 0);
	KERNEL_ASSERT(((uintptr_t)__free_ram_end & PAGE_MASK) == 0);

	// Compile time assertions on our assumptions
	static_assert((PAGE_SIZE & PAGE_MASK) == 0);
	static_assert(RAM_SIZE / PAGE_SIZE == MAX_PAGES);
	static_assert(RAM_SIZE % PAGE_SIZE == 0);
	static_assert(PAGES_PER_SLOT == (1ULL << SLOT_SHIFT));
	static_assert(NUM_SLOTS * PAGES_PER_SLOT == MAX_PAGES);
	static_assert((sizeof(bitmask[0]) << 3) == PAGES_PER_SLOT);
	static_assert((1ULL << PAGE_SHIFT) == PAGE_SIZE);
}

// Allocate a free page returning 0 and the page index on success, -ENOMEM on failure.
static inline int64_t bitmask_alloc(size_t *index) {
	KERNEL_ASSERT(index != 0);
	*index = 0; // avoid possible UB

	for (size_t slot_idx = 0; slot_idx < NUM_SLOTS; slot_idx++) {
		uint64_t entry = bitmask[slot_idx];
		if (entry == UINT64_MAX) {
			continue;
		}

		for (size_t bit_idx = 0; bit_idx < PAGES_PER_SLOT; bit_idx++) {
			uint64_t bit = (1ULL << bit_idx);
			if ((entry & bit) != 0) {
				continue;
			}

			bitmask[slot_idx] = entry | bit;
			KERNEL_ASSERT(slot_idx <= (SIZE_MAX >> SLOT_SHIFT));
			*index = (slot_idx << SLOT_SHIFT) | bit_idx;

			printk("bitmask_alloc: %llx %llx => %llx\n", slot_idx, bit_idx, *index);
			return 0;
		}
	}
	return -ENOMEM;
}

// Free an allocated page panicking if it was not allocated.
static inline void bitmask_free(size_t index) {
	KERNEL_ASSERT(index < MAX_PAGES);

	size_t slot_idx = (index >> SLOT_SHIFT);
	KERNEL_ASSERT(slot_idx < NUM_SLOTS);

	size_t bit_idx = (index & (PAGES_PER_SLOT - 1));
	KERNEL_ASSERT(bit_idx < PAGES_PER_SLOT);

	uint64_t bit = (1ULL << bit_idx);
	KERNEL_ASSERT((bitmask[slot_idx] & bit) != 0); // not allocated?

	bitmask[slot_idx] &= ~bit;
	printk("bitmask_free: %llx => %llx %llx\n", index, slot_idx, bit_idx);
}

// Spinlock for protecting allocation.
static struct spinlock lock;

// Convert page index into physical page address
static inline page_addr_t make_page_addr(size_t index) {
	uintptr_t page_offset = index << PAGE_SHIFT;

	uintptr_t base = (uintptr_t)__free_ram_start;
	KERNEL_ASSERT(base <= UINTPTR_MAX - page_offset);
	page_addr_t addr = base + page_offset;

	uintptr_t end = (uintptr_t)__free_ram_end;
	KERNEL_ASSERT(addr <= end - PAGE_SIZE);

	return addr;
}

int64_t page_alloc(page_addr_t *addr, uint64_t flags) {
	KERNEL_ASSERT(addr != 0);
	*addr = 0; // Avoid possible UB

	for (;;) {
		while (spinlock_try_acquire(&lock) != 0) {
			if ((flags & PAGE_ALLOC_WAIT) == 0) {
				return -EAGAIN;
			}
			if ((flags & PAGE_ALLOC_YIELD) != 0) {
				sched_thread_yield();
			}
		}

		size_t index = 0;
		int64_t rc = bitmask_alloc(&index);
		spinlock_release(&lock);

		if (rc < 0) {
			if ((flags & PAGE_ALLOC_WAIT) == 0) {
				return -ENOMEM;
			}
			if ((flags & PAGE_ALLOC_YIELD) != 0) {
				sched_thread_yield();
			}
			continue;
		}

		*addr = make_page_addr(index);
		printk("page_alloc: %llx => %llx\n", index, *addr);
		return 0;
	}
}

void page_free(page_addr_t addr) {
	// Ensure the address is within RAM and aligned
	printk("page_free: %llx %llx %llx\n", (uintptr_t)__free_ram_start, addr, (uintptr_t)__free_ram_end);
	KERNEL_ASSERT(addr >= (uintptr_t)__free_ram_start);
	KERNEL_ASSERT(addr < (uintptr_t)__free_ram_end);
	KERNEL_ASSERT((addr & PAGE_MASK) == 0);

	// Transform to offset that must be aligned
	uintptr_t offset = addr - (uintptr_t)__free_ram_start;
	KERNEL_ASSERT((offset & PAGE_MASK) == 0);

	// Transform to index and remove the index
	size_t index = offset >> PAGE_SHIFT;
	printk("page_free: %llx => %llx\n", addr, index);
	spinlock_acquire(&lock);
	bitmask_free(index);
	spinlock_release(&lock);
}

void page_debug_printk(void) {
	spinlock_acquire(&lock);
	for (size_t slot_idx = 0; slot_idx < NUM_SLOTS; slot_idx++) {
		printk("page_debug_printk: %lld %llx\n", slot_idx, bitmask[slot_idx]);
	}
	spinlock_release(&lock);
}
