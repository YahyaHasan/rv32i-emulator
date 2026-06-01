CC     = riscv64-unknown-elf-gcc
CFLAGS = -march=rv32i -mabi=ilp32 -nostdlib -static -ffreestanding -O0

all: hello

hello: hello.c
	$(CC) $(CFLAGS) -o hello hello.c

disasm: hello
	riscv64-unknown-elf-objdump -d hello

headers: hello
	riscv64-unknown-elf-readelf -h hello
	riscv64-unknown-elf-readelf -l hello

clean:
	rm -f hello

.PHONY: all disasm headers clean
