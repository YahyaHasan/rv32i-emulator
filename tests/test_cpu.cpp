#include <gtest/gtest.h>
#include "cpu.h"

// ---- Initialization ----

TEST(CPU, Initialization) {
    CPU cpu;
    EXPECT_EQ(cpu.get_register(0), 0u);
    EXPECT_EQ(cpu.get_pc(), 0u);
}

// ---- Decoder: field extraction and all immediate formats ----

TEST(Decoder, FieldsAndImmediates) {
    // R-type fields (ADD x1, x2, x3 = 0x003100B3)
    EXPECT_EQ(opcode_of(0x003100B3), 0x33u);
    EXPECT_EQ(rd_of    (0x003100B3),    1u);
    EXPECT_EQ(rs1_of   (0x003100B3),    2u);
    EXPECT_EQ(rs2_of   (0x003100B3),    3u);
    EXPECT_EQ(funct3_of(0x003100B3),    0u);
    EXPECT_EQ(funct7_of(0x003100B3),    0u);
    EXPECT_EQ(funct7_of(0x403100B3), 0x20u);  // SUB differs from ADD only in funct7

    // I-type fields (ADDI x1, x2, 100 = 0x06410093)
    EXPECT_EQ(opcode_of(0x06410093), 0x13u);
    EXPECT_EQ(rd_of    (0x06410093),    1u);
    EXPECT_EQ(funct3_of(0x06410093),    0u);
    EXPECT_EQ(rs1_of   (0x06410093),    2u);

    // I-type immediate: positive, negative, boundary values
    EXPECT_EQ(immI_of(0x06410093),   100);
    EXPECT_EQ(immI_of(0xFFF10093),    -1);
    EXPECT_EQ(immI_of(0x7FF10093),  2047);
    EXPECT_EQ(immI_of(0x80010093), -2048);

    // S-type immediate (split across bits 31:25 and 11:7)
    EXPECT_EQ(immS_of(0x00208223),   4);
    EXPECT_EQ(immS_of(0xFE208E23),  -4);

    // B-type immediate (scrambled bits, implicit zero at bit 0)
    EXPECT_EQ(immB_of(0x00208463),   8);
    EXPECT_EQ(immB_of(0xFE208CE3),  -8);

    // J-type immediate (most scrambled format)
    EXPECT_EQ(immJ_of(0x100000EF), 0x100);
    EXPECT_EQ(immJ_of(0xFFDFF0EF),    -4);
}

// ---- I-type arithmetic ----

TEST(IType, AllOps) {
    // ADDI: positive, negative, x0-write guard, load-immediate idiom
    { CPU cpu;
      cpu.execute(0x00500113); cpu.execute(0x00310093);
      EXPECT_EQ(cpu.get_register(1), 8u); }

    { CPU cpu;
      cpu.execute(0x00A00113); cpu.execute(0xFFD10093);
      EXPECT_EQ(cpu.get_register(1), 7u); }

    { CPU cpu;
      cpu.execute(0x02A00013);
      EXPECT_EQ(cpu.get_register(0), 0u); }

    { CPU cpu;
      cpu.execute(0x07B00093);
      EXPECT_EQ(cpu.get_register(1), 123u); }

    // SLTI (signed) / SLTIU (unsigned) — same input, opposite results
    { CPU cpu;
      cpu.execute(0xFFF00113); cpu.execute(0x00512093);
      EXPECT_EQ(cpu.get_register(1), 1u); }  // signed: -1 < 5

    { CPU cpu;
      cpu.execute(0xFFF00113); cpu.execute(0x00513093);
      EXPECT_EQ(cpu.get_register(1), 0u); }  // unsigned: 0xFFFFFFFF >= 5

    // XORI, ANDI, ORI
    { CPU cpu;
      cpu.execute(0x00F00113); cpu.execute(0xFFF14093);
      EXPECT_EQ(cpu.get_register(1), 0xFFFFFFF0u); }  // XOR with -1 = NOT

    { CPU cpu;
      cpu.execute(0xFFF00113); cpu.execute(0x0FF17093);
      EXPECT_EQ(cpu.get_register(1), 0xFFu); }

    { CPU cpu;
      cpu.execute(0x00F00113); cpu.execute(0x00F16093);
      EXPECT_EQ(cpu.get_register(1), 0x0Fu); }

    // SLLI / SRLI (logical, zero-fills) / SRAI (arithmetic, sign-extends)
    { CPU cpu;
      cpu.execute(0x00100113); cpu.execute(0x00411093);
      EXPECT_EQ(cpu.get_register(1), 16u); }

    { CPU cpu;
      cpu.execute(0xFFF00113);
      cpu.execute(0x00415093);
      EXPECT_EQ(cpu.get_register(1), 0x0FFFFFFFu);  // SRLI: zero-fills
      cpu.execute(0x40415093);
      EXPECT_EQ(cpu.get_register(1), 0xFFFFFFFFu); }  // SRAI: sign-extends
}

