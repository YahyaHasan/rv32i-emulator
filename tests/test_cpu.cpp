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

// ---- I-type immediate extraction ----
TEST(Decoder, ExtractsPositiveIImmediate) {
    // ADDI x1, x2, 100 = 0x06410093
    EXPECT_EQ(immI_of(0x06410093), 100);
}

TEST(Decoder, ExtractsNegativeIImmediate) {
    // ADDI x1, x2, -1 = 0xFFF10093
    // 12-bit imm field = 0xFFF, which is -1 in 12-bit two's complement;
    // must sign-extend to full 0xFFFFFFFF when read back.
    EXPECT_EQ(immI_of(0xFFF10093), -1);
}

TEST(Decoder, ExtractsMaxPositiveIImmediate) {
    // 2047 = 0x7FF, the largest positive 12-bit signed value
    EXPECT_EQ(immI_of(0x7FF10093), 2047);
}

TEST(Decoder, ExtractsMinNegativeIImmediate) {
    // -2048 = 0x800, the smallest negative 12-bit signed value
    EXPECT_EQ(immI_of(0x80010093), -2048);
}

// ---- ADDI ----

TEST(ADDI, AddsPositiveImmediate) {
    CPU cpu;
    // Load x2 = 5, then ADDI x1, x2, 3, expect x1 = 8
    cpu.execute(0x00500113);  // ADDI x2, x0, 5
    cpu.execute(0x00310093);  // ADDI x1, x2, 3
    EXPECT_EQ(cpu.get_register(1), 8u);
}

TEST(ADDI, AddsNegativeImmediate) {
    CPU cpu;
    // x2 = 10, then ADDI x1, x2, -3, expect x1 = 7
    cpu.execute(0x00A00113);  // ADDI x2, x0, 10
    cpu.execute(0xFFD10093);  // ADDI x1, x2, -3
    EXPECT_EQ(cpu.get_register(1), 7u);
}

TEST(ADDI, WritesToX0AreIgnored) {
    CPU cpu;
    // ADDI x0, x0, 42 must not change x0
    cpu.execute(0x02A00013);
    EXPECT_EQ(cpu.get_register(0), 0u);
}

TEST(ADDI, LoadImmediateIdiom) {
    CPU cpu;
    // The standard RISC-V "load immediate" pseudo-instruction:
    // ADDI rd, x0, imm — uses x0's zero value as the source so rd ends up as imm.
    cpu.execute(0x07B00093);  // ADDI x1, x0, 123
    EXPECT_EQ(cpu.get_register(1), 123u);
}

// ---- SLTI / SLTIU ----

TEST(SLTI, SignedLessThanReturnsOne) {
    CPU cpu;
    cpu.execute(0xFFF00113);  // ADDI x2, x0, -1   (x2 = -1)
    cpu.execute(0x00512093);  // SLTI x1, x2, 5    (signed: -1 < 5)
    EXPECT_EQ(cpu.get_register(1), 1u);
}

TEST(SLTIU, UnsignedComparisonTreatsNegativeAsLarge) {
    CPU cpu;
    cpu.execute(0xFFF00113);  // ADDI x2, x0, -1   (x2 = -1 = 0xFFFFFFFF unsigned)
    cpu.execute(0x00513093);  // SLTIU x1, x2, 5   (unsigned: 0xFFFFFFFF >= 5)
    EXPECT_EQ(cpu.get_register(1), 0u);
}

// ---- XORI / ORI / ANDI ----

TEST(XORI, NegOneInvertsAllBits) {
    CPU cpu;
    cpu.execute(0x00F00113);  // ADDI x2, x0, 15   (x2 = 0x0F)
    cpu.execute(0xFFF14093);  // XORI x1, x2, -1   (NOT idiom)
    EXPECT_EQ(cpu.get_register(1), 0xFFFFFFF0u);
}

TEST(ANDI, MasksLowByte) {
    CPU cpu;
    cpu.execute(0xFFF00113);  // ADDI x2, x0, -1   (x2 = 0xFFFFFFFF)
    cpu.execute(0x0FF17093);  // ANDI x1, x2, 0xFF (keep low byte only)
    EXPECT_EQ(cpu.get_register(1), 0xFFu);
}

TEST(ORI, SetsHighNibble) {
    CPU cpu;
    cpu.execute(0x00F00113);  // ADDI x2, x0, 15   (x2 = 0x0F)
    cpu.execute(0x00F16093);  // ORI x1, x2, 0x0F
    EXPECT_EQ(cpu.get_register(1), 0x0Fu);
}

// ---- Shifts ----

TEST(SLLI, ShiftsLeftByImmediate) {
    CPU cpu;
    cpu.execute(0x00100113);  // ADDI x2, x0, 1
    cpu.execute(0x00411093);  // SLLI x1, x2, 4    (1 << 4 = 16)
    EXPECT_EQ(cpu.get_register(1), 16u);
}

