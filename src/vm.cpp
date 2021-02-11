#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/syscall.h>
#include "vm.h"

using namespace std;

int kvm_fd = -1;

void init_kvm() {
	kvm_fd = open("/dev/kvm", O_RDWR);
	assert(kvm_fd != -1);

	int api_ver = ioctl(kvm_fd, KVM_GET_API_VERSION, 0);
	assert(api_ver == KVM_API_VERSION);
}

Vm::Vm(vsize_t mem_size, const string& filepath, const vector<string>& argv):
	vm_fd(ioctl_chk(kvm_fd, KVM_CREATE_VM, 0)),
	vcpu{
		ioctl_chk(vm_fd, KVM_CREATE_VCPU, 0),
		(kvm_run*)mmap(NULL, ioctl_chk(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0),
		               PROT_READ|PROT_WRITE, MAP_SHARED, vcpu.fd, 0)
	},
	elf(filepath),
	mmu(vm_fd, mem_size),
	running(false)
{
	// Check if mmap failed
	if (vcpu.run == MAP_FAILED)
		die("mmap kvm_run");

	setup_long_mode();

	load_elf(argv);
}

psize_t Vm::memsize() const {
	return mmu.size();
}

void Vm::setup_long_mode() {
	ioctl_chk(vm_fd, KVM_SET_TSS_ADDR, 0xfffbd000);

	kvm_sregs sregs;
	ioctl_chk(vcpu.fd, KVM_GET_SREGS, &sregs);
	sregs.cr3 = Mmu::PAGE_TABLE_PADDR;
	sregs.cr4 = CR4_PAE | CR4_OSXMMEXCPT | CR4_OSFXSR;
	sregs.cr0 = CR0_PE | CR0_MP | CR0_ET| CR0_NE | CR0_WP | CR0_AM | CR0_PG;
	sregs.efer = EFER_LME | EFER_LMA | EFER_SCE;

	// Setup segments
	// https://wiki.osdev.org/Global_Descriptor_Table
	// https://wiki.osdev.org/GDT_Tutorial
	kvm_segment seg = {
		.base     = 0,          // base address
		.limit    = 0xffffffff, // limit
		.selector = 0x8,        // index 1 (index 0 is null segment descriptor)
		.type     = 11,         // execute, read, accessed, idk why ???
		.present  = 1,          // bit P
		.dpl      = 3,          // Descriptor Privilege Level. 3 = user mode
		.db       = 0,          // Default operand size / Big
		.s        = 1,          // Descriptor type
		.l        = 1,          // Long: 64-bit segment. db must be zero
		.g        = 1           // Granularity: limit unit is byte and not page
	};
	sregs.cs = seg;
	seg.type     = 3;    // read, write, accessed, idk why ???
	seg.selector = 0x10; // index 2
	sregs.ds = sregs.es = sregs.fs = sregs.gs = sregs.ss = seg;
	ioctl_chk(vcpu.fd, KVM_SET_SREGS, &sregs);

	// Setup MSRs
	size_t sz = sizeof(kvm_msrs) + sizeof(kvm_msr_entry)*3;
	kvm_msrs* msrs = (kvm_msrs*)alloca(sz);
	memset(msrs, 0, sz);
	// Don't know if MSR_STAR is needed
	msrs->nmsrs = 3;
	msrs->entries[0] = {
		.index = MSR_LSTAR, // Long Syscall TARget
		.data = Mmu::SYSCALL_HANDLER_ADDR
	};
	msrs->entries[1] = {
		.index = MSR_STAR, // legacy Syscall TARget
		.data = 0x0020000800000000
	};
	msrs->entries[2] = {
		.index = MSR_SYSCALL_MASK,
		.data = 0x3f7fd5
	};
	ioctl_chk(vcpu.fd, KVM_SET_MSRS, msrs);

	// Setup cpuid, not sure if needed
	sz = sizeof(kvm_cpuid2) + sizeof(kvm_cpuid_entry2)*100;
	kvm_cpuid2* cpuid = (kvm_cpuid2*)alloca(sz);
	memset(cpuid, 0, sz);
	cpuid->nent = 100;
	ioctl_chk(kvm_fd, KVM_GET_SUPPORTED_CPUID, cpuid);
	ioctl_chk(vcpu.fd, KVM_SET_CPUID2, cpuid);
}

