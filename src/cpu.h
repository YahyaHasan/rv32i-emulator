#pragma once

#include <array>
#include <cstdint>

inline uint8_t opcode_of(uint32_t instr) {return instr & 0x7F; }
inline uint8_t rd_of(uint32_t instr) {return (instr >> 7) & 0x1F; }
inline uint8_t funct3_of(uint32_t instr) {return (instr >> 12) & 0x07; }
inline uint8_t rs1_of(uint32_t instr) {return (instr >> 15) & 0x1F; }
inline uint8_t rs2_of(uint32_t instr) {return (instr >> 20) & 0x1F; }
inline uint8_t funct7_of(uint32_t instr) {return (instr >> 25) & 0x7F; }

inline int32_t immI_of(uint32_t instr) {    // imm for I-types
    return static_cast<int32_t>(instr) >> 20;
}

inline int32_t immS_of(uint32_t instr) {    // imm for S-types
    // S-type immediate: imm[11:5] at bits 31:25 of instr, imm[4:0] at bits 11:7
    uint32_t hi = (instr >> 25) & 0x7F;     // imm[11:5]
    uint32_t lo = (instr >> 7) & 0x1F;      // imm[4:0]
    uint32_t raw = (hi << 5) | lo;
    return static_cast<int32_t>(raw << 20) >> 20;   // sign-extension
}

inline int32_t immB_of(uint32_t instr) {    // imm for B-types
    uint32_t bit12 = (instr >> 31) & 0x1;
    uint32_t bit11 = (instr >> 7) & 0x1;
    uint32_t bits10_5 = (instr >> 25) & 0x3F;
    uint32_t bits4_1 = (instr >> 8) & 0xF;
    uint32_t raw = (bit12 << 12) | (bit11 << 11) | (bits10_5 << 5) | (bits4_1 << 1);
    return static_cast<int32_t>(raw << 19) >> 19;
}

inline int32_t immJ_of(uint32_t instr) {    // imm for J-types
    uint32_t bit20 = (instr >> 31) & 0x1;
    uint32_t bits19_12 = (instr >> 12) & 0xFF;
    uint32_t bit11 = (instr >> 20) & 0x1;
    uint32_t bits10_1 = (instr >> 21) & 0x3FF;
    uint32_t raw = (bit20 << 20) | (bits19_12 << 12) | (bit11 << 11) | (bits10_1 << 1);
    return static_cast<int32_t>(raw << 11) >> 11;
}

inline uint32_t immU_of(uint32_t instr) {   // imm for U-types
    return instr & 0xFFFFF000;
}

class CPU {
    public:
        CPU();
        void cycle();
        void execute(uint32_t instruction);
        uint32_t get_pc() const {return pc; }
        uint32_t get_register(uint8_t index) const;

        // helper for tests; write a single byte into memory
        void write_byte(uint32_t addy, uint8_t value) {memory[addy] = value; }

        // helper for tests: read a single byte from  memory
        uint8_t read_byte(uint32_t addy) const {return memory[addy]; }

        bool is_halted() const { return halted; }

    private:
        // read 4 bytes from memory at pc, little-endian
        uint32_t fetch();
        
        // x0 through x31: 32 general-purpose 32-bit registers.
        std::array<uint32_t, 32> regs{};

        //1 MiB of simulated memory
        std::array<uint32_t, 1024 * 1024> memory{};
        uint32_t pc = 0;

        //Write to register. Every instruction that writes a register must go through this function.
        void set_register(uint8_t index, uint32_t value);

        bool halted = false;    // set by ECALL/EBREAK; main loop checks this to stop cycling
};
