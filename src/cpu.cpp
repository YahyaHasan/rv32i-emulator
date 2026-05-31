#include "cpu.h"

CPU::CPU() = default;   //default constructor

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

void CPU::execute(uint32_t instruction) {
    uint8_t opcode = opcode_of(instruction);

    switch(opcode) {
        case 0x13: {    // I-type arithmetic (ADDI, ANDI, ORI, ...)
            uint8_t rd = rd_of(instruction);
            uint8_t rs1 = rs1_of(instruction);
            uint8_t funct3 = funct3_of(instruction);
            uint32_t imm = immI_of(instruction);

            switch(funct3) {
                case 0x0: { // ADDI: rd = rs1 + sign_extend(imm)
                    set_register(rd, regs[rs1] + static_cast<uint32_t>(imm));
                    break;
                }
            }
            break;
        }
    }
}

void CPU::set_register(uint8_t index, uint32_t value) {
    // x0 is hardwired to zero in RISC-V. Writes to it are silently discarded
    if (index != 0) {
        regs[index] = value;
    }
}