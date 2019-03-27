.code

GetKVBOffset PROC ; KdVersionBlock
    mov rax, gs:[18h]
    mov rax, [rax + 108h]
    ret
GetKVBOffset ENDP

GetPML4Base PROC
    mov rax, cr3
    ret
GetPML4Base ENDP

UtilsAtomicAdd16 PROC ; (UINT16* atomic, UINT16 value)
    ;mov ecx, atomic
    ;mov dx, value
    lock add word ptr [rcx], dx
    ret
UtilsAtomicAdd16 ENDP

UtilsAtomicSub16 PROC ; (UINT32* atomic, UINT16 value)
    ;mov ecx, atomic
    ;mov dx, value
    lock sub word ptr [rcx], dx
UtilsAtomicSub16 ENDP

GetIDTR PROC
    sidt [rcx]
    ret
GetIDTR ENDP

END
              