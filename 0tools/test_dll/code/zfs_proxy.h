#ifndef __ZFS_PROXY_H_
#define __ZFS_PROXY_H_

/** История изменений (сверху-вниз):
    * Добавлено шифрование потока между драйвером и прокси-кодом (алгоритм ARC4 с 48-байтным ключём).
    * Кастомный формат времени заменён UNIX-временем.
    * Изменён формат элемента каталога.
    * Максимально допустимая длина имени равна 32 символам вместо 11.
    * Добавлена функция для получения времени и даты создания/изменения/доступа к файлу (zfs_get_file_time).
    * Добавлена функция для получения версии модуля (zfs_get_version).
    * Добавлена функция для получения размера файла (zfs_get_file_size).
    * Добавлена функция для установки физического размера файла до текущего указателя (zfs_set_end_of_file).
    * Добавлены недостающие константы для работы с функцией zfs_get_file_time.
    * Сделаны исправления в определении ZFS_isERR -> ((x) & ZFS_ERRFLAG).
    * Переименованы некоторые функции функции.
    * Добавлены определения для zfs_open (параметр flags).
    * Добавлены более точные описатели ошибок.
    * Добавлено чуть больше документации к функциям.
    * В функцию zfs_write добавлен необязательный параметр pWritten для проверки корректности записи.
    * Изменена сигнатура функции zfs_getc.
    * Изменена сигнатура функции zfs_open.
    * Изменена сигнатура функции zfs_get_time.
    * Изменена сигнатура функции zfs_get_size.
    * Изменена сигнатура функции zfs_get_filesize.
    * Реинжиниринг кода, результатом которого стало уменьшение кода и увеличение производительности.
    * Исправлена ошибка в функции zfs_get_filesize, связанныя с возвратом неправильного результата.
    * Для системнонезависимых описателей файлов добавлена константа ZFS_ERR_INVALID_HANDLE, вовзращаемая в случае указания неверного описателя.
    * Изменён тип описателя файла с void* на UINT32.
    * Добавлен параметр clientId в функцию zfs_init_proxy().
    * Добавлены константы CLIENT_KEY_SIZE и CLIENT_KEY_ID.
    * Добавлен код ошибки ZFS_ERR_INVALID_CLIENT_ID.
    * В сигнатуры функций добалено соглашение о вызове __stdcall.
*/

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
#define ZFS_MODULE_PROXY                        0
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

// Directory Error Codes                        50 +
#define ZFS_ERR_DIR_OBJECT_EXISTS               50  // Файл или папка с таким же именем уже есть в указанной директории.
#define ZFS_ERR_DIR_DIRECTORY_FULL              51  // Директория переполненна (больше нельзя создавать файлы или папки в этой директории).
#define ZFS_ERR_DIR_END_OF_DIR                  52  // Достигнут конец директории.
#define ZFS_ERR_DIR_NOT_EMPTY                   53  // Нельзя удалить директорию в которой имеются папки или файлы.
#define ZFS_ERR_DIR_INVALID_PATH                54  // Указанная диреткория не найдена.
#define ZFS_ERR_DIR_EXTEND_FAILED               55  // Не хватает места,чтобы расширить директорию.
#define ZFS_ERR_DIR_NAME_TOO_LONG               56  // Длина имени превышает допустимый размер.
#define ZFS_ERR_DIR_NAME_BAD                    57  // В имени содержатся недопустимые символы.

//                                              70 +
#define ZFS_ERR_ZFS_NO_FREE_CLUSTERS            70  // На диске нет свободного места.

typedef UINT32 zfs_handle_t;

#define ZFS_SEEK_SET 1  // С начала файла.
#define ZFS_SEEK_CUR 2  // С текущей позиции в файле.
#define ZFS_SEEK_END 3  // С конца файла.

#define ZFS_MAX_FILENAME 32

// Константы для функции zfs_open (параметр flags).
#define ZFS_MODE_READ       0x01    // Доступ на чтение.
#define	ZFS_MODE_WRITE      0x02    // Доступ на запись.
#define ZFS_MODE_APPEND     0x04    // Режим добавления данных в конец файла.
#define	ZFS_MODE_CREATE     0x08    // Файл будет создан, если его не существует.
#define ZFS_MODE_TRUNCATE   0x10    // Файл будет обнулён, если он существует.

// Константы для функции zfs_get_file_time (параметр aWhat).
#define ZFS_CREATED_TIME 1
#define ZFS_MODIFIED_TIME 2
#define ZFS_ACCESSED_TIME 4

#define CLIENT_KEY_SIZE 48
#define CLIENT_ID_SIZE 16

#pragma pack(push, 8)

typedef struct _zfs_buffer
{
    UINT32 sector;
    UINT32 lru;
    UINT16 numHandles;
    UINT16 persistance;
    UINT8 mode;
    char modified;
    char valid;
    UINT8* pBuffer;
} zfs_buffer_t, *pzfs_buffer_t;

typedef struct _zfs_fetch_context
{
    UINT32 ulChainLength;
    UINT32 ulDirCluster;
    UINT32 ulCurrentClusterLCN;
    UINT32 ulCurrentClusterNum;
    UINT32 ulCurrentEntry;
    zfs_buffer_t* pBuffer;
} zfs_fetch_context_t, *pzfs_fetch_context_t;

typedef struct _zfs_dir_entry
{
    UINT32 filesize;
    UINT32 objectCluster;
    UINT32 currentCluster;
    UINT32 addrCurrentCluster;
    UINT32 dirCluster;
    UINT16 currentItem;
    UINT32 createTime;   // Unix-время создания.
    UINT32 modifiedTime; // Unix-время изменения.
    UINT32 accessedTime; // Unix-время последнего доступа.
    char szWildCard[ZFS_MAX_FILENAME + 1];
    char fileName[ZFS_MAX_FILENAME + 1];
    UINT8 attrib;
    UINT8 special;
    zfs_fetch_context_t fetchContext;
} zfs_dir_entry_t, *pzfs_dir_entry_t;

