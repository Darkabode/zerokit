#ifndef __ZFS_ERROR_H_
#define __ZFS_ERROR_H_

/**
 * Формат кодов ошибок:
 *    1Bit     7Bits      8Bits           16Bits
 *     .     ........   ........    ........  ........
 * [ErrFlag][ModuleID][FunctionID][--   ERROR CODE   --]
*/

#define ZFS_MODULE_SHIFT                        24
#define ZFS_FUNCTION_SHIFT                      16

#define ZFS_GETERROR(x)                         (((unsigned)x) & 0xFFFF)
#define ZFS_ERRFLAG                             0x80000000
#define ZFS_isERR(x)                            ((x) & ZFS_ERRFLAG)

// Идентификаторы подсистем.
#define ZFS_MODULE_IOMAN                        ((1 << ZFS_MODULE_SHIFT) | ZFS_ERRFLAG)
#define ZFS_MODULE_DIR                          ((2 << ZFS_MODULE_SHIFT) | ZFS_ERRFLAG)
#define ZFS_MODULE_FILE                         ((3 << ZFS_MODULE_SHIFT) | ZFS_ERRFLAG)
#define ZFS_MODULE                              ((4 << ZFS_MODULE_SHIFT) | ZFS_ERRFLAG)
#define ZFS_MODULE_FORMAT                       ((5 << ZFS_MODULE_SHIFT) | ZFS_ERRFLAG)

// Идентификаторы функций.

// Идентификаторы функций IO менеджера.
#define ZFS_UNMOUNTPARTITION                    ((1 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_IOMAN)
#define ZFS_FLUSHCACHE                          ((2 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_IOMAN)
#define ZFS_BLOCKREAD                           ((3 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_IOMAN)
#define ZFS_BLOCKWRITE                          ((4 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_IOMAN)
#define ZFS_USERDRIVER                          ((5 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_IOMAN)

// Идентификаторы функций для работы с каталогами.
#define ZFS_FINDNEXTINDIR                       ((1 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_DIR)
#define ZFS_FETCHENTRYWITHCONTEXT               ((2 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_DIR)
#define ZFS_PUSHENTRYWITHCONTEXT                ((3 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_DIR)
#define ZFS_GETDIRENTRY                         ((4 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_DIR)
#define ZFS_FINDFIRST                           ((5 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_DIR)
#define ZFS_FINDNEXT                            ((6 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_DIR)
#define ZFS_REWINDFIND                          ((7 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_DIR)
#define ZFS_FINDFREEDIRENT                      ((8 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_DIR)
#define ZFS_PUTENTRY                            ((9 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_DIR)
#define ZFS_CREATESHORTNAME                     ((10 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_DIR)
#define ZFS_CREATELFNS                          ((11 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_DIR)
#define ZFS_EXTENDDIRECTORY                     ((12 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_DIR)
#define ZFS_MKDIR                               ((13 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_DIR)

// Идентификаторы функций для работы с файлами.
#define ZFS_OPEN                                ((1 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_RMDIR                               ((2 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_MOVE                                ((3	<< ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_EXTENDFILE                          ((4 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_READ                                ((5 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_GETC                                ((6 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_GETLINE                             ((7 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_WRITE                               ((8 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_PUTC                                ((9 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_SEEK                                ((10 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_CHECKVALID                          ((11 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_CLOSE                               ((12 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_SETTIME                             ((13 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_BYTESLEFT                           ((14 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_SETENDOFFILE                        ((15 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_ISEOF                               ((16 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_TELL                                ((17 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_UNLINK                              ((18 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_GETTIME                             ((19 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_GETVERSION                          ((20 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)
#define ZFS_GETFILESIZE                         ((21 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE_FILE)

// Идентификаторы функций для работы с файловой системой.
#define ZFS_GETENTRY                            ((1 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE)
#define ZFS_CLEARCLUSTER                        ((2 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE)
#define ZFS_PUTZFSENTRY                         ((3 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE)
#define ZFS_FINDFREECLUSTER                     ((4 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE)
#define ZFS_COUNTFREECLUSTERS                   ((5 << ZFS_FUNCTION_SHIFT) | ZFS_MODULE)

