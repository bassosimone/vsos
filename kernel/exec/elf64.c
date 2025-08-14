// File: kernel/exec/elf64.c
// Purpose: Code for parsing ELF64 binaries.
// SPDX-License-Identifier: MIT

#include <kernel/core/assert.h> // for KERNEL_ASSERT
#include <kernel/core/printk.h> // for printk
#include <kernel/exec/elf64.h>  // import API
#include <kernel/mm/page.h>     // for page_aligned
#include <kernel/mm/vm.h>       // for vm_map

#include <sys/errno.h> // for ENOEXEC
#include <sys/types.h> // for size_t

#include <string.h> // for memcmp

// Simplified view of the ELF64 header with enough fields for us to load a program.
struct elf64_ehdr {
	unsigned char e_ident[16]; // ELF magic + metadata
	uint16_t e_type;           // ET_EXEC for executables
	uint16_t e_machine;        // EM_AARCH64 (183)
	uint32_t e_version;        // EV_CURRENT (1)
	uint64_t e_entry;          // Entry point (the _start symbol)
	uint64_t e_phoff;          // Program header table offset
	uint64_t e_shoff;          // Section header table offset (ignore for loading)
	uint32_t e_flags;          // ARM64-specific flags
	uint16_t e_ehsize;         // ELF header size (64 bytes when using ARM64)
	uint16_t e_phentsize;      // Program header entry size (56 bytes)
	uint16_t e_phnum;          // Number of program headers

	// Fields that we don't parse because we do not care about them:
	//
	// uint16_t e_shentsize;   // Section header entry size
	// uint16_t e_shnum;       // Number of section headers
	// uint16_t e_shstrndx;    // String table index
};

#define EM_AARCH64 183
#define EV_CURRENT 1

struct elf64_phdr {
	uint32_t p_type;   // PT_LOAD, PT_NOTE, etc.
	uint32_t p_flags;  // PF_R, PF_W, PF_X permissions
	uint64_t p_offset; // Offset in file
	uint64_t p_vaddr;  // Virtual address to load at
	uint64_t p_paddr;  // Physical address (usually same as vaddr)
	uint64_t p_filesz; // Size in file
	uint64_t p_memsz;  // Size in memory
	uint64_t p_align;  // Alignment requirement
};

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4

// The user program base as defined in the linker script.
#define USER_PROGRAM_BASE 0x1000000

// The arbitrary maximum size of the user program.
#define MAX_USER_PROGRAM_SIZE 0x1000000

// The limit starting at which we cannot have user stuff.
#define USER_PROGRAM_LIMIT (USER_PROGRAM_BASE + MAX_USER_PROGRAM_SIZE)

// We do not enforce a maximum file size in the linker script but we
// check here that the user program is within bounds.
static inline bool valid_virtual_address(uintptr_t candidate) {
	return candidate >= USER_PROGRAM_BASE && candidate < USER_PROGRAM_LIMIT;
}

// Ensures that a virtual address plus its offset is still valid virtual memory.
static inline bool valid_virtual_address_offset(uintptr_t base, uintptr_t off) {
	if (!valid_virtual_address(base)) {
		return false;
	}
	if (base > UINTPTR_MAX - off) {
		return false;
	}
	return valid_virtual_address(base + off);
}

#define EI_CLASS 4
#define EI_DATA 5
#define ET_EXEC 2

#define ELFCLASS64 2
#define ELFDATA2LSB 1