#pragma pack(pop)


#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif // __cplusplus

/** Инициализирует доступ к файловой системе.

    Возвращает 0 в случае успешной инициализации.
*/
EXTERN_C int __stdcall zfs_init_proxy(unsigned char key[CLIENT_KEY_SIZE], unsigned char clientId[CLIENT_ID_SIZE]);

/** Освобождает все ресурсы и завершает работу с ZFS. */
EXTERN_C void __stdcall zfs_shutdown_proxy();

/** Создаёт директорию по указанному пути.

    Возвращает код ошибки (0 - успешно).
*/
EXTERN_C int __stdcall zfs_mkdir(char* dirName);

/** Удаляем директорию по указанному пути.

    Возвращает код ошибки (0 - успешно).
*/
EXTERN_C int __stdcall zfs_rmdir(char* dirName);

/** Записываем символ по текущему указателю в файле.

    Возвращает код ошибки (0 - успешно).
*/
EXTERN_C int __stdcall zfs_putc(zfs_handle_t handle, unsigned char ch);

/** Записывает данные по текущему указателю в файл.

    pWritten - если указан, возвращается количество записанных данных.
    Возвращает код ошибки (0 - успешно).
*/
EXTERN_C int __stdcall zfs_write(zfs_handle_t handle, unsigned char* buffer, unsigned int size, unsigned int* pWritten);

/** Проверяет достигнут ли конец файла.

    Возвращает 1 если достигнут конец файла, иначе 0.

    В случае возникновения ошибки возвращает отрицательное число.
*/
EXTERN_C int __stdcall zfs_iseof(zfs_handle_t handle);

/** Возвращает символ по текущему указателю из файла. 

    Возвращает считанный байт или ошибку.
    Если достигнут конец файла, возвращает -1.
*/
EXTERN_C int __stdcall zfs_getc(zfs_handle_t handle);

/** Возвращает строку из файла (строка должна оканчиваться символом \n).

    Возвращает код ошибки (0 - успешно).
*/
EXTERN_C int __stdcall zfs_getline(zfs_handle_t handle, char* buffer, int maxSize);

/** Читает данные по текущему указателю из файла.

    Возвращает код ошибки (0 - успешно).
*/
EXTERN_C int __stdcall zfs_read(zfs_handle_t handle, unsigned char* buffer, unsigned int size, unsigned int* pReaded);

/** Ищет первое совпадение по указанному пути. 

    Пока реализована базовая поддержка подстановочных символов (*, ?).

    Возвращает код ошибки (0 - успешно).
*/
EXTERN_C int __stdcall zfs_findfirst(char* find_path, pzfs_dir_entry_t pDirent);

/** Ищет следующее совпадение по пути указанному при вызове функции zfs_findfirst.

    Возвращает код ошибки (0 - успешно).
*/
EXTERN_C int __stdcall zfs_findnext(pzfs_dir_entry_t pDirent);

/** Премещает файл.

    Возвращает код ошибки (0 - успешно).
*/
EXTERN_C int __stdcall zfs_move(char* srcPath, char* dstPath);

/** Создаёт файл с указанными атрибутами.

    Возвращает код ошибки (0 - успешно).
*/
EXTERN_C int __stdcall zfs_open(zfs_handle_t* pHandle, char* fileName, unsigned char flags);

/** Проверяет правильность файлового описателя.

    Возвращает код ошибки (0 - валидный описатель).
*/
EXTERN_C int __stdcall zfs_checkvalid(zfs_handle_t handle);

/** Закрыват ранее открытый файл.

    Возвращает код ошибки (0 - успешно).
*/
EXTERN_C int __stdcall zfs_close(zfs_handle_t handle);

/** Возвращает текущую позицию указателя в файле.

    Возвращает код ошибки (0 - успешно).
*/
EXTERN_C int __stdcall zfs_tell(zfs_handle_t handle, unsigned int* pFilePtr);

/** Перемещает указатель в файле.

    В качестве origin необходимо передать одину из ZFS_SEEK_* констант.

    Возвращает код ошибки (0 - успешно).
    В случае указания неправильного значения origin функция вернёт -3.
    В случае, когда изменить положение указателя не удалось, функция вернёт -2.
*/
EXTERN_C int __stdcall zfs_seek(zfs_handle_t handle, int filePtr, unsigned char origin);

/** Удаляет файл по указанному пути.

    Возвращает код ошибки (0 - успешно).
*/
EXTERN_C int __stdcall zfs_unlink(char* fileName);

/** Получает время создания/изменения/доступа файла.

    Возвращает код ошибки (0 - успешно).
*/
EXTERN_C int __stdcall zfs_get_time(char* fileName, unsigned int aWhat, unsigned int* uTime);

/** Получает версию ядерного модуля.

    В младшем байте возвращает версию модуля (major = ret >> 4; minor = ret & 0x0F).
    В случае ошибки, возвращает код ошибки.
*/
EXTERN_C int __stdcall zfs_get_version();

/** Возвращает размер файла.

	Возвращает код ошибки (0 - успешно).
*/
EXTERN_C int __stdcall zfs_get_filesize(zfs_handle_t handle, unsigned int* pFileSize);

/** Урезает файл до текущей позиции в файле.

	Возвращает код ошибки (0 - успешно).
*/
EXTERN_C int __stdcall zfs_set_end_of_file(zfs_handle_t handle);

#endif // __ZFS_PROXY_H_