// ---- R-type arithmetic ----

TEST(RType, AllOps) {
    // ADD
    { CPU cpu;
      cpu.execute(0x00500093); cpu.execute(0x00300113); cpu.execute(0x002081B3);
      EXPECT_EQ(cpu.get_register(3), 8u); }

    // SUB: normal and underflow wrap
    { CPU cpu;
      cpu.execute(0x00500093); cpu.execute(0x00300113); cpu.execute(0x402081B3);
      EXPECT_EQ(cpu.get_register(3), 2u); }

    { CPU cpu;
      cpu.execute(0x00300093); cpu.execute(0x00500113); cpu.execute(0x402081B3);
      EXPECT_EQ(cpu.get_register(3), 0xFFFFFFFEu); }

    // SLL
    { CPU cpu;
      cpu.execute(0x00100093); cpu.execute(0x00400113); cpu.execute(0x002091B3);
      EXPECT_EQ(cpu.get_register(3), 16u); }

    // SLT (signed) / SLTU (unsigned) — same input, opposite results
    { CPU cpu;
      cpu.execute(0xFFF00093); cpu.execute(0x00500113); cpu.execute(0x0020A1B3);
      EXPECT_EQ(cpu.get_register(3), 1u); }  // signed: -1 < 5

    { CPU cpu;
      cpu.execute(0xFFF00093); cpu.execute(0x00500113); cpu.execute(0x0020B1B3);
      EXPECT_EQ(cpu.get_register(3), 0u); }  // unsigned: 0xFFFFFFFF > 5

    // XOR
    { CPU cpu;
      cpu.execute(0x0AA00093); cpu.execute(0x0FF00113); cpu.execute(0x0020C1B3);
      EXPECT_EQ(cpu.get_register(3), 0x55u); }

    // SRL (logical) vs SRA (arithmetic) — same input, different funct7
    { CPU cpu;
      cpu.execute(0xFFF00093); cpu.execute(0x00400113);
      cpu.execute(0x0020D1B3);
      EXPECT_EQ(cpu.get_register(3), 0x0FFFFFFFu);  // SRL: zero-fills
      cpu.execute(0x4020D1B3);
      EXPECT_EQ(cpu.get_register(3), 0xFFFFFFFFu); }  // SRA: sign-extends

    // OR, AND
    { CPU cpu;
      cpu.execute(0x00F00093); cpu.execute(0x0F000113); cpu.execute(0x0020E1B3);
      EXPECT_EQ(cpu.get_register(3), 0xFFu); }

    { CPU cpu;
      cpu.execute(0xFFF00093); cpu.execute(0x0F000113); cpu.execute(0x0020F1B3);
      EXPECT_EQ(cpu.get_register(3), 0xF0u); }
}

// ---- Loads ----

TEST(Loads, AllWidths) {
    // LW: little-endian word reassembly
    { CPU cpu;
      cpu.write_byte(0x100, 0xEF); cpu.write_byte(0x101, 0xBE);
      cpu.write_byte(0x102, 0xAD); cpu.write_byte(0x103, 0xDE);
      cpu.execute(0x10000093); cpu.execute(0x0000A103);
      EXPECT_EQ(cpu.get_register(2), 0xDEADBEEFu); }

    // LB sign-extends; LBU zero-extends — same source byte, different results
    { CPU cpu;
      cpu.write_byte(0x100, 0xFF);
      cpu.execute(0x10000093); cpu.execute(0x00008103);
      EXPECT_EQ(cpu.get_register(2), 0xFFFFFFFFu); }

    { CPU cpu;
      cpu.write_byte(0x100, 0xFF);
      cpu.execute(0x10000093); cpu.execute(0x0000C103);
      EXPECT_EQ(cpu.get_register(2), 0xFFu); }

    // LH sign-extends; LHU zero-extends — same source halfword, different results
    { CPU cpu;
      cpu.write_byte(0x100, 0x00); cpu.write_byte(0x101, 0x80);
      cpu.execute(0x10000093); cpu.execute(0x00009103);
      EXPECT_EQ(cpu.get_register(2), 0xFFFF8000u); }

    { CPU cpu;
      cpu.write_byte(0x100, 0x00); cpu.write_byte(0x101, 0x80);
      cpu.execute(0x10000093); cpu.execute(0x0000D103);
      EXPECT_EQ(cpu.get_register(2), 0x00008000u); }

    // Negative offset: LBU -4(x1) reads from 0x100 when x1 = 0x104
    { CPU cpu;
      cpu.write_byte(0x100, 0x42);
      cpu.execute(0x10400093); cpu.execute(0xFFC0C103);
      EXPECT_EQ(cpu.get_register(2), 0x42u); }
}

