#include "zgui.h"

namespace zgui {

StringPtrMap::StringPtrMap(int nSize) :
m_nCount(0)
{
	if (nSize < 16) {
		nSize = 16;
	}
	m_nBuckets = nSize;
	m_aT = (TITEM**)fn_memalloc(sizeof(TITEM*)* nSize);
	__stosb((uint8_t*)m_aT, 0, nSize * sizeof(TITEM*));
}

StringPtrMap::~StringPtrMap()
{
	if (m_aT) {
		int len = m_nBuckets;
		while (len--) {
			TITEM* pItem = m_aT[len];
			while (pItem) {
				TITEM* pKill = pItem;
				pItem = pItem->pNext;
				delete pKill;
			}
		}
		fn_memfree(m_aT);
		m_aT = NULL;
	}
}

void StringPtrMap::RemoveAll()
{
	this->Resize(m_nBuckets);
}

void StringPtrMap::Resize(int nSize)
{
	if (m_aT != 0) {
		int len = m_nBuckets;
		while (len--) {
			TITEM* pItem = m_aT[len];
			while (pItem != 0) {
				TITEM* pKill = pItem;
				pItem = pItem->pNext;
				delete pKill;
			}
		}
		fn_memfree(m_aT);
		m_aT = NULL;
	}

	if (nSize < 0) nSize = 0;
	if (nSize > 0) {
		m_aT = (TITEM**)fn_memalloc(sizeof(TITEM*)* nSize);
		__stosb((uint8_t*)m_aT, 0, nSize * sizeof(TITEM*));
	}
	m_nBuckets = nSize;
	m_nCount = 0;
}

LPVOID StringPtrMap::Find(const String& key, bool optimize) const
{
	if (m_nBuckets == 0 || GetSize() == 0) {
		return NULL;
	}

	UINT slot = (UINT)key.hashCode() % m_nBuckets;
	for (TITEM* pItem = m_aT[slot]; pItem; pItem = pItem->pNext) {
		if (pItem->Key == key) {
			if (optimize && pItem != m_aT[slot]) {
				if (pItem->pNext) {
					pItem->pNext->pPrev = pItem->pPrev;
				}
				pItem->pPrev->pNext = pItem->pNext;
				pItem->pPrev = NULL;
				pItem->pNext = m_aT[slot];
				pItem->pNext->pPrev = pItem;
				m_aT[slot] = pItem;
			}
			return pItem->Data;
		}
	}

	return NULL;
}

bool StringPtrMap::Insert(const String& key, LPVOID pData)
{
	if (m_nBuckets == 0) {
		return false;
	}
	if (Find(key)) {
		return false;
	}

	// Add first in bucket
	UINT slot = (UINT)key.hashCode() % m_nBuckets;
	TITEM* pItem = new TITEM;
	pItem->Key = key;
	pItem->Data = pData;
	pItem->pPrev = NULL;
	pItem->pNext = m_aT[slot];
	if (pItem->pNext) {
		pItem->pNext->pPrev = pItem;
	}
	m_aT[slot] = pItem;
	m_nCount++;

	return true;
}

LPVOID StringPtrMap::Set(const String& key, LPVOID pData)
{
	if (m_nBuckets == 0) {
		return pData;
	}

	if (GetSize() > 0) {
		UINT slot = (UINT)key.hashCode() % m_nBuckets;
		// Modify existing item
		for (TITEM* pItem = m_aT[slot]; pItem; pItem = pItem->pNext) {
			if (pItem->Key == key) {
				LPVOID pOldData = pItem->Data;
				pItem->Data = pData;
				return pOldData;
			}
		}
	}

	Insert(key, pData);
	return NULL;
}

bool StringPtrMap::Remove(const String& key)
{
	if (m_nBuckets == 0 || GetSize() == 0) {
		return false;
	}

	UINT slot = (UINT)key.hashCode() % m_nBuckets;
	TITEM** ppItem = &m_aT[slot];
	while (*ppItem) {
		if ((*ppItem)->Key == key) {
			TITEM* pKill = *ppItem;
			*ppItem = (*ppItem)->pNext;
			if (*ppItem) {
				(*ppItem)->pPrev = pKill->pPrev;
			}
			delete pKill;
			m_nCount--;
			return true;
		}
		ppItem = &((*ppItem)->pNext);
	}

	return false;
}

int StringPtrMap::GetSize() const
{
#if 0//def _DEBUG
	int nCount = 0;
	int len = m_nBuckets;
	while (len--) {
		for (const TITEM* pItem = m_aT[len]; pItem; pItem = pItem->pNext) nCount++;
	}
	zgui_assert(m_nCount == nCount);
#endif
	return m_nCount;
}

const String& StringPtrMap::GetAt(int iIndex) const
{
	if (m_nBuckets == 0 || GetSize() == 0) {
		return String::empty;
	}

	int pos = 0;
	int len = m_nBuckets;
	while (len--) {
		for (TITEM* pItem = m_aT[len]; pItem; pItem = pItem->pNext) {
			if (pos++ == iIndex) {
				return pItem->Key;
			}
		}
	}

	return String::empty;
}

const String& StringPtrMap::operator[] (int nIndex) const
{
	return GetAt(nIndex);
}

}