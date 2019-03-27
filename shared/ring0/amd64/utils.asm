.code

; rcx - name
; rdx - sz
calc_hash PROC
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
calc_hash ENDP

END
