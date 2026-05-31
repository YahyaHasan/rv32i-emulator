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