static int64_t __fill(struct elf64_image *image, const void *data, size_t size) {
	// 1. convert to pointer and ensure we have enough space to touch it.
	printk("elf64: attempting to load file into memory\n");
	struct elf64_ehdr *ehdr = (struct elf64_ehdr *)data;
	if (size < sizeof(struct elf64_ehdr)) {
		return -ENOEXEC;
	}

	// 2. check magic number, number of bits, endianness, etc
	if (ehdr->e_ident[0] != 0x7F || memcmp(&ehdr->e_ident[1], "ELF", 3) != 0) {
		return -ENOEXEC;
	}
	if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
		return -ENOEXEC;
	}
	printk("  EI_CLASS: %u\n", (unsigned int)ehdr->e_ident[EI_CLASS]);
	if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		return -ENOEXEC;
	}
	printk("  EI_DATA: %u\n", (unsigned int)ehdr->e_ident[EI_DATA]);

	// 3. make sure the type is correct
	if (ehdr->e_type != ET_EXEC) {
		return -ENOEXEC;
	}
	printk("  e_type: %u\n", (unsigned int)ehdr->e_type);

	// 4. make sure the machine type is correct
	if (ehdr->e_machine != EM_AARCH64) {
		return -ENOEXEC;
	}
	printk("  e_machine: %u\n", (unsigned int)ehdr->e_machine);

	// 5. make sure the version is correct
	if (ehdr->e_version != EV_CURRENT) {
		return -ENOEXEC;
	}
	printk("  e_version: %u\n", (unsigned int)ehdr->e_version);

	// 6. we will check this field later once we know the sections
	if (!valid_virtual_address(ehdr->e_entry)) {
		return -ENOEXEC;
	}
	image->entry = ehdr->e_entry;
	printk("  e_entry: 0x%llx\n", ehdr->e_entry);

	// 7. check the program header table offset
	if (ehdr->e_phoff > size) {
		return -ENOEXEC;
	}
	printk("  e_phoff: 0x%llx\n", ehdr->e_phoff);

	// 8. check the section header table offset
	if (ehdr->e_shoff > size) {
		return -ENOEXEC;
	}
	printk("  e_shoff: 0x%llx\n", ehdr->e_shoff);

	// 9. check the flags
	if ((ehdr->e_flags & 0xF0000000) != 0) {
		return -ENOEXEC;
	}
	printk("  e_flags: 0x%llx\n", (unsigned int)ehdr->e_flags);

	// 10. check the eh header size
	if (ehdr->e_ehsize != 64) {
		return -ENOEXEC;
	}
	printk("  e_ehsize: %d\n", (unsigned int)ehdr->e_ehsize);

	// 11. check the ph header size
	if (ehdr->e_phentsize != 56) {
		return -ENOEXEC;
	}
	printk("  e_phentsize: %d\n", (unsigned int)ehdr->e_phentsize);

	// 12. check that we have a compatible number of program header entries
	if (ehdr->e_phnum < 1 || ehdr->e_phnum >= ELF64_MAX_SEGMENTS) {
		return -ENOEXEC;
	}
	printk("  e_phnum: %d\n", (unsigned int)ehdr->e_phnum);

	// 13. we stop parsing the EHDR here since we don't care about the other fields.
	//
	// (void)ehdr->e_shentsize;
	// (void)ehdr->e_shnum;
	// (void)ehdr->e_shstrndx;

	// 14. ensure we have enough bytes to parse
	if (size < ehdr->e_phoff) {
		return -ENOEXEC;
	}
	struct elf64_phdr *phdrs = (struct elf64_phdr *)((char *)data + ehdr->e_phoff);
	if ((uintptr_t)data >= UINTPTR_MAX - size) {
		return -ENOEXEC;
	}
	uintptr_t limit = (uintptr_t)data + size;

	// 15. walk through each PT_LOAD segment
	bool is_entry_good = false;
	for (; image->nsegments < ehdr->e_phnum; image->nsegments++) {
		// 15.1. ensure we can safely access the entry.
		struct elf64_phdr *entry = &phdrs[image->nsegments];
		if ((uintptr_t)entry > limit) {
			return -ENOEXEC;
		}
		if ((uintptr_t)entry >= UINTPTR_MAX - sizeof(*entry)) {
			return -ENOEXEC;
		}
		if ((uintptr_t)entry + sizeof(*entry) > limit) {
			return -ENOEXEC;
		}

		// 15.2. skip the entry if it's not PT_LOAD.
		if (entry->p_type != PT_LOAD) {
			continue;
		}
		struct elf64_segment *segment = &image->segments[image->nsegments];
		printk("    #%d:\n", image->nsegments);
		printk("      p_type: PT_LOAD\n");

		// 15.3. copy the flags and enforce W^X
		if ((entry->p_flags & (ELF64_PF_W | ELF64_PF_X)) == (ELF64_PF_W | ELF64_PF_X)) {
			return -ENOEXEC;
		}
		segment->flags = entry->p_flags;
		printk("      p_flags: 0x%x\n", (unsigned int)entry->p_flags);

		// 15.4. copy the section virtual address
		if (!valid_virtual_address(entry->p_vaddr)) {
			return -ENOEXEC;
		}
		segment->virt_addr = entry->p_vaddr;
		if (entry->p_vaddr != entry->p_paddr) {
			return -ENOEXEC;
		}
		printk("      p_vaddr: 0x%llx\n", entry->p_vaddr);

		// 15.5. copy the size of the virtual address space
		if (!valid_virtual_address_offset(segment->virt_addr, entry->p_memsz)) {
			return -ENOEXEC;
		}
		segment->mem_size = entry->p_memsz;
		printk("      p_memsz: 0x%llx\n", entry->p_memsz);

		// 15.6. ensure that the offset is within the correct bounds.
		if (entry->p_offset > size) {
			return -ENOEXEC;
		}
		segment->file_offset = entry->p_offset;
		printk("      p_offset: 0x%llx\n", entry->p_offset);

		// 15.7. ensure that the amount to read from the file is still within bounds.
		if (size < entry->p_filesz || entry->p_offset > size - entry->p_filesz) {
			return -ENOEXEC;
		}
		segment->file_size = entry->p_filesz;
		printk("      p_filesz: 0x%llx\n", entry->p_filesz);

		// 15.8. verify the alignment
		if (!page_aligned(entry->p_align)) {
			return -ENOEXEC;
		}
		printk("      p_align: 0x%llx\n", entry->p_align);

		// 15.9. ensure that the entry is actually good
		if (image->entry >= segment->virt_addr &&
		    image->entry < segment->virt_addr + segment->mem_size &&
		    (segment->flags & ELF64_PF_X) != 0) {
			if (is_entry_good) {
				return -ENOEXEC;
			}
			is_entry_good = true;
			printk("      is_entry_good: true\n");
		}
	}

	// 15. only continue if the program entry is within the X region
	return (is_entry_good) ? 0 : -ENOEXEC;
}

int64_t elf64_load(struct elf64_image *image, const void *data, size_t size) {
	// 1. basic sanity checks
	if (image == 0 || data == 0 || size <= 0) {
		return -EINVAL;
	}

	// 2. completely zero the return argument
	__bzero_unaligned((void *)image, sizeof(*image));

	// 3. initialize base and size
	image->base = data;
	image->size = size;

	// 4. defer to the internal parser
	return __fill(image, data, size);
}
