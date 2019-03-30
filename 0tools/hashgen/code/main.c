#include <stdio.h>

#include "../../../shared/platform.h"
#include "../../../shared/types.h"
#include "../../../shared/native.h"

uint32_t common_calc_hash(uint8_t* name, size_t sz)
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

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Usage: hashgen <string>\n\n");
        return 1;
    }

    printf("%s: 0x%08X\n", argv[1], common_calc_hash(argv[1], 0));
    return 0;
}