void Vm::load_elf(const std::vector<std::string>& argv) {
	// http://articles.manugarg.com/aboutelfauxiliaryvectors.html
	mmu.load_elf(elf.get_segments());

	struct kvm_regs regs;
	memset(&regs, 0, sizeof(regs));

	// Allocate stack
	vaddr_t stack_init = 0x800000000000;
	vsize_t stack_size = 0x10000;
	mmu.alloc(stack_init - stack_size, stack_size);
	regs.rsp = stack_init;

	// NULL
	regs.rsp -= 16;
	mmu.write<vaddr_t>(regs.rsp, 0);
	mmu.write<vaddr_t>(regs.rsp + 8, 0);

	// Random bytes for auxv. Seed is not initialized
	regs.rsp -= 16;
	vaddr_t random_bytes = regs.rsp;
	for (int i = 0; i < 16; i++)
		mmu.write<uint8_t>(random_bytes + i, rand());

	// Write argv strings saving pointers to each arg
	vector<vaddr_t> argv_addrs;
	for (const string& arg : argv) {
		regs.rsp -= arg.size() + 1;
		mmu.write_mem(regs.rsp, arg.c_str(), arg.size()+1);
		argv_addrs.push_back(regs.rsp);
	}
	argv_addrs.push_back(0); // null ptr, end of argv

	// Align rsp
	regs.rsp &= ~0x7;
	//regs.rsp = (regs.rsp - 0x7) & ~0x7;

	// Set up auxp
	/* phinfo_t phinfo    = elf.get_phinfo();
	vaddr_t  load_addr = elf.get_load_addr();
	Elf64_auxv_t auxv[] = {
		{AT_RANDOM, {random_bytes}},               // Address of 16 random bytes
		{AT_EXECFN, {argv_addrs[0]}},              // Filename of the program
		{AT_PHDR,   {load_addr + phinfo.e_phoff}}, // Pointer to program headers
		{AT_PHENT,  {phinfo.e_phentsize}},         // Size of each entry
		{AT_PHNUM,  {phinfo.e_phnum}},             // Number of entries
		{AT_PAGESZ, {4096}},                       // Page size
		{AT_NULL,   {0}},                          // Auxv end
	}; */
	Elf64_auxv_t auxv[] = {
		{AT_RANDOM, {random_bytes}},
		{AT_NULL,   {0}}
	};
	regs.rsp -= sizeof(auxv);
	mmu.write_mem(regs.rsp, auxv, sizeof(auxv));

	// Set up envp
	regs.rsp -= 8;
	mmu.write<vaddr_t>(regs.rsp, 0);

	// Set up argv
	for (auto it = argv_addrs.rbegin(); it != argv_addrs.rend(); ++it) {
		regs.rsp -= 8;
		mmu.write<vaddr_t>(regs.rsp, *it);
	}

	// Set up argc
	regs.rsp -= 8;
	mmu.write<uint64_t>(regs.rsp, argv.size());

	regs.rflags = 2;
	regs.rip = elf.get_entry();
	ioctl_chk(vcpu.fd, KVM_SET_REGS, &regs);
}

void Vm::run() {
	running = true;

	while (running) {
		ioctl_chk(vcpu.fd, KVM_RUN, 0);
		switch (vcpu.run->exit_reason) {
			case KVM_EXIT_HLT:
				dump_regs();
				die("HLT\n");
				break;

			case KVM_EXIT_IO:
				if (vcpu.run->io.direction == KVM_EXIT_IO_OUT &&
					vcpu.run->io.port == 16)
				{
					handle_syscall();
				} else {
					die("IO\n");
				}
				break;

			case KVM_EXIT_FAIL_ENTRY:
				die("KVM_EXIT_FAIL_ENTRY");

			case KVM_EXIT_INTERNAL_ERROR:
				die("KVM_EXIT_INTERNAL_ERROR");

			case KVM_EXIT_SHUTDOWN:
				cout << endl << endl << "[KVM_EXIT_SHUTDOWN]" << endl;
				dump_regs();
				//dump_memory();
				die("KVM_EXIT_SHUTDOWN");

			default:
				die("UNKNOWN EXIT");
		}
	}
}


void Vm::dump_regs() {
	kvm_regs regs;
	ioctl_chk(vcpu.fd, KVM_GET_REGS, &regs);
	printf("rip: 0x%016llX\n", regs.rip);
	printf("rax: 0x%016llX  rbx: 0x%016llX  rcx: 0x%016llX  rdx: 0x%016llX\n", regs.rax, regs.rbx, regs.rcx, regs.rdx);
	printf("rsi: 0x%016llX  rdi: 0x%016llX  rsp: 0x%016llX  rbp: 0x%016llX\n", regs.rsi, regs.rdi, regs.rsp, regs.rbp);
	printf("r8:  0x%016llX  r9:  0x%016llX  r10: 0x%016llX  r11: 0x%016llX\n", regs.r8, regs.r9, regs.r10, regs.r11);
	printf("r12: 0x%016llX  r13: 0x%016llX  r14: 0x%016llX  r15: 0x%016llX\n", regs.r12, regs.r13, regs.r14, regs.r15);

	/* kvm_fpu fregs;
	ioctl_chk(vcpu.fd, KVM_GET_FPU, &regs);
	for (int i = 0; i < 16; i++) {
		printf("xmm%02d: %08Lf  ", i, *(long double*)&fregs.xmm[i]);
		if ((i+1)%4 == 0)
			printf("\n");
	} */
}

void Vm::dump_memory() const {
	dump_memory(memsize());
}

void Vm::dump_memory(psize_t len) const {
	mmu.dump_memory(len);
}