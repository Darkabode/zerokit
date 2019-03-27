.code

keGetCurrentIrql PROC
     mov rax, cr8
     ret
keGetCurrentIrql ENDP

kfRaiseIrql PROC
     mov rax, cr8
     movzx rcx, cl
     mov cr8, rcx
     ret
kfRaiseIrql ENDP

kfLowerIrql PROC
     movzx rcx, cl
     mov cr8, rcx
     ret
kfLowerIrql ENDP

; rcx - name
; rdx - sz
common_calc_hash PROC
     push rdi
     push rsi
     xor edi, edi
     mov rsi, rcx
     mov rcx, rdx
     cmp rcx, 0
     jz zero_based_calc
nextChar:
     xor eax, eax
     lodsb
     or al, 20h
     ror edi, 11
     add edi, eax
     loop nextChar
     jmp complete_calc
zero_based_calc:
     xor eax, eax
     lodsb
     cmp al, 0
     je complete_calc
     ror edi, 11
     add edi, eax
     jmp zero_based_calc
complete_calc:
     mov eax, edi
     pop rsi
     pop rdi
     ret
common_calc_hash ENDP

common_disable_wp PROC
     push rbx
     mov rbx, cr0
     and ebx, 0fffeffffh
     mov cr0, rbx
     pop rbx
     ret
common_disable_wp ENDP

common_enable_wp PROC
     push rbx
     mov rbx, cr0
     or ebx, 00010000h
     mov cr0, rbx
     pop rbx
     ret
common_enable_wp ENDP

getCurrentProcessor PROC
    xor eax, eax
     mov al, gs:[184h]
     ret
getCurrentProcessor ENDP

hardclock PROC
    rdtsc
    ret
hardclock ENDP

dissasm_trigger_trampoline PROC
pHookFunc: ; puchar_t pHookFunc
    db 0
    db 0
    db 0
    db 0
    db 0
    db 0
    db 0
    db 0
pThis: ; pvoid_t pThis
    db 0
    db 0
    db 0
    db 0
    db 0
    db 0
    db 0
    db 0
isExecuted: ; uint32_t isExecuted
    db 0
    db 0
    db 0
    db 0

    lock inc dword ptr [isExecuted] ; interlocked increment execution counter

    ; Сохраняем регистры, которые используются в качестве fastcall gпараметров.
    push rcx
    push rdx
    push r8
    push r9

    ; Место для SSE-регистров. +
    ; Теневое место для параметров, передаваемых через регистры.
    sub rsp, 8 + 4 * 16 

    movups [rsp + 8 + 3 * 16], xmm0
    movups [rsp + 8 + 2 * 16], xmm1
    movups [rsp + 8 + 1 * 16], xmm2
    movups [rsp + 8 + 0 * 16], xmm3

    mov rcx, qword ptr [pThis]
    call qword ptr [pHookFunc]

    movups xmm3, [rsp + 8 + 0 * 16]
    movups xmm2, [rsp + 8 + 1 * 16]
    movups xmm1, [rsp + 8 + 2 * 16]
    movups xmm0, [rsp + 8 + 3 * 16]

    add rsp, 8 + 4 * 16

    pop r9
    pop r8
    pop rdx
    pop rcx

    lock dec dword ptr [isExecuted] ; interlocked decrement execution counter

dissasm_trigger_trampoline ENDP

getHDETable PROC
	jmp dataHDELabel
codeHDELabel:
	pop rax
	ret
