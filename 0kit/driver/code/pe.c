


// #if _AMD64_
// 
// extern void FindExportByHash(PVOID* exportAddr, PVOID basePtr, UINT32 hashVal);
// 
// #else
// 
// void FindExportByHash(PVOID* exportAddr, PVOID basePtr, UINT32 hashVal)
// {
// 	__asm {
// 		cld
// 		mov ebx, [ebp + 12]
// 		add ebx, [ebx + 3Ch]
// 		mov ebx, [ebx + 78h	]
// 		add ebx, [ebp + 12]
// 		mov ecx, [ebx + 18h]
// 		mov esi, [ebp + 12]
// 		add esi, [ebx + 20h]
// next_api_name:
// 		lodsd
// 		push esi
// 		mov esi, [ebp + 12]
// 		add esi, eax
// 
// 		xor edi, edi
// fa_loop_modname:
// 		xor eax, eax
// 		lodsb
// 		cmp al, 0
// 		je fa_finish_hash
// 		ror edi, 11
// 		add edi, eax
// 		jmp fa_loop_modname
// fa_finish_hash:
// 		pop esi
// 
// 		cmp edi, [ebp + 16]
// 		je found_name
// 
// 		loop next_api_name
// 		xor eax, eax
// 		mov edx, [ebp + 8]
// 		mov [edx], eax
// 		jmp finish
// found_name:
// 		mov edx, [ebp + 12]
// 		add edx, [ebx + 24h]
// 		mov eax, [ebx + 18h]
// 		sub eax, ecx
// 		shl eax, 1
// 		add edx, eax
// 		xor eax, eax
// 		mov ax, [edx]
// 		shl eax, 2
// 		mov edx, [ebp + 12]
// 		add edx, [ebx + 1Ch]
// 		add edx, eax
// 		mov eax, [edx]
// 		add eax, [ebp + 12]
// 		mov edx, [ebp + 8]
// 		mov [edx], eax
// finish:
// 	}
// }
// 
// #endif
