;org 7C00h

; Данный модуль содержит MBR/VBR код и по сути является Stage0-загузчиком в оббязанности которого кходит:
; 1. Загрузка и дешифрация Stage1-загрузчика.
; 2. Передача управления на Stage1-загрузчик.
; 3. В случае возникновения каких-либо ошибок, передача управления на настоящий загрузчик системы (MBR/VBR).

xor    ax, ax
jmp    l_start
db 0x77 ; Произвольный байт для выравнивания

;l_dap:
; Пакет дискового адреса для загрузки Stage1-модуля.
dap_st1_size     db 10h
dap_st1_reserved db 0
dap_st1_sectors  dw 0
dap_st1_buff_lo  dw 0
dap_st1_buff_hi  dw 3000h
dap_st1_start_lo dd 0
dap_st1_start_hi dd 0

; Загружаем VBR настоящего активного раздела и передаём ему управление.
l_new_location:
mov    ds, bp ; Устанавливаем адрес нашего нового сегмента (3000h).
; Ищем активный раздел
sub    di, 0x42
l_next_entry:
test   byte [di], 0x80 ; Проверяем является ли раздел активным.
jnz    l_active_found
add    di, 0x10 ; Переходим к следующей таблице разделов.
cmp    di, 7C00h + 1FEh ; Проверяем, не вышли ли мы за пределы таблицы разделов.
jb     l_next_entry
call   f_print_text
db     'Invalid partition table', 0
l_active_found:
mov    eax, [di + 8]                                                ; В eax сохраняем номер начального сектора раздела.
; setup LBA block
mov   [dap_st1_start_lo], eax
xor   ebx, ebx
mov   [dap_st1_start_hi], ebx
mov   [dap_st1_buff_hi], bx
mov   word [dap_st1_buff_lo], 7C00h
inc   bx
mov   [dap_st1_sectors], bx
; reat boot sector
call  f_read_sectors
jnc   l_do_boot_1
call  f_print_text
db    'Disk read error', 0
l_do_boot_1:
; check boot signature
cmp   word [es:7C00h+1FEh], 0AA55h
jz    l_do_boot_2
call  f_print_text
db    'Invalid boot sector', 0
l_do_boot_2:
; jump to boot sector
push  es
push  bp
retf

l_start:
cli
mov    ss, ax
mov    sp, 0x7C00
push   0x07C0
pop    ds
pushad ; Сохраняем регистры для дальнейшей передачи загрузчику ОС.
sti

; Проверяем установлен ли наш перехват 13-го прерывания, если да, то грузимся из активного раздела.
mov    bx, [ss:0x4C]
mov    es, [ss:0x4E]
cmp    dword [es:bx + 2], 0x74B0D47F ; Проверяем сигнатуру вначале нашего INT 13h обработчика.
jz     l_boot_active
mov    si, 6 
call   f_read_sectors ; Загружаем наш буткит в память по адресу 3000h:0000h.
jc     l_boot_active ; Если возникла какая-то ошибка, то грузимся с активного раздела.

; Декриптуем тело буткита.
l_decryptor:
mov    di, $ ; Смещение от начала сектора.
mov    dx, di
add    dx, 64
mov    cx, [si + 2] ; Узнаём количество считанных секторов.
shl    cx, 9 ; В cx содержится размер тела буткита.
sub    cx, 512
xor    si, si
add    si, 512
xor    ebx, ebx ; В ebx будет лежать crc32 для дешифрованного буфера за исключением последних 4 байтов.
l_decrypt:
mov    al, [es:si] ; Загружаем очередной байт тела буткита в al.
xor    al, [ds:di] ; XOR-им с байтом ключа.
mov    [es:si], al ; Записываем дешифрованный байт обратно.
cmp    cx, 4
jle    skip_hash ; Последние 4 байта содержат хеш-значение, которое не должно учитываться.
rol    ebx, 7
xor    bl, al
skip_hash:
inc    di ; Увеличиваем позицию внутри ключа.
inc    si ; Увеличиваем позицию внтури тела буткита.
cmp    di, dx
jne     l_next_byte
sub    di, 64 ; Переходим на начало ключа.
l_next_byte:
loop   l_decrypt

; Проверяем контрольную сумму буткита.
sub    si, 4
cmp    ebx, [es:si]
jne    l_boot_active

