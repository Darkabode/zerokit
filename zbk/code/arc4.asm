push    ebp
mov     ebp, esp
sub     esp, 108h
push    ebx
push    esi
push    edi
xor     edi, edi
mov     [ebp-8], edi
xor     eax, eax

loc_401080:
mov     [ebp+eax-108h], al
inc     eax
cmp     eax, 100h
jb      short loc_401080
mov     [ebp-4], edi
mov     ecx, 0FFh

loc_401097:
mov     eax, [ebp-8]
cmp     eax, [ebp+0x14]
jb      short loc_4010A3
and     [ebp-8], 0

loc_4010A3:
mov     eax, [ebp-4]
mov     ebx, [ebp-8]
mov     esi, [ebp+0x10]
movzx   esi, byte ptr [ebx+esi]
lea     eax, [ebp+eax-108h]
movzx   edx, byte ptr [eax]
add     esi, edx
add     esi, edi
and     esi, ecx
inc     [ebp-4]
inc     [ebp-8]
cmp     [ebp-4], 100h
mov     edi, esi
lea     esi, [ebp+edi-108h]
mov     bl, [esi]
mov     [eax], bl
mov     [esi], dl
jb      short loc_401097
xor     eax, eax
xor     ebx, ebx
mov     [ebp-4], eax
cmp     [ebp+0x0C], eax
jbe     short loc_401130
jmp     short loc_4010EF

loc_4010EC:
mov     eax, [ebp+0x14]

loc_4010EF:
inc     ebx
and     ebx, ecx
lea     esi, [ebp+ebx-108h]
movzx   edx, byte ptr [esi]
add     eax, edx
and     eax, ecx
lea     edi, [ebp+eax-108h]
mov     [ebp+0x14], eax
movzx   eax, byte ptr [edi]
mov     [esi], al
mov     esi, [ebp+8]
add     al, dl
mov     [edi], dl
mov     edi, [ebp-4]
movzx   eax, al
mov     al, [ebp+eax-108h]
add     esi, edi
xor     [esi], al
inc     edi
mov     [ebp-4], edi
cmp     edi, [ebp+0x0C]
jb      short loc_4010EC

loc_401130:
pop     edi
pop     esi
pop     ebx
leave
retn    10h
