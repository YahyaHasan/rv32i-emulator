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

// ---- Loads ----

TEST(LW, ReadsLittleEndianWord) {
    CPU cpu;
    // Write 0xDEADBEEF at address 0x100 in little-endian order
    cpu.write_byte(0x100, 0xEF);
    cpu.write_byte(0x101, 0xBE);
    cpu.write_byte(0x102, 0xAD);
    cpu.write_byte(0x103, 0xDE);
    cpu.execute(0x10000093);  // ADDI x1, x0, 0x100
    cpu.execute(0x0000A103);  // LW x2, 0(x1)
    EXPECT_EQ(cpu.get_register(2), 0xDEADBEEFu);
}

TEST(LB, SignExtendsNegativeByte) {
    CPU cpu;
    cpu.write_byte(0x100, 0xFF);  // -1 as int8_t
    cpu.execute(0x10000093);      // x1 = 0x100
    cpu.execute(0x00008103);      // LB x2, 0(x1)
    EXPECT_EQ(cpu.get_register(2), 0xFFFFFFFFu);
}

TEST(LBU, ZeroExtendsByte) {
    // Same source byte as LB test, different funct3, different result.
    CPU cpu;
    cpu.write_byte(0x100, 0xFF);
    cpu.execute(0x10000093);      // x1 = 0x100
    cpu.execute(0x0000C103);      // LBU x2, 0(x1)
    EXPECT_EQ(cpu.get_register(2), 0xFFu);
}

TEST(LH, SignExtendsNegativeHalf) {
    CPU cpu;
    cpu.write_byte(0x100, 0x00);
    cpu.write_byte(0x101, 0x80);  // 0x8000 = -32768 as int16_t
    cpu.execute(0x10000093);      // x1 = 0x100
    cpu.execute(0x00009103);      // LH x2, 0(x1)
    EXPECT_EQ(cpu.get_register(2), 0xFFFF8000u);
}

TEST(LHU, ZeroExtendsHalf) {
    CPU cpu;
    cpu.write_byte(0x100, 0x00);
    cpu.write_byte(0x101, 0x80);
    cpu.execute(0x10000093);      // x1 = 0x100
    cpu.execute(0x0000D103);      // LHU x2, 0(x1)
    EXPECT_EQ(cpu.get_register(2), 0x00008000u);
}

TEST(Load, UsesNegativeOffset) {
    CPU cpu;
    cpu.write_byte(0x100, 0x42);
    cpu.execute(0x10400093);      // x1 = 0x104
    cpu.execute(0xFFC0C103);      // LBU x2, -4(x1)  → reads memory[0x100]
    EXPECT_EQ(cpu.get_register(2), 0x42u);
}

// ---- S-type immediate ----

TEST(Decoder, ExtractsPositiveSImmediate) {
    // SW x2, 4(x1) = 0x00208223 → imm = 4
    EXPECT_EQ(immS_of(0x00208223), 4);
}

TEST(Decoder, ExtractsNegativeSImmediate) {
    // SB x2, -4(x1) = 0xFE208E23 → imm = -4
    EXPECT_EQ(immS_of(0xFE208E23), -4);
}

// ---- Stores ----

TEST(SB, WritesLowByte) {
    CPU cpu;
    cpu.execute(0x10000093);  // x1 = 0x100
    cpu.execute(0x0FF00113);  // x2 = 0xFF
    cpu.execute(0x00208023);  // SB x2, 0(x1)
    EXPECT_EQ(cpu.read_byte(0x100), 0xFFu);
}

TEST(SB, IgnoresUpperBitsOfSource) {
    CPU cpu;
    cpu.execute(0x10000093);  // x1 = 0x100
    cpu.execute(0xFFF00113);  // x2 = 0xFFFFFFFF
    cpu.execute(0x00208023);  // SB x2, 0(x1)
    EXPECT_EQ(cpu.read_byte(0x100), 0xFFu);
    EXPECT_EQ(cpu.read_byte(0x101), 0x00u);   // adjacent byte untouched
}

TEST(SH, WritesLittleEndianHalfword) {
    CPU cpu;
    cpu.execute(0x10000093);  // x1 = 0x100
    cpu.execute(0xFFF00113);  // x2 = 0xFFFFFFFF
    cpu.execute(0x00209023);  // SH x2, 0(x1)
    EXPECT_EQ(cpu.read_byte(0x100), 0xFFu);
    EXPECT_EQ(cpu.read_byte(0x101), 0xFFu);
    EXPECT_EQ(cpu.read_byte(0x102), 0x00u);
}

TEST(SW, WritesAllFourBytes) {
    CPU cpu;
    cpu.execute(0x10000093);  // x1 = 0x100
    cpu.execute(0xFFF00113);  // x2 = 0xFFFFFFFF
    cpu.execute(0x0020A023);  // SW x2, 0(x1)
    EXPECT_EQ(cpu.read_byte(0x100), 0xFFu);
    EXPECT_EQ(cpu.read_byte(0x101), 0xFFu);
    EXPECT_EQ(cpu.read_byte(0x102), 0xFFu);
    EXPECT_EQ(cpu.read_byte(0x103), 0xFFu);
}

