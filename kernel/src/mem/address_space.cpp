#include "address_space.h"
#include "pmm.h"
#include "x86/page_table.h"
#include "x86/asm.h"

bool AddressSpace::is_user_address(uintptr_t addr) {
	return addr < 0x800000000000;
}

bool AddressSpace::is_user_range(uintptr_t addr, size_t len) {
	if (addr + len < addr)
		return false;
	return is_user_address(addr) && is_user_address(addr + len - 1);
}

bool AddressSpace::is_user_range(const Range& range) {
	return is_user_range(range.base(), range.size());
}

AddressSpace::AddressSpace(uintptr_t ptl4_paddr)
	: m_page_table(ptl4_paddr)
	, m_next_user_mapping(USER_MAPPINGS_START_ADDR)
{
}

bool AddressSpace::alloc_range(Range& range) {
	// Try to allocate physical memory
	vector<uintptr_t> frames;
	size_t num_frames = PAGE_CEIL(range.size()) / PAGE_SIZE;
	if (!PMM::alloc_frames(num_frames, frames))
		return false;

	range.m_frames = move(frames);
	return true;
}

bool AddressSpace::free_range(Range& range) {
	if (!range.is_allocated())
		return false;

	for (uintptr_t frame : range.m_frames)
		PMM::free_frame(frame);
	range.m_frames.clear();
	return true;
}

bool AddressSpace::map_range(Range& range, uint8_t perms,
                             bool discard_already_mapped)
{
	// Check the range is in user range
	if (!is_user_range(range))
		return false;

	if (perms == MemPerms::None) TODO

	// Check range has already been allocated
	ASSERT(range.is_allocated(), "not allocated range: %p %p",
	       range.base(), range.size());
	size_t num_frames = PAGE_CEIL(range.size()) / PAGE_SIZE;
	size_t real_num_frames = range.m_frames.size();
	ASSERT(real_num_frames == num_frames, "number of frames doesnt match,"
	       " should be %lu but it's %lu", num_frames, real_num_frames);

	// Assign a base address to the range in case it doesn't have one
	if (!range.base()) {
		range.set_base(m_next_user_mapping);
		m_next_user_mapping += range.size();
	}

	// Map every page
	uint64_t page_flags = PageTableEntry::User;
	if (perms & MemPerms::Write)
		page_flags |= PageTableEntry::ReadWrite;
	if (!(perms & MemPerms::Exec))
		page_flags |= PageTableEntry::NoExecute;
	for (size_t i = 0; i < num_frames; i++) {
		uintptr_t page_base = range.base() + i*PAGE_SIZE;
		if (!m_page_table.map(page_base, range.m_frames[i], page_flags,
		                      discard_already_mapped))
			return false;
	}
	return true;
}

bool AddressSpace::unmap_range(const Range& range, bool ignore_not_mapped) {
	// Check the range is in user range
	if (!is_user_range(range))
		return false;

	// Unmap every page
	for (uintptr_t page_base = range.base();
	     page_base < range.base() + range.size();
	     page_base += PAGE_SIZE)
	{
		if (!m_page_table.unmap(page_base, ignore_not_mapped))
			return false;
	}
	return true;
}

bool AddressSpace::set_range_perms(const Range& range, uint8_t perms) {
	// Check the range is in user range
	if (!is_user_range(range))
		return false;

	// Set permissions to every page
	PageTableEntry* pte;
	for (uintptr_t page_base = range.base();
	     page_base < range.base() + range.size();
	     page_base += PAGE_SIZE)
	{
		pte = m_page_table.ensure_pte(page_base);
		if (!pte) {
			return false;
		}
		if (!pte->is_present()) {
			return false;
		}
		pte->set_writable(perms & MemPerms::Write);
		pte->set_execute_disabled(!(perms & MemPerms::Exec));
		flush_tlb_entry(page_base);
	}
	return true;
}