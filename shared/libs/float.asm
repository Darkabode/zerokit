format MS COFF

section '.data' data readable
public __Inf
public __Nan

 __Inf db 0,0,0,0,0,0,0xF0,0x7F,0,0,0,0,0,0,0,0
 __Nan db 0,0,0,0,0,0,0xF8,0x7F,0,0,0,0,0,0,0,0
