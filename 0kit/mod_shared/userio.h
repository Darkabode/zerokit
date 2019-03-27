#ifndef __Z_USERIO_H_
#define __Z_USERIO_H_

#define USERIO_ZDEV 65500U
#define IOCTL_USERIO_ZFS_OPEN              CTL_CODE(USERIO_ZDEV, 0x901, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_READ              CTL_CODE(USERIO_ZDEV, 0x902, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_WRITE             CTL_CODE(USERIO_ZDEV, 0x903, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_CLOSE             CTL_CODE(USERIO_ZDEV, 0x904, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_UNLINK            CTL_CODE(USERIO_ZDEV, 0x906, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_MKDIR             CTL_CODE(USERIO_ZDEV, 0x907, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_TELL              CTL_CODE(USERIO_ZDEV, 0x908, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_ISEOF             CTL_CODE(USERIO_ZDEV, 0x909, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_RMDIR             CTL_CODE(USERIO_ZDEV, 0x90A, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_MOVE              CTL_CODE(USERIO_ZDEV, 0x90B, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_CHECKVALID        CTL_CODE(USERIO_ZDEV, 0x90C, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_GETC              CTL_CODE(USERIO_ZDEV, 0x90D, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_PUTC              CTL_CODE(USERIO_ZDEV, 0x90E, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_GETLINE           CTL_CODE(USERIO_ZDEV, 0x90F, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_SEEK              CTL_CODE(USERIO_ZDEV, 0x910, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_FINDFIRST         CTL_CODE(USERIO_ZDEV, 0x911, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_FINDNEXT          CTL_CODE(USERIO_ZDEV, 0x912, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_GET_TIME          CTL_CODE(USERIO_ZDEV, 0x913, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_GET_VERSION       CTL_CODE(USERIO_ZDEV, 0x914, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_GET_FILESIZE      CTL_CODE(USERIO_ZDEV, 0x915, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_ZFS_SET_END_OF_FILE   CTL_CODE(USERIO_ZDEV, 0x916, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_KEYB_HOOK             CTL_CODE(USERIO_ZDEV, 0x940, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_USERIO_KEYB_BLOCK            CTL_CODE(USERIO_ZDEV, 0x941, METHOD_NEITHER, FILE_ANY_ACCESS)

enum {
    in_type,
    out_type,
    in_out_type
};

enum
{
    userio_zfs_open_id,
    userio_zfs_close_id,
    userio_zfs_read_id,
    userio_zfs_write_id,
    userio_zfs_seek_id,
    userio_zfs_unlink_id,
    userio_zfs_mkdir_id,
    userio_zfs_tell_id,
    userio_zfs_iseof_id,
    userio_zfs_rmdir_id,
    userio_zfs_move_id,
    userio_zfs_checkvalid_id,
    userio_zfs_getc_id,
    userio_zfs_putc_id,
    userio_zfs_getline_id,
    userio_zfs_findfirst_id,
    userio_zfs_findnext_id,
    userio_zfs_get_time_id,
    userio_zfs_get_version_id,
    userio_zfs_get_filesize_id,
    userio_zfs_setendoffile_id,
    userio_keyb_hook_id,
    userio_keyb_block_id,
};

#define REQUEST_SIGNATURE 0x34798977

#ifndef CLIENT_ID_SIZE
#define CLIENT_ID_SIZE 16
#endif // CLIENT_ID_SIZE

#pragma pack(push, 1)

typedef struct _file_packet
{
    uint32_t signature; // Сигнатура запроса 0x34798977.
    uint32_t operation; // ID операции.
    uint32_t handle;    // Описатель файла.
    int errCode;        // Код ошибки.
    uint32_t param1;    // Свободный параметр для любых данных.
    uint8_t clientId[CLIENT_ID_SIZE];   // Уникальный идентификатор клиента.
    uint32_t dataSize;  // Размер данных.
    uint8_t data[1];    // Данные.
} file_packet_t, *pfile_packet_t;

#define MAX_BUFFER_SIZE 0x1000;

typedef struct _file_data
{
    char fileName[MAX_PATH];    // Имя файла/директории.
    int filepoiner;             // для операции установки файлового указателя
    int bEof;                   // для операции проверки конца файла или при создании и открытии файла - флаги создания\открытия или при выборе типа времени создания файла (возвращается время создания файла при запросе)
    char origin;                // для операции установки файлового указателя - относительное положение
    uint32_t fileSize;          // размер файла (обычно возвращается после создания\открытия)
    uint32_t dataSize;          // размер буфера для операций чтения \ записи
    uint8_t data[1];            // буфер чтения \ записи  (возможно он будет ограничен MAX_BUFFER_SIZE во избежании излишней траты ядерной памяти (ввод вывод буферизирован))
} file_data_t, *pfile_data_t;

#pragma pack()

#endif // __Z_USERIO_H_
