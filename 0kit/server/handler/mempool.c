#include <stdlib.h>
#include <pthread.h>

#include "../../shared_code/types.h"

#include "logger.h"
#include "mempool.h"
#include "errors.h"

#define BLOCK_RESERVE 128

// Первый байт блока - это байт состояния, в котором хранится информация о его доступности и прочие атрибуты
// 
// Бит 0 - определяем доступен ли блок (1) или используется (0)
//
//

int mempool_init(memory_pool_t* pPool, size_t blockSize, int preAlloc, int maxAlloc)
{
	int i;
	char* ptr;
	int r = BLOCK_RESERVE;

	if (preAlloc > r)
		r = preAlloc;
	if (maxAlloc > 0 && maxAlloc < r)
		r = maxAlloc;

	pPool->blocks = malloc(r * sizeof(char*));
	if (pPool->blocks == NULL) {
		logger_error("Memory Pool: Out of memory during initialization");
		return ERR_OUT_OF_MEMORY;
	}
	
	pPool->blockSize = blockSize;
	pPool->maxAlloc = maxAlloc;
	pPool->allocated = r;
	for (i = 0; i < r; ++i) {
		ptr = calloc(1, 1 + blockSize);
		if (ptr == NULL) {
			logger_error("Memory Pool: Out of memory during initialization");
			mempool_done(pPool);
			return ERR_OUT_OF_MEMORY;
		}
		*ptr = 0x01;
		pPool->blocks[i] = ptr;		
	}

	return ERR_OK;
}

void mempool_done(memory_pool_t* pPool)
{
	int i;
	
	for (i = 0; i < pPool->allocated; ++i)
		free(pPool->blocks[i]);

	free(pPool->blocks);
}

void* mempool_get(memory_pool_t* pPool)
{
	int i;
	char** newBlocks;
	char* ptr = NULL;

	pthread_mutex_lock(&pPool->mutex);

	for (i = 0; i < pPool->allocated; ++i) {
		if (*(pPool->blocks[i]) == 0x01)
			break;
	}

	if (i >= pPool->allocated) {
		if (pPool->maxAlloc == 0 || pPool->allocated < pPool->maxAlloc) {
			if ((newBlocks = realloc(pPool->blocks, (pPool->allocated + 1) * sizeof(char*))) == NULL) {
				logger_error("Memory Pool '%s': Out of memory", pPool->name);
			}
			else {
				// Данное равенство нужно,т. к. при перераспределнии памяти блок мог переместиться.
				pPool->blocks = newBlocks;
				ptr = malloc(1 + pPool->blockSize);
				if (ptr == NULL) {
					newBlocks = realloc(pPool->blocks, pPool->allocated * sizeof(char*));
					if (newBlocks != NULL && newBlocks != pPool->blocks)
						pPool->blocks = newBlocks;
					logger_error("Memory Pool '%s': Out of memory", pPool->name);
				}
				else {
					logger_info("'%s' pool capacity: %s/%d", pPool->name, pPool->allocated, pPool->maxAlloc);
					pPool->blocks[pPool->allocated++] = ptr;
					*ptr = 0x00;
					ptr++;
				}
			}
		}
		else {
			logger_error("Memory Pool '%s': Maximum allowed blocks number was reached", pPool->name);
		}
	}
	else {
		ptr = pPool->blocks[i];
		*ptr = 0x00;
		ptr++; 
	}

	pthread_mutex_unlock(&pPool->mutex);
	return ptr;	
}

void mempool_release(memory_pool_t* pPool, void* pMem)
{
	int i;
	char* ptr;

	pthread_mutex_lock(&pPool->mutex);

	for (i = 0; i < pPool->allocated; ++i) {
		ptr = pPool->blocks[i];
		if ((char*)pMem == (ptr + 1) && (*ptr == 0x00)) {
			*ptr = 0x01;
			pthread_mutex_unlock(&pPool->mutex);	
			return;
		}
	}

	pthread_mutex_unlock(&pPool->mutex);
	logger_error("Memory Pool '%s': Cannot release undefined block of memory", pPool->name);
}
