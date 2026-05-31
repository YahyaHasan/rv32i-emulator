#pragma once

#include <array>
#include <cstdint>

inline uint8_t opcode_of(uint32_t instr) {return instr & 0x7F; }
inline uint8_t rd_of(uint32_t instr) {return (instr >> 7) & 0x1F; }
inline uint8_t funct3_of(uint32_t instr) {return (instr >> 12) & 0x07; }
inline uint8_t rs1_of(uint32_t instr) {return (instr >> 15) & 0x1F; }
inline uint8_t rs2_of(uint32_t instr) {return (instr >> 20) & 0x1F; }
inline uint8_t funct7_of(uint32_t instr) {return (instr >> 25) & 0x7F; }

inline int32_t immI_of(uint32_t instr) { return static_cast<int32_t>(instr) >> 20; }


class CPU {
    public:
        CPU();
        void cycle();
        void execute(uint32_t instruction);
        uint32_t get_pc() const {return pc; }
        uint32_t get_register(uint8_t index) const;

        // helper for tests; poke a single byte into the memory array.
        void write_byte(uint32_t addy, uint8_t value) {memory[addy] = value; }

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
};