//                                              0 +
#define ZFS_ERR_NULL_POINTER                    1   // Нулевой указатель менеджера ZFS.
#define ZFS_ERR_NOT_ENOUGH_MEMORY               2   // Не удалось выделить память.
#define ZFS_ERR_DEVICE_DRIVER_FAILED            3   // Ошибка блочного устройства.
#define ZFS_ERR_UNSUPPORTED_OPERATION           4   // Операция не поддерживается.
#define ZFS_ERR_DEVIO_FAILED                    5   // Функция DeviceIoControl вернёла FALSE.
#define ZFS_ERR_INVALID_HANDLE                  6   // Неверный описатель файла.
#define ZFS_ERR_INVALID_CLIENT_ID               7   // Неверный идентификатор клиента.

//                                              20 +
#define ZFS_ERR_ACTIVE_HANDLES                  20  // The partition cannot be unmounted until all active file handles are closed. (There may also be active handles on the cache).
#define ZFS_ERR_NOT_ENOUGH_FREE_SPACE	        21  // Нет свободного места.
#define ZFS_ERR_OUT_OF_BOUNDS_READ              22  // Запрашиваемый блок на чтение превышает максимально возможный в файловой системе.
#define ZFS_ERR_OUT_OF_BOUNDS_WRITE             23  // Запрашиваемый блок на запись превышает максимально возможный в файловой системе.

//                                              30 +
#define ZFS_ERR_FILE_ALREADY_OPEN               30  // Файл уже использует.
#define ZFS_ERR_FILE_NOT_FOUND                  31  // Файл не найден.
#define ZFS_ERR_FILE_OBJECT_IS_A_DIR            32  // Не является файлом.
#define ZFS_ERR_FILE_IS_READ_ONLY               33  // Попытка открыть файл для изменения, который имеет атрибут 'только для чтения'.
#define ZFS_ERR_FILE_INVALID_PATH               34  // Не корректный путь к файлу.
#define ZFS_ERR_FILE_NOT_OPENED_IN_WRITE_MODE   35  // Файл не открыт в режиме записи.
#define ZFS_ERR_FILE_NOT_OPENED_IN_READ_MODE    36  // Файл не открыт в режиме чтения.
#define ZFS_ERR_FILE_DESTINATION_EXISTS         37  // Указанного файла не существует или не является файлом.
#define ZFS_ERR_FILE_DIR_NOT_FOUND              38  // Не найден указанный путь к файлу.
#define ZFS_ERR_FILE_BAD_HANDLE                 39  // Неверный описатель файла.
#define ZFS_ERR_FILE_INCORRECT_OFFSET           40  // Не допустимое смещение.

// Directory Error Codes                        50 +
#define ZFS_ERR_DIR_OBJECT_EXISTS               50  // Файл или папка с таким же именем уже есть в указанной директории.
#define ZFS_ERR_DIR_DIRECTORY_FULL              51  // Директория переполненна (больше нельзя создавать файлы или папки в этой директории).
#define ZFS_ERR_DIR_END_OF_DIR                  52  // Достигнут конец директории.
#define ZFS_ERR_DIR_NOT_EMPTY                   53  // Нельзя удалить директорию в которой имеются папки или файлы.
#define ZFS_ERR_DIR_INVALID_PATH                54  // Указанная диреткория не найдена.
#define ZFS_ERR_DIR_EXTEND_FAILED               55  // Не хватает места,чтобы расширить директорию.
#define ZFS_ERR_DIR_NAME_TOO_LONG               56	// Длина имени превышает допустимый размер.
#define ZFS_ERR_DIR_NAME_BAD                    57	// В имени содержатся недопустимые символы.

//                                              70 +
#define ZFS_ERR_ZFS_NO_FREE_CLUSTERS            70  // На диске нет свободного места.

#endif // __ZFS_ERROR_H_
