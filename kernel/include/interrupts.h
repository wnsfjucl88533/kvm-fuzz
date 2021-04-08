#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H
#include "common.h"

struct InterruptFrame {
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
};

// Entry point of interrupts
void handle_page_fault(InterruptFrame* frame, uint64_t error_code);
void handle_breakpoint(InterruptFrame* frame);
void handle_general_protection_fault(InterruptFrame* frame, uint64_t error_code);
void handle_div_by_zero(InterruptFrame* frame);
void handle_stack_segment_fault(InterruptFrame* frame, uint64_t error_code);

#endif