// ---- Stores ----

TEST(Stores, AllWidths) {
    // SB: stores low byte only
    { CPU cpu;
      cpu.execute(0x10000093); cpu.execute(0x0FF00113); cpu.execute(0x00208023);
      EXPECT_EQ(cpu.read_byte(0x100), 0xFFu); }

    // SB: upper bits of source are ignored; adjacent byte untouched
    { CPU cpu;
      cpu.execute(0x10000093); cpu.execute(0xFFF00113); cpu.execute(0x00208023);
      EXPECT_EQ(cpu.read_byte(0x100), 0xFFu);
      EXPECT_EQ(cpu.read_byte(0x101), 0x00u); }

    // SH: little-endian halfword
    { CPU cpu;
      cpu.execute(0x10000093); cpu.execute(0xFFF00113); cpu.execute(0x00209023);
      EXPECT_EQ(cpu.read_byte(0x100), 0xFFu);
      EXPECT_EQ(cpu.read_byte(0x101), 0xFFu);
      EXPECT_EQ(cpu.read_byte(0x102), 0x00u); }

    // SW: all four bytes written
    { CPU cpu;
      cpu.execute(0x10000093); cpu.execute(0xFFF00113); cpu.execute(0x0020A023);
      EXPECT_EQ(cpu.read_byte(0x100), 0xFFu);
      EXPECT_EQ(cpu.read_byte(0x101), 0xFFu);
      EXPECT_EQ(cpu.read_byte(0x102), 0xFFu);
      EXPECT_EQ(cpu.read_byte(0x103), 0xFFu); }

    // SW + LW round-trip: byte order must survive
    { CPU cpu;
      cpu.write_byte(0x200, 0x78); cpu.write_byte(0x201, 0x56);
      cpu.write_byte(0x202, 0x34); cpu.write_byte(0x203, 0x12);
      cpu.execute(0x10000093); cpu.execute(0x20000113);
      cpu.execute(0x00012183); cpu.execute(0x0030A023);
      EXPECT_EQ(cpu.read_byte(0x100), 0x78u);
      EXPECT_EQ(cpu.read_byte(0x101), 0x56u);
      EXPECT_EQ(cpu.read_byte(0x102), 0x34u);
      EXPECT_EQ(cpu.read_byte(0x103), 0x12u); }

    // SB with negative offset
    { CPU cpu;
      cpu.execute(0x10400093); cpu.execute(0x07B00113); cpu.execute(0xFE208E23);
      EXPECT_EQ(cpu.read_byte(0x100), 123u); }
}

// ---- Branches ----

