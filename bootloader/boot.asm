[org 0x7c00]
mov [BOOT_DISK], dl

; setting up the stack

xor ax, ax      ; make sure ds is set to 0
mov es, ax
mov ds, ax
mov bp, 0x8000
mov sp, bp

mov bx, 0x7e00

; reading the disk

mov ah, 2 
mov al, 1 		; number of sectors to read
mov ch, 0       ; number of cylinder
mov dh, 0       ; head number
mov cl, 2       ; sector number
mov dl, [BOOT_DISK]     ; drive number to be stored in variable
int 0x13

; printing what is in the next sector

mov ah, 0x0e
mov al, [0x7e00]
int 0x10
jmp $
BOOT_DISK: db 0

; magic padding

times 510-($-$$) db 0
dw 0xaa55

; filling the second sector ( the one that will be readed ) with 'A's

times 512 db 'A'