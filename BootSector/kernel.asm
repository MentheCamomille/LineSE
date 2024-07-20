[BITS 16]
[ORG 0x0]

jmp start

%include "UTIL.INC"

start:
; initialisation des segments en 0x100
    mov ax, 0x100
    mov ds, ax
    mov es, ax

; initialisation du segment de pile
    mov ax, 0x8000
    mov ss, ax
    mov sp, 0xf000

; affiche un msg
    mov si, msg00
    call afficher

end:
    jmp end


msg00: db 'Kernel is speaking !', 10, 0

