#pragma once

#include <array>
#include <cstdint>

class CPU {
    public:
        CPU();
        void cycle();
        void execute(uint32_t instruction);
        uint32_t get_pc() const {return pc; }
        uint32_t get_register(uint8_t index) const;

    private:
        // read 4 bytes from memory at pc, little-endian
        uint32_t fetch();
        
        // x0 through x31: 32 general-purpose 32-bit registers.
        std::array<uint32_t, 32> regs{};

        //1 MiB of simulated memory
        std::array<uint32_t, 1024 * 1024> memory{};
        uint32_t pc = 0;
};