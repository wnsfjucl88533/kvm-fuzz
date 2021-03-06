#include "interrupts.h"
#include "common.h"
#include "asm.h"

struct InterruptFrame {
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
};

extern "C" void handle_interrupt(int interrupt, InterruptFrame* frame) {
	// Default interrupt handler, called by default ISRs
	printf("Interrupt %d at 0x%lx\n", interrupt, frame->rip);
	TODO
}

static void handle_page_fault(InterruptFrame* frame, uint64_t error_code) {
	bool present = error_code & (1 << 0);
	bool write   = error_code & (1 << 1);
	bool user    = error_code & (1 << 2);
	bool execute = error_code & (1 << 4);
	uint64_t fault_addr = rdcr2();
	ASSERT(user, "woops, kernel PF. addr: 0x%lx, present: %d, write: %d, ex: %d",
	       fault_addr, present, write, execute);

	FaultInfo fault = {
		.rip        = frame->rip,
		.fault_addr = fault_addr,
	};
	if (present)
		if (execute)
			fault.type = FaultInfo::Type::Exec;
		else
			fault.type = (write ? FaultInfo::Type::Write :
			                      FaultInfo::Type::Read);
	else
		if (execute)
			fault.type = FaultInfo::Type::OutOfBoundsExec;
		else
			fault.type = (write ? FaultInfo::Type::OutOfBoundsWrite :
			                      FaultInfo::Type::OutOfBoundsRead);

	// This won't return
	hc_fault(&fault);
}

static void handle_breakpoint(InterruptFrame* frame) {
	TODO
}

__attribute__((naked))
void _handle_page_fault() {
	asm volatile(
		"pop rsi;"
		"mov rdi, rsp;"
		"call %0;"
		"hlt;"
		:
		: "i" (handle_page_fault)
	);
}

__attribute__((naked))
void _handle_breakpoint() {
	asm volatile(
		"mov rdi, rsp;"
		"call %0;"
		"hlt;"
		:
		: "i" (handle_breakpoint)
	);
}