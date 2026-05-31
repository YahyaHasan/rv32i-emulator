#include <gtest/gtest.h>
#include "cpu.h"

TEST(CPUSkeleton, X0IsZeroOnInit) {
    CPU cpu;
    EXPECT_EQ(cpu.get_register(0), 0u);
}

TEST(CPUSkeleton, PCIsZeroOnInit) {
    CPU cpu;
    EXPECT_EQ(cpu.get_pc(), 0u);
}

// All tests below use the encoding: ADD x1, x2, x3 = 0x003100B3
//
// Breakdown (bit ranges):
//   funct7 = 0000000   (bits 31:25)
//   rs2    = 00011     (bits 24:20, x3)
//   rs1    = 00010     (bits 19:15, x2)
//   funct3 = 000       (bits 14:12)
//   rd     = 00001     (bits 11:7,  x1)
//   opcode = 0110011   (bits 6:0,   R-type ALU = 0x33)

TEST(Decoder, ExtractsOpcode) {
    EXPECT_EQ(opcode_of(0x003100B3), 0x33);
}

TEST(Decoder, ExtractsRd) {
    EXPECT_EQ(rd_of(0x003100B3), 1);   // x1
}

TEST(Decoder, ExtractsRs1AndRs2) {
    EXPECT_EQ(rs1_of(0x003100B3), 2);  // x2
    EXPECT_EQ(rs2_of(0x003100B3), 3);  // x3
}

TEST(Decoder, ExtractsFunct3AndFunct7) {
    EXPECT_EQ(funct3_of(0x003100B3), 0);
    EXPECT_EQ(funct7_of(0x003100B3), 0);   // ADD-specific
}

TEST(Decoder, Funct7DistinguishesAddFromSub) {
    // SUB x1, x2, x3 = 0x403100B3
    // Same as ADD but funct7 = 0100000 (0x20) instead of 0000000.
    EXPECT_EQ(funct7_of(0x403100B3), 0x20);
    EXPECT_EQ(opcode_of(0x403100B3), 0x33);  // opcode unchanged
}

TEST(Decoder, HandlesIType) {
    // ADDI x1, x2, 100 = 0x06410093
    //   imm    = 000001100100 (decimal 100, bits 31:20)
    //   rs1    = 00010        (x2)
    //   funct3 = 000
    //   rd     = 00001        (x1)
    //   opcode = 0010011      (I-type arithmetic = 0x13)
    EXPECT_EQ(opcode_of(0x06410093), 0x13);
    EXPECT_EQ(rd_of(0x06410093),     1);
    EXPECT_EQ(funct3_of(0x06410093), 0);
    EXPECT_EQ(rs1_of(0x06410093),    2);
}