TEST(SRLI_vs_SRAI, DiffersOnNegativeInput) {
    CPU cpu;
    cpu.execute(0xFFF00113);  // ADDI x2, x0, -1   (x2 = 0xFFFFFFFF)
    cpu.execute(0x00415093);  // SRLI x1, x2, 4    (logical: 0x0FFFFFFF)
    EXPECT_EQ(cpu.get_register(1), 0x0FFFFFFFu);

    cpu.execute(0x40415093);  // SRAI x1, x2, 4    (arithmetic: stays 0xFFFFFFFF)
    EXPECT_EQ(cpu.get_register(1), 0xFFFFFFFFu);
}

// ---- R-type arithmetic ----

TEST(ADD, AddsRegisters) {
    CPU cpu;
    cpu.execute(0x00500093);  // ADDI x1, x0, 5
    cpu.execute(0x00300113);  // ADDI x2, x0, 3
    cpu.execute(0x002081B3);  // ADD  x3, x1, x2
    EXPECT_EQ(cpu.get_register(3), 8u);
}

TEST(SUB, SubtractsRegisters) {
    CPU cpu;
    cpu.execute(0x00500093);  // x1 = 5
    cpu.execute(0x00300113);  // x2 = 3
    cpu.execute(0x402081B3);  // SUB x3, x1, x2 = 2
    EXPECT_EQ(cpu.get_register(3), 2u);
}

TEST(SUB, WrapsOnUnderflow) {
    CPU cpu;
    cpu.execute(0x00300093);  // x1 = 3
    cpu.execute(0x00500113);  // x2 = 5
    cpu.execute(0x402081B3);  // SUB x3, x1, x2 = 3 - 5 wraps to 0xFFFFFFFE
    EXPECT_EQ(cpu.get_register(3), 0xFFFFFFFEu);
}

TEST(SLL, ShiftsByRegisterValue) {
    CPU cpu;
    cpu.execute(0x00100093);  // x1 = 1
    cpu.execute(0x00400113);  // x2 = 4
    cpu.execute(0x002091B3);  // SLL x3, x1, x2 = 16
    EXPECT_EQ(cpu.get_register(3), 16u);
}

TEST(SLT, SignedLessThan) {
    CPU cpu;
    cpu.execute(0xFFF00093);  // x1 = -1
    cpu.execute(0x00500113);  // x2 = 5
    cpu.execute(0x0020A1B3);  // SLT x3, x1, x2 = 1  (signed: -1 < 5)
    EXPECT_EQ(cpu.get_register(3), 1u);
}

TEST(SLTU, UnsignedTreatsNegativeAsLarge) {
    CPU cpu;
    cpu.execute(0xFFF00093);  // x1 = 0xFFFFFFFF
    cpu.execute(0x00500113);  // x2 = 5
    cpu.execute(0x0020B1B3);  // SLTU x3, x1, x2 = 0  (unsigned: 0xFFFFFFFF > 5)
    EXPECT_EQ(cpu.get_register(3), 0u);
}

TEST(XOR, BitwiseXor) {
    CPU cpu;
    cpu.execute(0x0AA00093);  // x1 = 0xAA
    cpu.execute(0x0FF00113);  // x2 = 0xFF
    cpu.execute(0x0020C1B3);  // XOR x3, x1, x2 = 0x55
    EXPECT_EQ(cpu.get_register(3), 0x55u);
}

TEST(SRL_vs_SRA, DiffersOnNegativeInput) {
    // Same source, same shift amount, different funct7. The contrast is the point.
    CPU cpu;
    cpu.execute(0xFFF00093);  // x1 = 0xFFFFFFFF
    cpu.execute(0x00400113);  // x2 = 4
    cpu.execute(0x0020D1B3);  // SRL x3, x1, x2 = 0x0FFFFFFF (zero-fills)
    EXPECT_EQ(cpu.get_register(3), 0x0FFFFFFFu);

    cpu.execute(0x4020D1B3);  // SRA x3, x1, x2 = 0xFFFFFFFF (sign-extends)
    EXPECT_EQ(cpu.get_register(3), 0xFFFFFFFFu);
}

TEST(OR, BitwiseOr) {
    CPU cpu;
    cpu.execute(0x00F00093);  // x1 = 0x0F
    cpu.execute(0x0F000113);  // x2 = 0xF0
    cpu.execute(0x0020E1B3);  // OR x3, x1, x2 = 0xFF
    EXPECT_EQ(cpu.get_register(3), 0xFFu);
}

TEST(AND, BitwiseAnd) {
    CPU cpu;
    cpu.execute(0xFFF00093);  // x1 = 0xFFFFFFFF
    cpu.execute(0x0F000113);  // x2 = 0xF0
    cpu.execute(0x0020F1B3);  // AND x3, x1, x2 = 0xF0
    EXPECT_EQ(cpu.get_register(3), 0xF0u);
}

