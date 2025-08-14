// File: kernel/exec/elf64.c
// Purpose: Code for parsing ELF64 binaries.
// SPDX-License-Identifier: MIT

#include <kernel/core/assert.h> // for KERNEL_ASSERT
#include <kernel/core/printk.h> // for printk
#include <kernel/exec/elf64.h>  // import API

#include <sys/errno.h> // for ENOEXEC
#include <sys/types.h> // for size_t

#include <string.h> // for memcmp

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
	uint16_t e_shentsize;      // Section header entry size
	uint16_t e_shnum;          // Number of section headers
	uint16_t e_shstrndx;       // String table index
};

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

// File class (32/64-bit)
#define EI_CLASS 4

// Data encoding (endianness)
#define EI_DATA 5

// Invalid class
#define ELFCLASSNONE 0

// 32-bit objects
#define ELFCLASS32 1

// 64-bit objects
#define ELFCLASS64 2

// Invalid data encoding
#define ELFDATANONE 0

// Little-endian
#define ELFDATA2LSB 1

// Big-endian
#define ELFDATA2MSB 2

// arm64
#define EM_AARCH64 183

// Executable file
#define ET_EXEC 2

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

static inline bool valid_virtual_address_offset(uintptr_t base, uintptr_t off) {
	if (!valid_virtual_address(base)) {
		return false;
	}
	if (base > UINTPTR_MAX - off) {
		return false;
	}
	return valid_virtual_address(base + off);
}

// Parse the EHDR section and return the result into *result.
//
// Returns 0 or a negative errno code.
static inline int64_t parse_ehdr(struct elf64_ehdr **result, const void *data, size_t size) {
	// 1. ensure we have enough bytes to parse
	if (size < sizeof(struct elf64_ehdr)) {
		return -ENOEXEC;
	}
	struct elf64_ehdr *ehdr = (struct elf64_ehdr *)data;

	// 2. check magic number
	if (ehdr->e_ident[0] != 0x7F || memcmp(&ehdr->e_ident[1], "ELF", 3) != 0) {
		return -ENOEXEC;
	}
	printk("elf64: attempting to load file into memory\n");

	// 3. check for 64 bit
	if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
		return -ENOEXEC;
	}
	printk("  EI_CLASS: %u\n", (unsigned int)ehdr->e_ident[EI_CLASS]);

	// 4. check for little endian
	if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		return -ENOEXEC;
	}
	printk("  EI_DATA: %u\n", (unsigned int)ehdr->e_ident[EI_DATA]);

	// 5. check for arm64
	if (ehdr->e_machine != EM_AARCH64) {
		return -ENOEXEC;
	}
	printk("  e_machine: %u\n", (unsigned int)ehdr->e_machine);

	// 6. check for type
	if (ehdr->e_type != ET_EXEC) {
		return -ENOEXEC;
	}
	printk("  e_type: %u\n", (unsigned int)ehdr->e_type);

	// 7. ensure the entry point is valid
	printk("  e_entry: 0x%llx\n", ehdr->e_entry);
	if (!valid_virtual_address(ehdr->e_entry)) {
		return -ENOEXEC;
	}

	// 8. so far, so good...
	*result = ehdr;
	return 0;
}

// Unused entry
#define PT_NULL 0

// Loadable segment
#define PT_LOAD 1

// Dynamic linking info
#define PT_DYNAMIC 2

// Interpreter path
#define PT_INTERP 3

// Auxiliary info
#define PT_NOTE 4

// Execute permission
#define PF_X 1

// Write permission
#define PF_W 2

// Read permission
#define PF_R 4

// Given a single PT_LOAD segment, copy it into user memory pages.
static inline int64_t mmap_segment(struct elf64_phdr *entry) {
	uint64_t vaddr = entry->p_vaddr;
	printk("      vaddr: 0x%llx\n", vaddr);
	if (!valid_virtual_address(vaddr)) {
		return -ENOEXEC;
	}

	uint64_t memsz = entry->p_memsz;
	printk("      memsz: 0x%llx\n", memsz);
	if (!valid_virtual_address_offset(vaddr, memsz)) {
		return -ENOEXEC;
	}

	uint32_t flags = entry->p_flags;
	printk("      flags: 0x%llx\n", flags);

	uint64_t filesz = entry->p_filesz;
	printk("      filesz: 0x%llx\n", filesz);
	if (filesz > memsz) {
		return -ENOEXEC;
	}

	// TODO(bassosimone): actually load into memory

	return 0;
}

// Use a safe upper bound to the total number of entries.
#define MAX_PT_LOAD_ENTRIES 128

// Loads the PT_LOAD segments into memory pages.
static inline int64_t load_pt_load_segments(struct elf64_ehdr *ehdr, const void *data, size_t size) {
	// 1. ensure we have enough bytes to parse
	if (size < ehdr->e_phoff) {
		return -ENOEXEC;
	}
	struct elf64_phdr *phdrs = (struct elf64_phdr *)((char *)data + ehdr->e_phoff);
	if ((uintptr_t)data > UINTPTR_MAX - size) {
		return -ENOEXEC;
	}
	uintptr_t limit = (uintptr_t)data + size;

	// 2. arbitrarily limit to a reasonable number of entries
	// mainly to avoid being DoS-ed by a shell ELF.
	if (ehdr->e_phnum <= 0 || ehdr->e_phnum > MAX_PT_LOAD_ENTRIES) {
		return -ENOEXEC;
	}

	// 3. walk through each PT_LOAD segment
	printk("  e_phnum: %u\n", (unsigned int)ehdr->e_phnum);
	for (int idx = 0; idx < ehdr->e_phnum; idx++) {
		// 3.1. ensure we can safely access the entry.
		struct elf64_phdr *entry = &phdrs[idx];
		if ((uintptr_t)entry > limit) {
			return -ENOEXEC;
		}
		if ((uintptr_t)entry >= UINTPTR_MAX - sizeof(*entry)) {
			return -ENOEXEC;
		}
		if ((uintptr_t)entry + sizeof(*entry) > limit) {
			return -ENOEXEC;
		}

		// 3.2. skip the entry if it's not PT_LOAD.
		if (entry->p_type != PT_LOAD) {
			continue;
		}
		printk("    #%d:\n", idx);
		printk("      p_type: PT_LOAD\n");

		// 3.3. attempt to mmap the segment.
		int rc = mmap_segment(entry);
		if (rc != 0) {
			return rc;
		}
	}

	return 0;
}

int64_t elf64_load(uintptr_t *entry, const void *data, size_t size) {
	// 1. Parse the ELF header
	struct elf64_ehdr *ehdr = 0;
	int rc = parse_ehdr(&ehdr, data, size);
	if (rc != 0) {
		return rc;
	}
	KERNEL_ASSERT(ehdr != 0);

	// 2. Load each section into memory
	rc = load_pt_load_segments(ehdr, data, size);
	if (rc != 0) {
		return rc;
	}

	// TODO(bassosimone): we probably need to validate e_entry
	// and ensure it is inside an executable section.

	// 3. save the entry point and return
	*entry = ehdr->e_entry;
	return 0;
}
