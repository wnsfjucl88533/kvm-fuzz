usingnamespace @import("common.zig");
const idt = @import("x86/idt.zig");
const assembly = @import("x86/asm.zig");

/// The type of each interrupt handler entry point, which will end up jumping
/// to the actual interrupt handler.
pub const InterruptHandlerEntryPoint = fn () callconv(.Naked) void;

/// The type of each interrupt handler, which will do the actual work.
const InterruptHandler = fn (*InterruptFrame) void;

/// The data we'll have in the stack inside every interrupt handler.
const InterruptFrame = struct {
    // Registers. Pushed by us, except rsp which is pushed by the CPU and
    // it's below.
    rax: u64,
    rbx: u64,
    rcx: u64,
    rdx: u64,
    rbp: u64,
    rsi: u64,
    rdi: u64,
    r8: u64,
    r9: u64,
    r10: u64,
    r11: u64,
    r12: u64,
    r13: u64,
    r14: u64,
    r15: u64,

    // Interrupt number, pushed by us.
    interrupt_number: u64,

    // Pushed by the CPU for those interrupts that have error code, and set to
    // zero by us for the rest of them.
    error_code: u64,

    // Pushed by the CPU.
    rip: u64,
    cs: u64,
    rflags: u64,
    rsp: u64,
};

/// Array of interrupt handlers.
const handlers = blk: {
    var handlers_tmp: [idt.N_IDT_ENTRIES]InterruptHandler = undefined;
    var i = 0;
    while (i < idt.N_IDT_ENTRIES) : (i += 1) {
        handlers_tmp[i] = defaultInterruptHandler;
    }

    handlers_tmp[idt.ExceptionNumber.PageFault] = handlePageFault;
    handlers_tmp[idt.ExceptionNumber.Breakpoint] = handleBreakpoint;
    handlers_tmp[idt.ExceptionNumber.GeneralProtectionFault] = handleGeneralProtectionFault;
    handlers_tmp[idt.ExceptionNumber.DivByZero] = handleDivByZero;
    handlers_tmp[idt.ExceptionNumber.StackSegmentFault] = handleStackSegmentFault;
    handlers_tmp[idt.IRQNumber.APICTimer] = handleApicTimer;
    break :blk handlers_tmp;
};

/// Returns whether the interrupt pushes an error code into the stack or not.
fn pushesErrorCode(interrupt_number: usize) bool {
    return switch (interrupt_number) {
        0x00...0x07 => false,
        0x08 => true,
        0x09 => false,
        0x0A...0x0E => true,
        0x0F...0x10 => false,
        0x11 => true,
        0x12...0x14 => false,
        else => false,
    };
}

/// Creates the interrupt handler entry point for each interrupt at compile time.
/// This is where the CPU will jump to when the interrupt occurs. It will simply
/// push a 0 as error code if the CPU didn't push one, push the interrupt number,
/// and jump to interruptHandlerCommon.
pub fn getInterruptHandlerEntryPoint(comptime interrupt_number: usize) InterruptHandlerEntryPoint {
    return struct {
        fn handler() callconv(.Naked) void {
            if (comptime !pushesErrorCode(interrupt_number)) {
                asm volatile ("push $0");
            }

            asm volatile (
                \\push %[interrupt_number]
                \\jmp interruptHandlerCommon
                :
                : [interrupt_number] "im" (interrupt_number)
            );
        }
    }.handler;
}

/// This function is the first common part of each interrupt handler. It is
/// in charge of saving and restoring user registers using the stack and
/// callign interruptHandler.
export fn interruptHandlerCommon() callconv(.Naked) void {
    asm volatile (
    // Push registers in InterruptFrame in reverse order
        \\push %%r15
        \\push %%r14
        \\push %%r13
        \\push %%r12
        \\push %%r11
        \\push %%r10
        \\push %%r9
        \\push %%r8
        \\push %%rdi
        \\push %%rsi
        \\push %%rbp
        \\push %%rdx
        \\push %%rcx
        \\push %%rbx
        \\push %%rax

        // Call interruptHandler, passing a pointer to the InterruptFrame we
        // just built in the stack as first argument
        \\mov %%rsp, %%rdi
        \\call interruptHandler

        // Restore the registers
        \\pop %%rax
        \\pop %%rbx
        \\pop %%rcx
        \\pop %%rdx
        \\pop %%rbp
        \\pop %%rsi
        \\pop %%rdi
        \\pop %%r8
        \\pop %%r9
        \\pop %%r10
        \\pop %%r11
        \\pop %%r12
        \\pop %%r13
        \\pop %%r14
        \\pop %%r15

        // Return from the interrupt
        \\iretq
    );
}

/// Simply calls the handler for the interrupt specified in the interrupt frame.
export fn interruptHandler(frame: *InterruptFrame) void {
    handlers[frame.interrupt_number](frame);
}

fn defaultInterruptHandler(frame: *InterruptFrame) void {
    panic("unhandled interrupt: {}\n", .{frame.interrupt_number});
}

fn handlePageFault(frame: *InterruptFrame) void {
    const present = (frame.error_code & (1 << 0)) != 0;
    const write = (frame.error_code & (1 << 1)) != 0;
    const user = (frame.error_code & (1 << 2)) != 0;
    const execute = (frame.error_code & (1 << 4)) != 0;
    const fault_addr = assembly.rdcr2();

    panic("page fault at {x}, fault addr {x}\n", .{ frame.rip, fault_addr });
}

fn handleBreakpoint(frame: *InterruptFrame) void {
    @panic("breakpoint\n");
}

fn handleGeneralProtectionFault(frame: *InterruptFrame) void {
    @panic("gpf\n");
}

fn handleDivByZero(frame: *InterruptFrame) void {
    @panic("div by zero\n");
}

fn handleStackSegmentFault(frame: *InterruptFrame) void {
    @panic("stack segment fault\n");
}

fn handleApicTimer(frame: *InterruptFrame) void {
    @panic("apic timer\n");
}
