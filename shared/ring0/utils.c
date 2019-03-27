#ifndef _WIN64

UINT32 calc_hash(PUCHAR name, SIZE_T sz)
{
	__asm {
		xor edx, edx
		mov esi, name
		mov ecx, sz
		cmp ecx, 0
		jz zero_based_calc
nextChar:
		xor eax, eax
		lodsb
		or al, 20h
		ror edx, 11
		add edx, eax
		loop nextChar		
		jmp complete_calc
zero_based_calc:
		xor eax, eax
		lodsb
		cmp al, 0
		je complete_calc
		ror edx, 11
		add edx, eax
		jmp zero_based_calc
complete_calc:
		mov eax, edx
	}
}

#else

extern UINT32 calc_hash(PUCHAR name, SIZE_T sz);

#endif // _WIN64