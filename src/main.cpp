#include "cpu.h"
#include <iostream>
#include <iomanip>

static void section(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

// Write a 32-bit instruction to memory at addr (little-endian).
static void write_instr(CPU& cpu, uint32_t addr, uint32_t instr) {
    cpu.write_byte(addr,     instr & 0xFF);
    cpu.write_byte(addr + 1, (instr >> 8)  & 0xFF);
    cpu.write_byte(addr + 2, (instr >> 16) & 0xFF);
    cpu.write_byte(addr + 3, (instr >> 24) & 0xFF);
}

// Read a 32-bit word from memory at addr (little-endian).
static uint32_t read_word(CPU& cpu, uint32_t addr) {
    return  static_cast<uint32_t>(cpu.read_byte(addr))
         | (static_cast<uint32_t>(cpu.read_byte(addr + 1)) << 8)
         | (static_cast<uint32_t>(cpu.read_byte(addr + 2)) << 16)
         | (static_cast<uint32_t>(cpu.read_byte(addr + 3)) << 24);
}

int main() {
    std::cout << "RISC-V RV32I Emulator  --  Phase 1\n";
    std::cout << "===================================\n";

    // ---- Arithmetic ----
    section("Integer arithmetic (ADDI, ADD, SUB, LUI + ADDI)");
    {
        CPU cpu;
        cpu.execute(0x00A00093);  // ADDI x1, x0, 10
        cpu.execute(0x00300113);  // ADDI x2, x0, 3
        cpu.execute(0x002081B3);  // ADD  x3, x1, x2    = 13
        cpu.execute(0x40208233);  // SUB  x4, x1, x2    = 7
        cpu.execute(0x123452B7);  // LUI  x5, 0x12345   = 0x12345000
        cpu.execute(0x67828293);  // ADDI x5, x5, 0x678 = 0x12345678

        std::cout << "  10 + 3          = " << cpu.get_register(3) << "\n";
        std::cout << "  10 - 3          = " << cpu.get_register(4) << "\n";
        std::cout << "  LUI+ADDI result = 0x"
                  << std::hex << cpu.get_register(5) << std::dec << "\n";
    }

    // ---- Memory ----
    section("Memory (SW, LW, SB, LBU)");
    {
        CPU cpu;
        cpu.execute(0x40000093);  // ADDI x1, x0, 0x400    (base address)
        cpu.execute(0x0AB00113);  // ADDI x2, x0, 0xAB     (value)
        cpu.execute(0x00208023);  // SB   x2, 0(x1)
        cpu.execute(0x0000C183);  // LBU  x3, 0(x1)        (load it back)

        std::cout << "  Stored 0xAB at 0x400, loaded back: 0x"
                  << std::hex << cpu.get_register(3) << std::dec << "\n";
    }

    // ---- Branches and jumps ----
    section("Branches and jumps (BLT, JAL, JALR)");
    {
        CPU cpu;
        cpu.execute(0x00500093);  // ADDI x1, x0, 5
        cpu.execute(0x00A00113);  // ADDI x2, x0, 10
        uint32_t pc_before = cpu.get_pc();
        cpu.execute(0x0020C463);  // BLT x1, x2, +8  (5 < 10, taken)
        std::cout << "  BLT (5 < 10, taken): jumped "
                  << (cpu.get_pc() - pc_before) << " bytes\n";

        cpu.execute(0x100000EF);  // JAL x1, 0x100  (save ra, jump)
        std::cout << "  JAL 0x100: ra = " << cpu.get_register(1)
                  << ", pc = 0x" << std::hex << cpu.get_pc() << std::dec << "\n";
    }

    // ---- Loop via cycle() ----
    // Computes 1+2+3+4+5=15 as a real program running through fetch/execute.
    section("Loop: sum of 1..5 running via cycle()");
    {
        CPU cpu;

        // Program at address 0x000
        write_instr(cpu, 0x000, 0x00000093);  // ADDI x1, x0, 0   counter = 0
        write_instr(cpu, 0x004, 0x00000113);  // ADDI x2, x0, 0   sum = 0
        write_instr(cpu, 0x008, 0x00500193);  // ADDI x3, x0, 5   limit = 5
        write_instr(cpu, 0x00C, 0x00108093);  // ADDI x1, x1, 1   counter++  (loop)
        write_instr(cpu, 0x010, 0x00110133);  // ADD  x2, x2, x1  sum += counter
        write_instr(cpu, 0x014, 0xFE30CCE3);  // BLT  x1, x3, -8  if counter < 5, loop
        write_instr(cpu, 0x018, 0x20000213);  // ADDI x4, x0, 0x200  data address
        write_instr(cpu, 0x01C, 0x00222023);  // SW   x2, 0(x4)   store result
        write_instr(cpu, 0x020, 0x00000073);  // ECALL             halt

        uint32_t cycles = 0;
        while (!cpu.is_halted() && cycles < 10000) {
            cpu.cycle();
            ++cycles;
        }

        uint32_t result = read_word(cpu, 0x200);

        std::cout << "  sum of 1..5 = " << result
                  << (result == 15 ? "  (correct)" : "  (WRONG)") << "\n";
        std::cout << "  cycles used = " << cycles << "\n";
        std::cout << "  final pc    = 0x"
                  << std::hex << cpu.get_pc() << std::dec << "\n";
    }

    std::cout << "\nPhase 1 complete. All RV32I base instructions implemented.\n";
    std::cout << "Next: Phase 2 -- ELF binary loader.\n";
    return 0;
}
