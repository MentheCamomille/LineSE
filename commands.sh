export PATH=$PATH:/usr/local/i386elfgcc/bin
echo Boot:
nasm "bootloader/boot.asm" -f bin -o "bins/boot.bin" -i bootloader

qemu-system-x86_64 "bins/boot.bin"