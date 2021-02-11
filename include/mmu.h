#ifndef _MMU_H
#define _MMU_H

#include <vector>
#include "elf_parser.hpp"
#include "common.h"

class Mmu {
public:
	static const paddr_t PAGE_TABLE_PADDR = 0x1000;
	static const paddr_t SYSCALL_HANDLER_ADDR = 0x0; // both paddr and vaddr
	Mmu(int vm_fd, size_t mem_size);

	psize_t size() const;
	void load_elf(const std::vector<segment_t>& segments);
	void* translate(vaddr_t guest);
	void dump_memory(psize_t len) const;

	// Allocate a physical page, no vaddr associated
	paddr_t alloc_frame();

	// Get page table entry of given virtual address, allocating entries
	// if needed
	paddr_t* get_pte(vaddr_t vaddr);

	// Translate a virtual address to a physical address, allocating a frame
	// if needed
	paddr_t virt_to_phys(vaddr_t vaddr);

	// Get brk
	vaddr_t get_brk();

	// Set brk. Returns true if change was successful, false otherwise.
	bool set_brk(vaddr_t new_brk);

	void read_mem(void* dst, vaddr_t src, vsize_t len);
	void write_mem(vaddr_t dst, const void* src, vsize_t len);
	void set_mem(vaddr_t addr, int c, vsize_t len);
	void alloc(vaddr_t start, vsize_t len);

	template<class T>
	T read(vaddr_t addr);

	template <class T>
	void write(vaddr_t addr, T value);

private:
	// Guest physical memory
	uint8_t* memory;
	size_t   memory_len;

	// Pointer to page table level 4
	// (at physical address PAGE_TABLE_PADDR)
	paddr_t* ptl4;

	// Physical address of the next page allocated
	paddr_t next_page_alloc;

	vaddr_t brk, min_brk;

	void init_page_table();
};

template<class T>
T Mmu::read(vaddr_t addr) {
	T result;
	read_mem(&result, addr, sizeof(T));
	return result;
}

template<class T>
void Mmu::write(vaddr_t addr, T value) {
	write_mem(addr, &value, sizeof(T));
}
#endif