; Передача управления буткиту
push   es
push   word 0x200 ; Первым идёт наш VBR-сектор в котором есть нужные нам функции.
retf ; Передаём управление Stage1-загрузчику.

l_boot_active:
popad ; Восстанавливаем регистры.
; Перемещаем себя по адресу 777h:7C00h.
mov    bp, 0x7c00
mov    ax, 0x60
xor    di, di
mov    es, ax
mov    si, bp
mov    cx, 128
cld
rep    movsd ; Копируем сами себя.
push   es
push   l_new_location
retf ; Передаём управление на блок l_new_location скопированный на новое место.


; si - смещение до DAP
; Важно, чтобы ds сожержал начало сегмента, внутри которого si будет указывать именно на DAP.
f_read_sectors:
pushad
; Сохраняем номер диска.
mov    bp, dx
; setup read segment
push   word [si + 6] ; dap_st1_buff_hi
pop    es
; if read area below that 504mb use CHS enforcement. This needed for compatibility with some stupid BIOSes
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
;mov    si, l_dap
mov    ah, 0x42
mov    dx, bp
jmp    l_read
l_chs_mode: 
; get drive geometry
mov    ah, 0x08
mov    dx, bp
push   es
int    0x13
pop es
; if get geometry failed, then try to use LBA mode
jc l_lba_mode
; Преобразуем LBA в CHS.
and    cl, 0x3F
inc    dh
movzx  ecx, cl ; ecx - max_sect
movzx  esi, dh ; esi - max_head
mov    eax, [ds:si + 8] ; dap_st1_start_lo
xor    edx, edx
div    ecx
inc    dx
mov    cl, dl
xor    dx, dx
div    esi
mov    dh, dl
mov    ch, al
shr    ax, 0x02
and    al, 0xC0
or     cl, al
mov    ax, [si + 2] ; dap_st1_sectors
mov    ah, 2
mov    bx, bp ; Указываем номер диска.
mov    dl, bl
mov    bx, [si + 4] ; dap_st1_buff_lo
l_read:
push   es
int    0x13
pop    es
popad
ret

f_print_text:                                                ; Выводит текст на экран (только для отладки).
pop si                                                     ; Начальный адрес выводимого сообщения находится сразу за командой call.
mov bh, 00h                                                ; Номер страницы = 0.
mov ah, 0Eh                                                ; Вывести символ в режиме телетайпа.
cs lodsb                                                   ; Загружаем в al первый символ.
next_printed_char:
int 10h
cs lodsb                                                   ; Загружаем в al очередной символ.
or al, al                                                  ; Проверяем на конец строки
jnz next_printed_char                                      ; Продолжаем выводить.
jmp $
;push si                                                    ; Ложим в стек смещение адреса возврата (это будет адрес сразу за выводимой строкой).
;ret

; Partition Table (set up for test debugging environment)

times 0x1BE - ($ - $$) db 0


; Partition_Table
Partition_1:
    Partition_1_bootable        db      0
    Partition_1_Start_CHS       db      00h, 01h, 01h
    Partition_1_Type            db      04h                 ; FAT16
    Partition_1_End_CHS         db      0, 0, 0
    Partition_1_Start_LBA       dd      63
    Partition_1_Sectors         dd      16128               ; ~ 8 MB
Partition_2:
    Partition_2_bootable        db      80h                 ; active partition
    Partition_2_Start_CHS       db      0, 0, 0
    Partition_2_Type            db      07h                 ; NTFS
    Partition_2_End_CHS         db      0, 0, 0
    Partition_2_Start_LBA       dd      16128 + 63
    Partition_2_Sectors         dd      40952               ; ~ 20 MB
Partition_3:
    Partition_3_bootable        db      0
    Partition_3_Start_CHS       db      0, 0, 0
    Partition_3_Type            db      0
    Partition_3_End_CHS         db      0, 0, 0
    Partition_3_Start_LBA       dd      0
    Partition_3_Sectors         dd      0
Partition_4:
    Partition_4_bootable        db      0
    Partition_4_Start_CHS       db      0, 0, 0
    Partition_4_Type            db      0
    Partition_4_End_CHS         db      0, 0, 0
    Partition_4_Start_LBA       dd      0
    Partition_4_Sectors         dd      0

times 510 -($ - $$) db 0

dw 0AA55h

file "..\bin\module.bin"

nonalign_end_of_zbk:

align 512

unused dw ($ - nonalign_end_of_zbk)