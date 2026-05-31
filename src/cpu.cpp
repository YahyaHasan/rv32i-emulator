#include "cpu.h"

CPU::CPU() = default;           //default constructor

uint32_t CPU::get_register(uint8_t index) const {
    return regs[index];
}

void CPU::cycle() {
    uint32_t instr = fetch();
    execute(instr);
}

uint32_t CPU::fetch() {
    //RISC-V is little-endian
    uint32_t instr = static_cast<uint32_t>(memory[pc])
                  | (static_cast<uint32_t>(memory[pc] + 1) << 8)
                  | (static_cast<uint32_t>(memory[pc + 2]) << 16)
                  | (static_cast<uint32_t>(memory[pc + 3] << 24));
    pc += 4;
    return instr;
}

void CPU::execute(uint32_t /*instruction*/) {
    //temp for now
}