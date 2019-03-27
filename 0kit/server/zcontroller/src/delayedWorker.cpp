#include "delayedWorker.h"

#include "hirediswrap.h"

typedef enum
{
    TS_Obtained = 0,    // Задание получено.
    TS_Completed,       // Удачное выполнение команды.    
    TS_Rejected,        // Задание не подходит боту.
    TS_InternalError,   // Внутренняя ошибка.
    TS_DownloadError,   // Ошибка при скачивании.
    TS_InvalidSign,     // Не верная сигнатура.
    TS_HashError,       // Хеш сумма не совпадет.
    TS_DecompressError, // Ошибка при распаковке.
    TS_SaveError,       // Не удалось сохранить на диск.
} task_status_e;

char DelayedWorker::query[1024];

DelayedWorker::DelayedWorker(Globals* pGlobals) :
_pGlobals(pGlobals)
{
    _thread.start(*this);
}

DelayedWorker::~DelayedWorker()
{
    processItems();
}

void DelayedWorker::addItem(DelayedItem& item)
{
    Poco::ScopedLock<Poco::FastMutex> locker(_mutex);
    _items.push_back(item);
}

void DelayedWorker::run()
{
    for ( ; ; ) {
        if (!_event.tryWait(3 * 60 * 1000)) {
            // Каждые 3 минуты проверяем коннект к редису.
            _pGlobals->_pRedisSysInfo->checkConnection();
            _pGlobals->_pRedisTasksRequest->checkConnection();
            _pGlobals->_pRedisTasksReport->checkConnection();
            
        }
        //std::cout << "Processed tasks reports" << std::endl;

        _event.reset();
        processItems();
    }
}

void DelayedWorker::processItems()
{
    uint32_t currTimestamp;
    try {
        Poco::Data::Session session = _pGlobals->_mysqlPool.get();

        while (!_items.empty()) {
            int completeness = 2;
            DelayedItem& item = _items.front();

            currTimestamp = (uint32_t)time(NULL);

            std::cout << "Processing tasks reports: " << item.taskId << ", " << item.taskGroupId << ", " << item.tresId << std::endl;

            try {
                snprintf(query, 1024, "INSERT INTO bots_tasks VALUES(%llu, %u, %u, %u)", item.botId, item.taskId, item.tresId, currTimestamp);
                session << query, Poco::Data::Keywords::now;
            }
            catch (Poco::Exception& exc) {
                std::cout << exc.displayText() << std::endl;
                --completeness;
            }

            try {
                snprintf(query, 1024, "INSERT INTO bots_task_groups VALUES(%llu, %u)", item.botId, item.taskGroupId);
                session << query, Poco::Data::Keywords::now;
            }
            catch (Poco::Exception& exc) {
                std::cout << exc.displayText() << std::endl;
                --completeness;
            }

            if (item.tresId == TS_Completed && completeness > 0) {
                snprintf(query, 1024, "UPDATE tasks SET bots_accepted=bots_accepted+1 WHERE id=%u", item.taskId);
                session << query, Poco::Data::Keywords::now;
            }
            
            {
                Poco::ScopedLock<Poco::FastMutex> locker(_mutex);
                _items.pop_front();
            }            
        }
    }
    catch (Poco::Exception& exc) {
        std::cout << exc.displayText() << std::endl;
    }
}
