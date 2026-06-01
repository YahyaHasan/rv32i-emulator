// Bare-metal RISC-V hello world.
// No OS, no libc. Uses the Linux syscall ABI directly via ECALL.
// Syscall convention: a7 = syscall number, a0/a1/a2 = arguments.

// Write len bytes from buf to file descriptor fd.
static void sys_write(int fd, const char* buf, int len) {
    register int         a0 asm("a0") = fd;
    register const char* a1 asm("a1") = buf;
    register int         a2 asm("a2") = len;
    register int         a7 asm("a7") = 64;  // Linux write syscall
    asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a7));
}

// Exit the program with the given code.
static void sys_exit(int code) {
    register int a0 asm("a0") = code;
    register int a7 asm("a7") = 93;           // Linux exit syscall
    asm volatile("ecall" :: "r"(a0), "r"(a7));
    __builtin_unreachable();                   // tells GCC this never returns
}

// Static string lives in .rodata, not on the stack.
static const char msg[] = "Hello from RISC-V!\n";

// Main logic. Called after the stack is set up.
static void run(void) {
    sys_write(1, msg, sizeof(msg) - 1);
    sys_exit(0);
}

// Entry point. __attribute__((naked)) suppresses the compiler-generated
// prologue/epilogue entirely so we control exactly what runs first.
// We must set up the stack pointer before calling any C function,
// because normal C prologues push to the stack immediately.
// lui sp, 0x100 loads 0x100 << 12 = 0x100000 (top of our 1MB memory)
// into sp. The stack grows downward from there.
__attribute__((naked)) void _start(void) {
    asm volatile(
        "lui  sp, 0x100\n"   // sp = 0x100000 (top of 1MB emulator memory)
        "call run\n"          // jump to C code with stack ready
        ::: "sp"
    );
}
