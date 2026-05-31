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
                  | (static_cast<uint32_t>(memory[pc + 1]) << 8)
                  | (static_cast<uint32_t>(memory[pc + 2]) << 16)
                  | (static_cast<uint32_t>(memory[pc + 3] << 24));
    pc += 4;
    return instr;
}

void CPU::set_register(uint8_t index, uint32_t value) {
    // x0 is hardwired to zero in RISC-V. Writes to it are silently discarded
    if (index != 0) {
        regs[index] = value;
    }
}

void CPU::execute(uint32_t instruction) {
    uint8_t opcode = opcode_of(instruction);

    switch(opcode) {
        case 0x13: {    // I-type arithmetic (ADDI, ANDI, ORI, ...)
            uint8_t rd = rd_of(instruction);
            uint8_t rs1 = rs1_of(instruction);
            uint8_t funct3 = funct3_of(instruction);
            int32_t imm = immI_of(instruction);

            switch(funct3) {
                case 0x0: { // ADDI: rd = rs1 + sign_extend(imm)
                    set_register(rd, regs[rs1] + static_cast<uint32_t>(imm));
                    break;
                }
                case 0x1: { // SLLI: logical shift left by imm[4:0]
                    uint32_t shamt = imm & 0x1F;
                    set_register(rd, regs[rs1] << shamt);
                    break;
                }
                case 0x2:   // SLTI: rd = (rs1 < imm) signed comparison ? 1 : 0
                    set_register(rd, (static_cast<int32_t>(regs[rs1]) < imm) ? 1 : 0);
                    break;

                case 0x3:   // SLTIU: rd = (rs1 < imm) unsigned comparison ? 1 : 0
                    set_register(rd, (regs[rs1] < static_cast<uint32_t>(imm)) ? 1 : 0);
                    break;

                case 0x4:   // XORI
                    set_register(rd, regs[rs1] ^ static_cast<uint32_t>(imm));
                    break;
                
                case 0x5: { //// SRLI or SRAI: funct7 picks which
                    uint32_t shamt = imm & 0x1F;
                    if (funct7_of(instruction) == 0x20) {
                        // SRAI: arithmetic right shift (sign-extending)
                        set_register(rd, static_cast<uint32_t>(static_cast<int32_t>(regs[rs1]) >> shamt));
                    }
                    else {
                        // SRLI: logical right shift (zero-filling)
                        set_register(rd, regs[rs1] >> shamt);
                    }
                    break;
                }
                case 0x6:   //ORI
                    set_register(rd, regs[rs1] | static_cast<uint32_t>(imm));
                    break;

                case 0x7:    //ANDI
                    set_register(rd, regs[rs1] & static_cast<uint32_t>(imm));
                    break;
            }
            break;
        }
    }
}
