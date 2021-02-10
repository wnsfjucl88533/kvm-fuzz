
/* 64-bit page * entry bits */
#define PDE64_PRESENT 1
#define PDE64_RW (1U << 1)
#define PDE64_USER (1U << 2)
#define PDE64_ACCESSED (1U << 5)
#define PDE64_DIRTY (1U << 6)
#define PDE64_PS (1U << 7)
#define PDE64_G (1U << 8)

/* CR0 bits */
#define CR0_PE 1u
#define CR0_MP (1U << 1)
#define CR0_EM (1U << 2)
#define CR0_TS (1U << 3)
#define CR0_ET (1U << 4)
#define CR0_NE (1U << 5)
#define CR0_WP (1U << 16)
#define CR0_AM (1U << 18)
#define CR0_NW (1U << 29)
#define CR0_CD (1U << 30)
#define CR0_PG (1U << 31)

/* CR4 bits */
#define CR4_VME 1
#define CR4_PVI (1U << 1)
#define CR4_TSD (1U << 2)
#define CR4_DE (1U << 3)
#define CR4_PSE (1U << 4)
#define CR4_PAE (1U << 5)
#define CR4_MCE (1U << 6)
#define CR4_PGE (1U << 7)
#define CR4_PCE (1U << 8)
#define CR4_OSFXSR (1U << 9)
#define CR4_OSXMMEXCPT (1U << 10)
#define CR4_UMIP (1U << 11)
#define CR4_VMXE (1U << 13)
#define CR4_SMXE (1U << 14)
#define CR4_FSGSBASE (1U << 16)
#define CR4_PCIDE (1U << 17)
#define CR4_OSXSAVE (1U << 18)
#define CR4_SMEP (1U << 20)
#define CR4_SMAP (1U << 21)

#define EFER_SCE 1
#define EFER_LME (1U << 8)
#define EFER_LMA (1U << 10)
#define EFER_NXE (1U << 11)

/* x86-64 specific MSRs */
#define MSR_EFER		0xc0000080 /* extended feature register */
#define MSR_STAR		0xc0000081 /* legacy mode SYSCALL target */
#define MSR_LSTAR		0xc0000082 /* long mode SYSCALL target */
#define MSR_CSTAR		0xc0000083 /* compat mode SYSCALL target */
#define MSR_SYSCALL_MASK	0xc0000084 /* EFLAGS mask for syscall */
#define MSR_FS_BASE		0xc0000100 /* 64bit FS base */
#define MSR_GS_BASE		0xc0000101 /* 64bit GS base */
#define MSR_KERNEL_GS_BASE	0xc0000102 /* SwapGS GS shadow */
#define MSR_TSC_AUX		0xc0000103 /* Auxiliary TSC */

// Page table stuff
#define PTL4_SHIFT 39
#define PTL4_BITS   9
#define PTL4_SIZE  (1UL << PTL4_SHIFT)
#define PTL4_MASK  (~(PTL4_SIZE - 1))
#define PTRS_PER_PTL4 (1UL << PTL4_BITS)
#define PTL4_INDEX(addr) ((addr >> PTL4_SHIFT) & (PTRS_PER_PTL4 - 1))

#define PTL3_SHIFT 30
#define PTL3_BITS   9
#define PTL3_SIZE  (1UL << PTL3_SHIFT)
#define PTL3_MASK  (~(PTL3_SIZE - 1))
#define PTRS_PER_PTL3 (1UL << PTL3_BITS)
#define PTL3_INDEX(addr) ((addr >> PTL3_SHIFT) & (PTRS_PER_PTL3 - 1))

#define PTL2_SHIFT 21
#define PTL2_BITS   9
#define PTL2_SIZE  (1UL << PTL2_SHIFT)
#define PTL2_MASK  (~(PTL2_SIZE - 1))
#define PTRS_PER_PTL2 (1UL << PTL2_BITS)
#define PTL2_INDEX(addr) ((addr >> PTL2_SHIFT) & (PTRS_PER_PTL2 - 1))

#define PTL1_SHIFT 12
#define PTL1_BITS  9
#define PTL1_SIZE  (1UL << PTL1_SHIFT)
#define PTL1_MASK  (~(PTL1_SIZE - 1))
#define PTRS_PER_PTL1 (1UL << PTL1_BITS)
#define PTL1_INDEX(addr) ((addr >> PTL1_SHIFT) & (PTRS_PER_PTL1 - 1))

#define PAGE_SIZE PTL1_SIZE
#define PAGE_OFFSET(addr) ((addr) & (~PTL1_MASK))