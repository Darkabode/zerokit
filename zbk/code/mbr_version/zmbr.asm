org 7C00h

xor ax, ax
jmp l_start
l_dap:
; Здесь у нас будет пакет дискового адреса.
dap_size     db 10h
dap_reserved db 0
dap_sectors  dw 0
dap_buff_lo  dw 0
dap_buff_hi  dw 3000h
dap_start_lo dd 0
dap_start_hi dd 0
l_start:
cli
mov ds, ax
mov ss, ax
mov sp, 7C00h
pushad                                                           ; Сохраняем регистры для дальнейшей передачи загрузчику ОС.
sti

; Проверяем установлен ли наш перехват 13-го прерывания, если да, то грузимся из активного раздела.
mov bx, [4Ch]
mov es, [4Eh]
cmp dword [es:bx + 2], 74B0D47Fh                                 ; Проверяем сигнатуру вначале нашего INT 13h обработчика.
jz l_boot_active
call f_read_sectors                                              ; Загружаем наш буткит в память по адресу 3000h:0000h.
jc l_boot_active                                                 ; Если возникла какая-то ошибка, то грузимся с активного раздела.
push es
push word 200h                                                   ; Первым идёт оригинальный бутсектор.
retf                                                             ; Передаём управление буткиту.

l_boot_active:
popad                                                            ; Восстанавливаем регистры.
; Перемещаем себя по адресу 777h:7C00h.
mov bp, 7c00h
mov ax, 777h
mov es, ax
mov si, bp
mov di, bp
mov cx, 128
cld
rep movsd                                                        ; Копируем 512 байт.
push es
push l_new_location
retf

l_new_location:
mov ds, bp                                                       ; Устанавливаем адрес нашего нового сегмента (3000h).
; Ищем активный раздел
sub di, 42h
l_next_entry:
test byte [di], 80h                                              ; Проверяем является ли раздел активным.
jnz l_active_found
add di, 0x10                                                     ; Переходим к следующей таблице разделов.
cmp di, 7C00h + 1FEh                                             ; Проверяем, не вышли ли мы за пределы таблицы разделов.
jb l_next_entry
call f_print_text
db 'Invalid partition table', 0
l_active_found:
mov eax, [di + 8]                                                ; В eax сохраняем номер начального сектора раздела.
; setup LBA block
mov [dap_start_lo], eax
xor ebx, ebx
mov [dap_start_hi], ebx
mov [dap_buff_hi], bx
mov word [dap_buff_lo], 7C00h
inc bx
mov [dap_sectors], bx
; reat boot sector
call f_read_sectors
jnc l_do_boot_1
call f_print_text
db 'Disk read error', 0
l_do_boot_1:
; check boot signature
cmp word [es:7C00h+1FEh], 0AA55h
jz l_do_boot_2
call f_print_text
db 'Invalid boot sector', 0
l_do_boot_2:
; jump to boot sector
push  es
push  bp
retf


f_read_sectors:
pusha
; save drive number
mov bp, dx
; setup read segment
push word [dap_buff_hi]
pop es
; if read area below that 504mb use CHS enforcement
; this needed for compatibility with some stupid BIOSes
xor eax, eax
cmp dword [dap_start_hi], eax
jnz l_check_lba
cmp dword [dap_start_lo], 504 * 1024 * 2
jb l_chs_mode
l_check_lba:
; check for LBA support
mov ah, 41h
mov bx, 55AAh
int 13h
jc l_chs_mode
cmp bx, 0AA55h
jnz l_chs_mode
test cl, 1
jz l_chs_mode
 ; setup LBA parameters
l_lba_mode:
mov si, l_dap
mov ah, 42h
mov dx, bp
jmp l_read
l_chs_mode: 
; get drive geometry
mov ah, 08h
mov dx, bp
push es
int 13h
pop es
; if get geometry failed, then try to use LBA mode
jc l_lba_mode
; translate LBA to CHS
and cl, 3Fh
inc dh
movzx ecx, cl ; ecx - max_sect
movzx esi, dh ; esi - max_head
mov eax, [dap_start_lo]
xor edx, edx
div ecx
inc dx
mov cl, dl
xor dx, dx
div esi
mov dh, dl
mov ch, al
shr ax, 002h
and al, 0C0h
or cl, al
mov ax, [dap_sectors]
mov ah, 2
; set up drive number
mov bx, bp
mov dl, bl
mov bx, [dap_buff_lo]
l_read:
push es
int 13h
pop es
popa
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