TEST(SW, PreservesByteOrderEndToEnd) {
    // Read a known 4-byte sequence in via LW, then write it back via SW.
    // The output bytes must match the input bytes in the same order.
    CPU cpu;
    cpu.write_byte(0x200, 0x78);
    cpu.write_byte(0x201, 0x56);
    cpu.write_byte(0x202, 0x34);
    cpu.write_byte(0x203, 0x12);
    cpu.execute(0x10000093);  // x1 = 0x100
    cpu.execute(0x20000113);  // x2 = 0x200
    cpu.execute(0x00012183);  // LW x3, 0(x2)   → x3 = 0x12345678
    cpu.execute(0x0030A023);  // SW x3, 0(x1)
    EXPECT_EQ(cpu.read_byte(0x100), 0x78u);
    EXPECT_EQ(cpu.read_byte(0x101), 0x56u);
    EXPECT_EQ(cpu.read_byte(0x102), 0x34u);
    EXPECT_EQ(cpu.read_byte(0x103), 0x12u);
}

TEST(SB, WritesAtNegativeOffset) {
    CPU cpu;
    cpu.execute(0x10400093);  // x1 = 0x104
    cpu.execute(0x07B00113);  // x2 = 123
    cpu.execute(0xFE208E23);  // SB x2, -4(x1)  → writes to memory[0x100]
    EXPECT_EQ(cpu.read_byte(0x100), 123u);
}

// ---- B-type immediate ----

TEST(Decoder, ExtractsPositiveBImmediate) {
    // BEQ x1, x2, +8 = 0x00208463
    EXPECT_EQ(immB_of(0x00208463), 8);
}

TEST(Decoder, ExtractsNegativeBImmediate) {
    // BEQ x1, x2, -8 = 0xFE208CE3
    EXPECT_EQ(immB_of(0xFE208CE3), -8);
}

// ---- Branches ----

TEST(BEQ, TakenWhenEqual) {
    CPU cpu;
    cpu.execute(0x00500093);  // x1 = 5
    cpu.execute(0x00500113);  // x2 = 5
    // After the two ADDIs, pc has advanced to 8.
    cpu.execute(0x00208463);  // BEQ x1, x2, +8  → next_pc = 8 + 8 = 16
    EXPECT_EQ(cpu.get_pc(), 16u);
}

TEST(BEQ, NotTakenWhenUnequal) {
    CPU cpu;
    cpu.execute(0x00500093);  // x1 = 5
    cpu.execute(0x00300113);  // x2 = 3
    // pc is now 8. Branch should fall through to pc + 4 = 12.
    cpu.execute(0x00208463);  // BEQ x1, x2, +8 (not taken)
    EXPECT_EQ(cpu.get_pc(), 12u);
}

TEST(BNE, TakenWhenUnequal) {
    CPU cpu;
    cpu.execute(0x00500093);  // x1 = 5
    cpu.execute(0x00300113);  // x2 = 3
    cpu.execute(0x00209463);  // BNE x1, x2, +8
    EXPECT_EQ(cpu.get_pc(), 16u);
}

TEST(BLT, TakenForNegativeRs1) {
    CPU cpu;
    cpu.execute(0xFFF00093);  // x1 = -1
    cpu.execute(0x00500113);  // x2 = 5
    cpu.execute(0x0020C463);  // BLT x1, x2, +8  (signed: -1 < 5)
    EXPECT_EQ(cpu.get_pc(), 16u);
}

TEST(BLTU_vs_BLT, DiffersOnNegativeRs1) {
    // Same registers, same offset; BLTU and BLT disagree because BLTU treats
    // 0xFFFFFFFF as huge (unsigned) while BLT treats it as -1 (signed).
    CPU cpu;
    cpu.execute(0xFFF00093);  // x1 = 0xFFFFFFFF (-1 signed)
    cpu.execute(0x00500113);  // x2 = 5
    // pc = 8.
    cpu.execute(0x0020E463);  // BLTU x1, x2, +8 (unsigned: 0xFFFFFFFF > 5, not taken)
    EXPECT_EQ(cpu.get_pc(), 12u);
}

TEST(BGE, TakenForGreaterSigned) {
    CPU cpu;
    cpu.execute(0x00500093);  // x1 = 5
    cpu.execute(0xFFF00113);  // x2 = -1
    cpu.execute(0x0020D463);  // BGE x1, x2, +8  (signed: 5 >= -1)
    EXPECT_EQ(cpu.get_pc(), 16u);
}

TEST(BGEU, NotTakenWhenRs1IsZero) {
    CPU cpu;
    // Without any setup, x0 stays 0 and x1 starts at 0.
    cpu.execute(0x00100113);  // x2 = 1
    cpu.execute(0x0020F063);  // BGEU x1, x2, +0  (unsigned: 0 >= 1 is false)
    EXPECT_EQ(cpu.get_pc(), 8u);  // fall through, pc was 4 → +4 = 8
}

