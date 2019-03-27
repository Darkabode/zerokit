#ifndef __MEMPOOL_H_
#define __MEMPOOL_H_

typedef struct _memory_pool
{
	size_t blockSize;		// Размер блока
	int maxAlloc;			// Максимально возможное количество блоков для пула
	int allocated;		// Выделенное количество блоков в пуле
	char** blocks;		// Список блоков
	pthread_mutex_t mutex;	// Мютекс для взаимоблокировки потоков

	char* name;			// Имя пула
} memory_pool_t;

int mempool_init(memory_pool_t* pPool, size_t blockSize, int preAlloc, int maxAlloc);
void mempool_done(memory_pool_t* pPool);

void* mempool_get(memory_pool_t* pPool);
void mempool_release(memory_pool_t* pPool, void* pMem);

#endif // __MEMPOOL_H_
