# RV32I Emulator

A RISC-V RV32I emulator written from scratch in C++17, built to understand the
instruction set and CPU/memory model at the level of individual bit fields,
not just to call a library.

## Status

- All 47 base integer instructions implemented across all six RV32I encoding
  formats (R, I, S, B, U, J): full decode of opcode/rd/funct3/rs1/rs2/funct7
  and all five immediate encodings, register/memory state, `fetch`/`execute`/
  `cycle` loop.
- GoogleTest suite grouped by instruction family (`tests/test_cpu.cpp`).
- RISC-V GNU cross-compilation toolchain configured; `programs/hello.c`
  compiles to a bare-metal RV32I ELF32 binary (`riscv64-unknown-elf-gcc
  -march=rv32i -mabi=ilp32 -nostdlib -static -ffreestanding`). This
  establishes the target format the loader below will consume; the emulator
  does not yet parse or run it.
- CPU memory lives on the heap (`std::vector<uint8_t>`, 1 MiB) to avoid a
  stack-frame blowup from a fixed-size array.

## Build

```
mkdir build && cd build
cmake ..
cmake --build .
ctest
```

## Next steps

Roughly in the order that makes sense to build them, since each one unlocks
the next:

1. **ELF loader.** Parse `programs/hello`'s ELF header and program headers,
   load `PT_LOAD` segments into emulator memory at their specified virtual
   addresses, and set the initial PC to the ELF entry point. This is the
   natural next step since the toolchain already produces the binary; right
   now nothing in the emulator consumes it. Turns the project from "runs
   hand-assembled instruction sequences" into "runs a real compiled program."

2. **Memory-mapped UART output.** A single write-only TX data register at a
   fixed address outside the RAM window; a store to that address appends the
   byte to a captured output stream instead of writing to backing memory.
   This is the minimum viable way for a loaded ELF program to produce
   observable output (`hello.c` can `printf`/write a string through it),
   and it's the standard way real embedded RISC-V systems talk to a serial
   console, so it reads as a deliberate peripheral-modeling choice, not a
   toy add-on.

3. **CSRs and trap handling.** Minimal privileged-state registers (`mtvec`,
   `mepc`, `mcause`) and a real trap path for ECALL/EBREAK and illegal
   instructions, replacing today's stub that just halts. This is what
   actually makes "diagnosing why a program halted" defensible in an
   interview, and it's the piece embedded/firmware interviewers are most
   likely to probe given it's where hardware and software behavior meet.

4. **A second peripheral with interrupts** (e.g. a timer that raises a
   periodic interrupt once traps exist). Demonstrates the interrupt path
   end-to-end rather than just polling, which is the detail that
   distinguishes real embedded systems work from an instruction-set
   exercise.

Each step should keep the existing GoogleTest suite green and add its own
grouped test file, the same way `test_cpu.cpp` is organized by instruction
family.
