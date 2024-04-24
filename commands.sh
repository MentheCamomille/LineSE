# ./setup-gcc-debian.sh
export PATH=$PATH:/usr/local/i386elfgcc/bin

echo Boot:
nasm "bootloader/boot.asm" -f bin -o "bins/boot.bin" -i bootloader
nasm "bootloader/kernel_entry.asm" -f elf -o "bins/kernel_entry.o"
i386-elf-gcc -ffreestanding -m32 -g -c "bootloader/kernel.cpp" -o "bins/kernel.o"
nasm "bootloader/zeroes.asm" -f bin -o "bins/zeroes.bin"

i386-elf-ld -o "bins/full_kernel.bin" -Ttext 0x1000 "bins/kernel_entry.o" "bins/kernel.o" --oformat binary
cat "bins/boot.bin" "bins/full_kernel.bin" "bins/zeroes.bin" > "bins/OS.bin"

qemu-system-x86_64 -drive format=raw,file="bins/OS.bin",index=0,if=floppy, -m 128M

