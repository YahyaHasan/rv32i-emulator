include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

add_executable(riscv_tests test_cpu.cpp)
target_link_libraries(riscv_tests PRIVATE riscv_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(riscv_tests)
