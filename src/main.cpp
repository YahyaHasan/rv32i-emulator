#include "cpu.h"
#include <iostream>

int main() {
    CPU cpu;
    std::cout << "RISC-V RV32I emulator: skeleton\n";
    std::cout << "Initial pc = 0x" << std::hex << cpu.get_pc() << std::dec << "\n";
    return 0;
}