dataHDELabel:
	nop
	call codeHDELabel
	db 0a5h
	db 0aah
	db 0a5h
	db 0b8h
	db 0a5h
	db 0aah
	db 0a5h
	db 0aah
	db 0a5h
	db 0b8h
	db 0a5h
	db 0b8h
	db 0a5h
	db 0b8h
	db 0a5h
	db 0b8h
	db 0c0h
	db 0c0h
	db 0c0h
	db 0c0h
	db 0c0h
	db 0c0h
	db 0c0h
	db 0c0h
	db 0ach
	db 0c0h
	db 0cch
	db 0c0h
	db 0a1h
	db 0a1h
	db 0a1h
	db 0a1h
	db 0b1h
	db 0a5h
	db 0a5h
	db 0a6h
	db 0c0h
	db 0c0h
	db 0d7h
	db 0dah
	db 0e0h
	db 0c0h
	db 0e4h
	db 0c0h
	db 0eah
	db 0eah
	db 0e0h
	db 0e0h
	db 098h
	db 0c8h
	db 0eeh
	db 0f1h
	db 0a5h
	db 0d3h
	db 0a5h
	db 0a5h
	db 0a1h
	db 0eah
	db 09eh
	db 0c0h
	db 0c0h
	db 0c2h
	db 0c0h
	db 0e6h
	db 003h
	db 07fh
	db 011h
	db 07fh
	db 001h
	db 07fh
	db 001h
	db 03fh
	db 001h
	db 001h
	db 0abh
	db 08bh
	db 090h
	db 064h
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 092h
	db 05bh
	db 05bh
	db 076h
	db 090h
	db 092h
	db 092h
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 06ah
	db 073h
	db 090h
	db 05bh
	db 052h
	db 052h
	db 052h
	db 052h
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 077h
	db 07ch
	db 077h
	db 085h
	db 05bh
	db 05bh
	db 070h
	db 05bh
	db 07ah
	db 0afh
	db 076h
	db 076h
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 05bh
	db 086h
	db 001h
	db 003h
	db 001h
	db 004h
	db 003h
	db 0d5h
	db 003h
	db 0d5h
	db 003h
	db 0cch
	db 001h
	db 0bch
	db 003h
	db 0f0h
	db 003h
	db 003h
	db 004h
	db 000h
	db 050h
	db 050h
	db 050h
	db 050h
	db 0ffh
	db 020h
	db 020h
	db 020h
	db 020h
	db 001h
	db 001h
	db 001h
	db 001h
	db 0c4h
	db 002h
	db 010h
	db 0ffh
	db 0ffh
	db 0ffh
	db 001h
	db 000h
	db 003h
	db 011h
	db 0ffh
	db 003h
	db 0c4h
	db 0c6h
	db 0c8h
	db 002h
	db 010h
	db 000h
	db 0ffh
	db 0cch
	db 001h
	db 001h
	db 001h
	db 000h
	db 000h
	db 000h
	db 000h
	db 001h
	db 001h
	db 003h
	db 001h
	db 0ffh
	db 0ffh
	db 0c0h
	db 0c2h
	db 010h
	db 011h
	db 002h
	db 003h
	db 001h
	db 001h
	db 001h
	db 0ffh
	db 0ffh
	db 0ffh
	db 000h
	db 000h
	db 000h
	db 0ffh
	db 000h
	db 000h
	db 0ffh
	db 0ffh
	db 0ffh
	db 0ffh
	db 010h
	db 010h
	db 010h
	db 010h
	db 002h
	db 010h
	db 000h
	db 000h
	db 0c6h
	db 0c8h
	db 002h
	db 002h
	db 002h
	db 002h
	db 006h
	db 000h
	db 004h
	db 000h
	db 002h
	db 0ffh
	db 000h
	db 0c0h
	db 0c2h
	db 001h
	db 001h
	db 003h
	db 003h
	db 003h
	db 0cah
	db 040h
	db 000h
	db 00ah
	db 000h
	db 004h
	db 000h
	db 000h
	db 000h
	db 000h
	db 07fh
	db 000h
	db 033h
	db 001h
	db 000h
	db 000h
	db 000h
	db 000h
	db 000h
	db 000h
	db 0ffh
	db 0bfh
	db 0ffh
	db 0ffh
	db 000h
	db 000h
	db 000h
	db 000h
	db 007h
	db 000h
	db 000h
	db 0ffh
	db 000h
	db 000h
	db 000h
	db 000h
	db 000h
	db 000h
	db 000h
	db 000h
	db 000h
	db 000h
	db 000h
	db 000h
	db 000h
	db 000h
	db 0ffh
	db 0ffh
	db 000h
	db 000h
	db 000h
	db 0bfh
	db 000h
	db 000h
	db 000h
	db 000h
	db 000h
	db 000h
	db 000h
	db 000h
	db 07fh
	db 000h
	db 000h
	db 0ffh
	db 040h
	db 040h
	db 040h
	db 040h
	db 041h
	db 049h
	db 040h
	db 040h
	db 040h
	db 040h
	db 04ch
	db 042h
	db 040h
	db 040h
	db 040h
	db 040h
	db 040h
	db 040h
	db 040h
	db 040h
	db 04fh
	db 044h
	db 053h
	db 040h
	db 040h
	db 040h
	db 044h
	db 057h
	db 043h
	db 05ch
	db 040h
	db 060h
	db 040h
	db 040h
	db 040h
	db 040h
	db 040h
	db 040h
	db 040h
	db 040h
	db 040h
	db 040h
	db 040h
	db 040h
	db 040h
	db 040h
	db 064h
	db 066h
	db 06eh
	db 06bh
	db 040h
	db 040h
	db 06ah
	db 046h
	db 040h
	db 040h
	db 044h
	db 046h
	db 040h
	db 040h
	db 05bh
	db 044h
	db 040h
	db 040h
	db 000h
	db 000h
	db 000h
	db 000h
	db 006h
	db 006h
	db 006h
	db 006h
	db 001h
	db 006h
	db 006h
	db 002h
	db 006h
	db 006h
	db 000h
	db 006h
	db 000h
	db 00ah
	db 00ah
	db 000h
	db 000h
	db 000h
	db 002h
	db 007h
	db 007h
	db 006h
	db 002h
	db 00dh
	db 006h
	db 006h
	db 006h
	db 00eh
	db 005h
	db 005h
	db 002h
	db 002h
	db 000h
	db 000h
	db 004h
	db 004h
	db 004h
	db 004h
	db 005h
	db 006h
	db 006h
	db 006h
	db 000h
	db 000h
	db 000h
	db 00eh
	db 000h
	db 000h
	db 008h
	db 000h
	db 010h
	db 000h
	db 018h
	db 000h
	db 020h
	db 000h
	db 028h
	db 000h
	db 030h
	db 000h
	db 080h
	db 001h
	db 082h
	db 001h
	db 086h
	db 000h
	db 0f6h
	db 0cfh
	db 0feh
	db 03fh
	db 0abh
	db 000h
	db 0b0h
	db 000h
	db 0b1h
	db 000h
	db 0b3h
	db 000h
	db 0bah
	db 0f8h
	db 0bbh
	db 000h
	db 0c0h
	db 000h
	db 0c1h
	db 000h
	db 0c7h
	db 0bfh
	db 062h
	db 0ffh
	db 000h
	db 08dh
	db 0ffh
	db 000h
	db 0c4h
	db 0ffh
	db 000h
	db 0c5h
	db 0ffh
	db 000h
	db 0ffh
	db 0ffh
	db 0ebh
	db 001h
	db 0ffh
	db 00eh
	db 012h
	db 008h
	db 000h
	db 013h
	db 009h
	db 000h
	db 016h
	db 008h
	db 000h
	db 017h
	db 009h
	db 000h
	db 02bh
	db 009h
	db 000h
	db 0aeh
	db 0ffh
	db 007h
	db 0b2h
	db 0ffh
	db 000h
	db 0b4h
	db 0ffh
	db 000h
	db 0b5h
	db 0ffh
	db 000h
	db 0c3h
	db 001h
	db 000h
	db 0c7h
	db 0ffh
	db 0bfh
	db 0e7h
	db 008h
	db 000h
	db 0f0h
	db 002h
	db 000h
	ret
getHDETable ENDP

END