TEST(Branches, AllConditions) {
    // BEQ: taken when equal, not taken when unequal
    { CPU cpu;
      cpu.execute(0x00500093); cpu.execute(0x00500113);
      cpu.execute(0x00208463);  // equal → taken: pc = 8 + 8 = 16
      EXPECT_EQ(cpu.get_pc(), 16u); }

    { CPU cpu;
      cpu.execute(0x00500093); cpu.execute(0x00300113);
      cpu.execute(0x00208463);  // unequal → fall through: pc = 12
      EXPECT_EQ(cpu.get_pc(), 12u); }

    // BNE: taken when unequal
    { CPU cpu;
      cpu.execute(0x00500093); cpu.execute(0x00300113); cpu.execute(0x00209463);
      EXPECT_EQ(cpu.get_pc(), 16u); }

    // BLT (signed): -1 < 5 → taken
    { CPU cpu;
      cpu.execute(0xFFF00093); cpu.execute(0x00500113); cpu.execute(0x0020C463);
      EXPECT_EQ(cpu.get_pc(), 16u); }

    // BLTU (unsigned): 0xFFFFFFFF > 5 → not taken
    { CPU cpu;
      cpu.execute(0xFFF00093); cpu.execute(0x00500113); cpu.execute(0x0020E463);
      EXPECT_EQ(cpu.get_pc(), 12u); }

    // BGE (signed): 5 >= -1 → taken
    { CPU cpu;
      cpu.execute(0x00500093); cpu.execute(0xFFF00113); cpu.execute(0x0020D463);
      EXPECT_EQ(cpu.get_pc(), 16u); }

    // BGEU (unsigned): 0 >= 1 → not taken
    { CPU cpu;
      cpu.execute(0x00100113); cpu.execute(0x0020F063);
      EXPECT_EQ(cpu.get_pc(), 8u); }

    // Backward branch: pc = 8 + (-8) = 0
    { CPU cpu;
      cpu.execute(0x00500093); cpu.execute(0x00500113);
      cpu.execute(0xFE208CE3);
      EXPECT_EQ(cpu.get_pc(), 0u); }
}

// ---- Jumps ----

TEST(Jumps, JALAndJALR) {
    // JAL: jump forward and save return address
    { CPU cpu;
      cpu.execute(0x100000EF);
      EXPECT_EQ(cpu.get_register(1), 4u);
      EXPECT_EQ(cpu.get_pc(), 0x100u); }

    // JAL: backward jump
    { CPU cpu;
      cpu.execute(0x00500093); cpu.execute(0xFFDFF0EF);
      EXPECT_EQ(cpu.get_pc(), 0u); }

    // JAL x0: jump happens, return address silently discarded
    { CPU cpu;
      cpu.execute(0x1000006F);
      EXPECT_EQ(cpu.get_register(0), 0u);
      EXPECT_EQ(cpu.get_pc(), 0x100u); }

    // JALR: jump to register + offset
    { CPU cpu;
      cpu.execute(0x20000113); cpu.execute(0x000100E7);
      EXPECT_EQ(cpu.get_register(1), 8u);
      EXPECT_EQ(cpu.get_pc(), 0x200u); }

    // JALR: clears LSB of computed target per RISC-V spec
    { CPU cpu;
      cpu.execute(0x00300113); cpu.execute(0x000100E7);
      EXPECT_EQ(cpu.get_pc(), 2u); }

    // JALR: function call and return with rd == rs1
    { CPU cpu;
      cpu.execute(0x100000EF);
      EXPECT_EQ(cpu.get_pc(), 0x100u);
      EXPECT_EQ(cpu.get_register(1), 4u);
      cpu.execute(0x000080E7);
      EXPECT_EQ(cpu.get_pc(), 4u); }
}

// ---- Upper-immediate ----

TEST(UpperImmediate, LUIAndAUIPC) {
    // LUI: places 20-bit immediate in upper bits, lower 12 bits zeroed
    { CPU cpu;
      cpu.execute(0x123450B7);
      EXPECT_EQ(cpu.get_register(1), 0x12345000u); }

    { CPU cpu;
      cpu.execute(0xFFFFF0B7);
      EXPECT_EQ(cpu.get_register(1), 0xFFFFF000u); }

    // LUI + ADDI: standard 32-bit constant loading pattern
    { CPU cpu;
      cpu.execute(0x123450B7); cpu.execute(0x67808093);
      EXPECT_EQ(cpu.get_register(1), 0x12345678u); }

    // AUIPC: adds upper immediate to pc for position-independent addressing
    { CPU cpu;
      cpu.execute(0x00500093);
      cpu.execute(0x12345117);  // AUIPC x2, 0x12345 at pc=4
      EXPECT_EQ(cpu.get_register(2), 0x12345004u); }
}

// ---- System ----

TEST(System, ECALLAndEBREAK) {
    // ECALL: sets halted flag, pc still advances
    { CPU cpu;
      EXPECT_FALSE(cpu.is_halted());
      cpu.execute(0x00000073);
      EXPECT_TRUE(cpu.is_halted());
      EXPECT_EQ(cpu.get_pc(), 4u); }

    // EBREAK: also sets halted flag
    { CPU cpu;
      cpu.execute(0x00100073);
      EXPECT_TRUE(cpu.is_halted()); }
}