TEST(BEQ, BranchesBackward) {
    CPU cpu;
    cpu.execute(0x00500093);  // x1 = 5;    pc = 0 → 4
    cpu.execute(0x00500113);  // x2 = 5;    pc = 4 → 8
    cpu.execute(0xFE208CE3);  // BEQ x1, x2, -8 (taken) → next_pc = 8 + (-8) = 0
    EXPECT_EQ(cpu.get_pc(), 0u);
}

// ---- J-type immediate ----

TEST(Decoder, ExtractsPositiveJImmediate) {
    // JAL x1, 0x100 = 0x100000EF
    EXPECT_EQ(immJ_of(0x100000EF), 0x100);
}

TEST(Decoder, ExtractsNegativeJImmediate) {
    // JAL x1, -4 = 0xFFDFF0EF
    EXPECT_EQ(immJ_of(0xFFDFF0EF), -4);
}

// ---- JAL ----

TEST(JAL, JumpsAndSavesReturnAddress) {
    CPU cpu;
    // pc = 0. JAL x1, 0x100. After: x1 = 4 (return addr), pc = 0x100.
    cpu.execute(0x100000EF);  // JAL x1, 0x100
    EXPECT_EQ(cpu.get_register(1), 4u);
    EXPECT_EQ(cpu.get_pc(), 0x100u);
}

TEST(JAL, JumpsBackward) {
    CPU cpu;
    cpu.execute(0x00500093);  // ADDI x1, x0, 5   (pc: 0 → 4)
    cpu.execute(0xFFDFF0EF);  // JAL x1, -4        (pc: 4 → 0)
    EXPECT_EQ(cpu.get_pc(), 0u);
}

TEST(JAL, WritesToX0Discarded) {
    CPU cpu;
    cpu.execute(0x1000006F);  // JAL x0, 0x100
    EXPECT_EQ(cpu.get_register(0), 0u);   // return address discarded
    EXPECT_EQ(cpu.get_pc(), 0x100u);      // jump still happens
}

// ---- JALR ----

TEST(JALR, JumpsToRegisterPlusOffset) {
    CPU cpu;
    cpu.execute(0x20000113);  // ADDI x2, x0, 0x200  (pc: 0 → 4)
    cpu.execute(0x000100E7);  // JALR x1, x2, 0      (target = 0x200)
    EXPECT_EQ(cpu.get_register(1), 8u);     // return address = pc(4) + 4
    EXPECT_EQ(cpu.get_pc(), 0x200u);
}

TEST(JALR, ClearsLSBOfTarget) {
    CPU cpu;
    cpu.execute(0x00300113);  // ADDI x2, x0, 3    (odd value)
    cpu.execute(0x000100E7);  // JALR x1, x2, 0    (target = 3 & ~1 = 2)
    EXPECT_EQ(cpu.get_pc(), 2u);
}

TEST(JALR, CallAndReturn) {
    // Simulates a function call (JAL) and return (JALR ra, 0).
    CPU cpu;
    cpu.execute(0x100000EF);  // JAL x1, 0x100   → x1 = 4, pc = 0x100
    EXPECT_EQ(cpu.get_pc(), 0x100u);
    EXPECT_EQ(cpu.get_register(1), 4u);

    cpu.execute(0x000080E7);  // JALR x1, x1, 0  → pc = x1 = 4 (return to caller)
    EXPECT_EQ(cpu.get_pc(), 4u);
}

// ---- LUI ----

TEST(LUI, LoadsUpperBits) {
    CPU cpu;
    cpu.execute(0x123450B7);  // LUI x1, 0x12345
    EXPECT_EQ(cpu.get_register(1), 0x12345000u);
}

TEST(LUI, SetsHighBit) {
    CPU cpu;
    cpu.execute(0xFFFFF0B7);  // LUI x1, 0xFFFFF
    EXPECT_EQ(cpu.get_register(1), 0xFFFFF000u);
}

TEST(LUI_ADDI, Builds32BitConstant) {
    // The standard RISC-V pattern for loading a full 32-bit value.
    // Compilers emit this pair for any constant that doesn't fit in 12 bits.
    CPU cpu;
    cpu.execute(0x123450B7);  // LUI  x1, 0x12345    → x1 = 0x12345000
    cpu.execute(0x67808093);  // ADDI x1, x1, 0x678  → x1 = 0x12345678
    EXPECT_EQ(cpu.get_register(1), 0x12345678u);
}

// ---- AUIPC ----

TEST(AUIPC, AddsPCToUpperImmediate) {
    CPU cpu;
    cpu.execute(0x00500093);  // ADDI x1, x0, 5     (pc: 0 → 4)
    cpu.execute(0x12345117);  // AUIPC x2, 0x12345   → x2 = 4 + 0x12345000
    EXPECT_EQ(cpu.get_register(2), 0x12345004u);
}

