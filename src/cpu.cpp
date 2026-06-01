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
                  | (static_cast<uint32_t>(memory[pc + 3]) << 24);
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
    uint32_t next_pc = pc + 4;  // default

    switch(opcode) {
        case 0x03: {    // I-type loads (LB, LH, LW, LBU, LHU)
            uint8_t rd = rd_of(instruction);
            uint8_t rs1 = rs1_of(instruction);
            uint8_t funct3 = funct3_of(instruction);
            int32_t imm = immI_of(instruction);
            uint32_t addy = regs[rs1] + static_cast<uint32_t>(imm);

            switch(funct3) {
                case 0x0: {     // LB: load 1 byte, sign-extend to 32 bits
                    int8_t byte = static_cast<int8_t>(memory[addy]);
                    set_register(rd, static_cast<uint32_t>(static_cast<int32_t>(byte)));
                    break;
                }
                case 0x1: {     // LH: load 2 bytes (little-endian), sign-extend to 32 bits
                    int16_t raw = static_cast<int16_t>(memory[addy])
                                | (static_cast<int16_t>(memory[addy + 1]) << 8);
                    int16_t half = static_cast<int16_t>(raw);
                    set_register(rd, static_cast<uint32_t>(static_cast<int32_t>(half)));
                    break;
                }
                case 0x2: {     // LW: load 4 bytes (little-endian), no extension needed
                    uint32_t word = static_cast<uint32_t>(memory[addy])
                                 | (static_cast<uint32_t>(memory[addy + 1]) << 8)
                                 | (static_cast<uint32_t>(memory[addy + 2]) << 16)
                                 | (static_cast<uint32_t>(memory[addy + 3]) << 24);
                    set_register(rd, word);
                    break;
                }
                case 0x4:       // LBU: load 1 byte, zero-extend to 32 bits
                    set_register(rd, static_cast<uint32_t>(memory[addy]));
                    break;
                case 0x5: {     // LHU: load 2 bytes, zero-extend to 32 bits
                    uint16_t half = static_cast<uint16_t>(memory[addy])
                                 | (static_cast<uint16_t>(memory[addy + 1]) << 8);
                    set_register(rd, half);
                    break;
                }
            }
            break;
        }

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

        case 0x17: {    // AUIPC: add upper immediate to pc, store in rd (for pc-relative addressing)
            uint8_t rd = rd_of(instruction);
            uint32_t imm = immU_of(instruction);
            set_register(rd, pc + imm);
            break;
        }

        case 0x23: {    // S-type stores (SB, SH, SW)
            uint8_t rs1 = rs1_of(instruction);
            uint8_t rs2 = rs2_of(instruction);
            uint8_t funct3 = funct3_of(instruction);
            int32_t imm = immS_of(instruction);
            uint32_t addy = regs[rs1] + static_cast<uint32_t>(imm);
            uint32_t rs2_value = regs[rs2];

            switch(funct3) {
                case 0x0: {     // SB: store low 8 bits of rs2
                    uint8_t byte = static_cast<uint8_t>(rs2_value & 0xFF);
                    memory[addy] = byte;
                    break;
                }
                case 0x1: {     // SH: store low 16 bits of rs2, little-endian
                    uint8_t byte1 = static_cast<uint8_t>(rs2_value & 0xFF);
                    uint8_t byte2 = static_cast<uint8_t>((rs2_value >> 8) & 0xFF);
                    memory[addy] = byte1;
                    memory[addy + 1] = byte2;
                    break;
                }
                case 0x2: {     // SW: store all 32 bits of rs2, little-endian
                    uint8_t byte1 = static_cast<uint8_t>(rs2_value & 0xFF);
                    uint8_t byte2 = static_cast<uint8_t>((rs2_value >> 8) & 0xFF);
                    uint8_t byte3 = static_cast<uint8_t>((rs2_value >> 16) & 0xFF);
                    uint8_t byte4 = static_cast<uint8_t>((rs2_value >> 24) & 0xFF);
                    memory[addy] = byte1;
                    memory[addy + 1] = byte2;
                    memory[addy + 2] = byte3;
                    memory[addy + 3] = byte4;
                    break;
                }
            }
            break;
        }
        
        case 0x33: {    // R-type register-register arithmetic
            uint8_t rd = rd_of(instruction);
            uint8_t rs1 = rs1_of(instruction);
            uint8_t rs2 = rs2_of(instruction);
            uint8_t funct3 = funct3_of(instruction);
            uint8_t funct7 = funct7_of(instruction);
            uint8_t shamt = regs[rs2] & 0x1F;   // shift amount: low 5 bits of rs2

            switch(funct3) {
                case 0x00:  // SUB/ADD
                    if (funct7 == 0x20) {   //SUB
                        set_register(rd, regs[rs1] - regs[rs2]);
                    }
                    else {  //ADD
                        set_register(rd, regs[rs1] + regs[rs2]);
                    }
                    break;
                
                case 0x1:   // SLL: shift rs1 left by low 5 bits of rs2 (zero-fill)
                    set_register(rd, regs[rs1] << shamt);
                    break;
                
                case 0x2:   // SLT: rd = 1 if rs1 < rs2 (signed comparison), else 0
                    set_register(rd, (static_cast<int32_t>(regs[rs1]) < static_cast<int32_t>(regs[rs2])) ? 1 : 0);
                    break;
                
                case 0x3:   // SLTU: rd = 1 if rs1 < rs2 (unsigned comparison), else 0
                    set_register(rd, (regs[rs1] < regs[rs2]) ? 1 : 0);
                    break;
                
                case 0x4:   // XOR: bitwise exclusive OR
                    set_register(rd, regs[rs1] ^ regs[rs2]);
                    break;
                    
                case 0x5:   // SRA/SRL
                    if (funct7 == 0x20) {   // SRA: arithmetic right shift, preserves sign bit
                        set_register(rd, static_cast<uint32_t>(static_cast<int32_t>(regs[rs1]) >> shamt));
                    }
                    else {  // SRL: logical right shift, zero-fills high bits
                        set_register(rd, regs[rs1] >> shamt);
                    }
                    break;

                case 0x6:   // OR: bitwise inclusive OR
                    set_register(rd, regs[rs1] | regs[rs2]);
                    break;
                
                case 0x7:   // AND: bitwise AND
                    set_register(rd, regs[rs1] & regs[rs2]);
                    break;
            }
            break;
        }

        case 0x37: {    // LUI: place 20-bit immediate in upper bits of rd, lower 12 bits zeroed
            uint8_t rd = rd_of(instruction);
            uint32_t imm = immU_of(instruction);
            set_register(rd, imm);
            break;
        }

        case 0x63: {    // B-type branches
            uint8_t rs1 = rs1_of(instruction);
            uint8_t rs2 = rs2_of(instruction);
            uint8_t funct3 = funct3_of(instruction);
            int32_t imm = immB_of(instruction);
            bool jump = false;

            switch(funct3) {
                case 0x0:   // BEQ: branch if rs1 == rs2
                    jump = regs[rs1] == regs[rs2];
                    break;
                case 0x1:   // BNE: branch if rs1 != rs2
                    jump = regs[rs1] != regs[rs2];
                    break;
                case 0x4:   // BLT: branch if rs1 < rs2 (signed)
                    jump = static_cast<int32_t>(regs[rs1]) < static_cast<int32_t>(regs[rs2]);
                    break;
                case 0x5:   // BGE: branch if rs1 >= rs2 (signed)
                    jump = static_cast<int32_t>(regs[rs1]) >= static_cast<int32_t>(regs[rs2]);
                    break;
                case 0x6:   // BLTU: branch if rs1 < rs2 (unsigned)
                    jump = regs[rs1] < regs[rs2];
                    break;
                case 0x7:   // BGEU: branch if rs1 >= rs2 (unsigned)
                    jump = regs[rs1] >= regs[rs2];
                    break;
            }
            if (jump) {
                next_pc = pc + static_cast<uint32_t>(imm);
            }
            break;
        }

        case 0x6F: {    // JAL: jump to pc + imm, save return address in rd
            uint8_t rd = rd_of(instruction);
            int32_t imm = immJ_of(instruction);
            set_register(rd, next_pc);
            next_pc = pc + static_cast<uint32_t>(imm);
            break;
        }

        case 0x67: {    // JALR: jump to (rs1 + imm) with LSB cleared, save return address in rd
            uint8_t rd = rd_of(instruction);
            uint8_t rs1 = rs1_of(instruction);
            int32_t imm = immI_of(instruction);
            uint32_t jump_loc = (regs[rs1] + static_cast<uint32_t>(imm)) & ~1u;
            set_register(rd, next_pc);
            next_pc = jump_loc;
            break;
        }

        case 0x73: {    // SYSTEM opcode (ECALL, EBREAK)
            uint32_t funct12 = (instruction >> 20) & 0xFFF;
            if (funct12 == 0x0) {
                // ECALL: environment call (syscall entry point).
                // Phase 2 will inspect a7/x17 and dispatch real syscalls.
                // For now, halt so the run loop knows execution is done.
                halted = true;
            }
            else if (funct12 == 0x1) {
                // EBREAK: debugger breakpoint. Halt.
                halted = true;
            }
            break;
        }
    }

    pc = next_pc;
}
