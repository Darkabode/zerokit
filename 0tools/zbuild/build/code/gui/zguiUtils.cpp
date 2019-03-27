#include "zgui.h"

namespace zgui
{
#ifdef _UNICODE
    int isalnum(int c)
    {
        return ((c<=L'z' && c>=L'a') || (c<=L'Z' && c>=L'A') || (c<=L'9' && c>=L'0'));
    }

    int isdigit(int c)
    {
        return (c <= L'9' && c >= L'0');
    }
#else 
    int isalnum(int c)
    {
        return ((c<='z' && c>='a') || (c<='Z' && c>='A') || (c<='9' && c>='0'));
    }

    int isdigit(int c)
    {
        return (c <= '9' && c >= '0');
    }
#endif // _UNICODE


	CPoint::CPoint()
	{
		x = y = 0;
	}

	CPoint::CPoint(const POINT& src)
	{
		x = src.x;
		y = src.y;
	}

	CPoint::CPoint(int _x, int _y)
	{
		x = _x;
		y = _y;
	}

	CPoint::CPoint(LPARAM lParam)
	{
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
	}


	/////////////////////////////////////////////////////////////////////////////////////
	//
	//

	CSize::CSize()
	{
		cx = cy = 0;
	}

	CSize::CSize(const SIZE& src)
	{
		cx = src.cx;
		cy = src.cy;
	}

	CSize::CSize(const RECT rc)
	{
		cx = rc.right - rc.left;
		cy = rc.bottom - rc.top;
	}

	CSize::CSize(int _cx, int _cy)
	{
		cx = _cx;
		cy = _cy;
	}


	/////////////////////////////////////////////////////////////////////////////////////
	//
	//

	CDuiRect::CDuiRect()
	{
		left = top = right = bottom = 0;
	}

	CDuiRect::CDuiRect(const RECT& src)
	{
		left = src.left;
		top = src.top;
		right = src.right;
		bottom = src.bottom;
	}

	CDuiRect::CDuiRect(int iLeft, int iTop, int iRight, int iBottom)
	{
		left = iLeft;
		top = iTop;
		right = iRight;
		bottom = iBottom;
	}

	int CDuiRect::GetWidth() const
	{
		return right - left;
	}

	int CDuiRect::GetHeight() const
	{
		return bottom - top;
	}

	void CDuiRect::Empty()
	{
		left = top = right = bottom = 0;
	}

	bool CDuiRect::IsNull() const
	{
		return (left == 0 && right == 0 && top == 0 && bottom == 0); 
	}

	void CDuiRect::Join(const RECT& rc)
	{
		if( rc.left < left ) left = rc.left;
		if( rc.top < top ) top = rc.top;
		if( rc.right > right ) right = rc.right;
		if( rc.bottom > bottom ) bottom = rc.bottom;
	}

	void CDuiRect::ResetOffset()
	{
		::OffsetRect(this, -left, -top);
	}

	void CDuiRect::Normalize()
	{
		if( left > right ) { int iTemp = left; left = right; right = iTemp; }
		if( top > bottom ) { int iTemp = top; top = bottom; bottom = iTemp; }
	}

	void CDuiRect::Offset(int cx, int cy)
	{
		::OffsetRect(this, cx, cy);
	}

	void CDuiRect::Inflate(int cx, int cy)
	{
		::InflateRect(this, cx, cy);
	}

	void CDuiRect::Deflate(int cx, int cy)
	{
		::InflateRect(this, -cx, -cy);
	}

	void CDuiRect::Union(CDuiRect& rc)
	{
		::UnionRect(this, this, &rc);
	}


    bool Helper::splitString(const String& str, const String& seps, const String& quotes, int& val1, int& val2, int& val3, int& val4)
    {
        StringArray params;
        params.addTokens(str, seps, quotes);
        if (params.size() != 4) {
            return false;
        }

        val1 = params[0].getIntValue();
        val2 = params[1].getIntValue();
        val3 = params[2].getIntValue();
        val4 = params[3].getIntValue();

        return true;
    }

    bool Helper::splitString(const String& str, const String& seps, const String& quotes, int& val1, int& val2)
    {
        StringArray params;
        params.addTokens(str, seps, quotes);
        if (params.size() != 2) {
            return false;
        }

        val1 = params[0].getIntValue();
        val2 = params[1].getIntValue();

        return true;
    }

	CStdPtrArray::CStdPtrArray(int iPreallocSize) : m_ppVoid(NULL), m_nCount(0), m_nAllocated(iPreallocSize)
	{
		ASSERT(iPreallocSize>=0);
        if (iPreallocSize > 0) {
            m_ppVoid = static_cast<LPVOID*>(::HeapAlloc(::GetProcessHeap()/*gHeap*/, 0, iPreallocSize * sizeof(LPVOID)));
        }
	}

	CStdPtrArray::CStdPtrArray(const CStdPtrArray& src) : m_ppVoid(NULL), m_nCount(0), m_nAllocated(0)
	{
		for(int i=0; i<src.GetSize(); i++)
			Add(src.GetAt(i));
	}

	CStdPtrArray::~CStdPtrArray()
	{
        if (m_ppVoid != NULL) {
            ::HeapFree(::GetProcessHeap()/*gHeap*/, 0, m_ppVoid);
        }
	}

	void CStdPtrArray::Empty()
	{
        if (m_ppVoid != NULL) {
            ::HeapFree(::GetProcessHeap()/*gHeap*/, 0, m_ppVoid);
        }
		m_ppVoid = NULL;
		m_nCount = m_nAllocated = 0;
	}

	void CStdPtrArray::Resize(int iSize)
	{
		Empty();
		m_ppVoid = static_cast<LPVOID*>(::HeapAlloc(::GetProcessHeap()/*gHeap*/, 0, iSize * sizeof(LPVOID)));
		::ZeroMemory(m_ppVoid, iSize * sizeof(LPVOID));
		m_nAllocated = iSize;
		m_nCount = iSize;
	}

	bool CStdPtrArray::IsEmpty() const
	{
		return m_nCount == 0;
	}

	bool CStdPtrArray::Add(LPVOID pData)
	{
		if( ++m_nCount >= m_nAllocated) {
			int nAllocated = m_nAllocated * 2;
            if( nAllocated == 0 ) nAllocated = 11;
			LPVOID* ppVoid = static_cast<LPVOID*>(cvec_reallocate(m_ppVoid, nAllocated * sizeof(LPVOID)));
			if( ppVoid != NULL ) {
				m_nAllocated = nAllocated;
				m_ppVoid = ppVoid;
			}
			else {
				--m_nCount;
				return false;
			}
		}
		m_ppVoid[m_nCount - 1] = pData;
		return true;
	}

	bool CStdPtrArray::InsertAt(int iIndex, LPVOID pData)
	{
		if( iIndex == m_nCount ) return Add(pData);
		if( iIndex < 0 || iIndex > m_nCount ) return false;
		if( ++m_nCount >= m_nAllocated) {
			int nAllocated = m_nAllocated * 2;
			if( nAllocated == 0 ) nAllocated = 11;
			LPVOID* ppVoid = static_cast<LPVOID*>(cvec_reallocate(m_ppVoid, nAllocated * sizeof(LPVOID)));
			if( ppVoid != NULL ) {
				m_nAllocated = nAllocated;
				m_ppVoid = ppVoid;
			}
			else {
				--m_nCount;
				return false;
			}
		}
		memmove(&m_ppVoid[iIndex + 1], &m_ppVoid[iIndex], (m_nCount - iIndex - 1) * sizeof(LPVOID));
		m_ppVoid[iIndex] = pData;
		return true;
	}

	bool CStdPtrArray::SetAt(int iIndex, LPVOID pData)
	{
		if( iIndex < 0 || iIndex >= m_nCount ) return false;
		m_ppVoid[iIndex] = pData;
		return true;
	}

	bool CStdPtrArray::Remove(int iIndex)
	{
		if( iIndex < 0 || iIndex >= m_nCount ) return false;
		if( iIndex < --m_nCount ) ::CopyMemory(m_ppVoid + iIndex, m_ppVoid + iIndex + 1, (m_nCount - iIndex) * sizeof(LPVOID));
		return true;
	}

	int CStdPtrArray::Find(LPVOID pData) const
	{
		for( int i = 0; i < m_nCount; i++ ) if( m_ppVoid[i] == pData ) return i;
		return -1;
	}

	int CStdPtrArray::GetSize() const
	{
		return m_nCount;
	}

	LPVOID* CStdPtrArray::GetData()
	{
		return m_ppVoid;
	}

	LPVOID CStdPtrArray::GetAt(int iIndex) const
	{
		if( iIndex < 0 || iIndex >= m_nCount ) return NULL;
		return m_ppVoid[iIndex];
	}

	LPVOID CStdPtrArray::operator[] (int iIndex) const
	{
		ASSERT(iIndex>=0 && iIndex<m_nCount);
		return m_ppVoid[iIndex];
	}


	/////////////////////////////////////////////////////////////////////////////////////
	//
	//

	CStdValArray::CStdValArray(int iElementSize, int iPreallocSize /*= 0*/) : 
	m_pVoid(NULL), 
		m_nCount(0), 
		m_iElementSize(iElementSize), 
		m_nAllocated(iPreallocSize)
	{
		ASSERT(iElementSize>0);
		ASSERT(iPreallocSize>=0);
		if( iPreallocSize > 0 ) m_pVoid = static_cast<LPBYTE>(::HeapAlloc(::GetProcessHeap()/*gHeap*/, 0, iPreallocSize * m_iElementSize));
	}

	CStdValArray::~CStdValArray()
	{
        if (m_pVoid != NULL) {
            ::HeapFree(::GetProcessHeap()/*gHeap*/, 0, m_pVoid);
        }
	}

	void CStdValArray::Empty()
	{   
		m_nCount = 0;  // NOTE: We keep the memory in place
	}

	bool CStdValArray::IsEmpty() const
	{
		return m_nCount == 0;
	}

	bool CStdValArray::Add(LPCVOID pData)
	{
		if( ++m_nCount >= m_nAllocated) {
			int nAllocated = m_nAllocated * 2;
			if( nAllocated == 0 ) nAllocated = 11;
			LPBYTE pVoid = static_cast<LPBYTE>(cvec_reallocate(m_pVoid, nAllocated * m_iElementSize));
			if( pVoid != NULL ) {
				m_nAllocated = nAllocated;
				m_pVoid = pVoid;
			}
			else {
				--m_nCount;
				return false;
			}
		}
		::CopyMemory(m_pVoid + ((m_nCount - 1) * m_iElementSize), pData, m_iElementSize);
		return true;
	}

	bool CStdValArray::Remove(int iIndex)
	{
		if( iIndex < 0 || iIndex >= m_nCount ) return false;
		if( iIndex < --m_nCount ) ::CopyMemory(m_pVoid + (iIndex * m_iElementSize), m_pVoid + ((iIndex + 1) * m_iElementSize), (m_nCount - iIndex) * m_iElementSize);
		return true;
	}

	int CStdValArray::GetSize() const
	{
		return m_nCount;
	}

	LPVOID CStdValArray::GetData()
	{
		return static_cast<LPVOID>(m_pVoid);
	}

	LPVOID CStdValArray::GetAt(int iIndex) const
	{
		if( iIndex < 0 || iIndex >= m_nCount ) return NULL;
		return m_pVoid + (iIndex * m_iElementSize);
	}

	LPVOID CStdValArray::operator[] (int iIndex) const
	{
		ASSERT(iIndex>=0 && iIndex<m_nCount);
		return m_pVoid + (iIndex * m_iElementSize);
	}

	static UINT HashKey(LPCTSTR Key)
	{
		UINT i = 0;
		SIZE_T len = lstrlen(Key);
        while (len-- > 0) {
            i = (i << 5) + i + Key[len];
        }

		return i;
	}

	static UINT HashKey(const String& Key)
	{
		return HashKey(Key.toWideCharPointer());
	};

	CStdStringPtrMap::CStdStringPtrMap(int nSize) : m_nCount(0)
	{
        if (nSize < 16) {
            nSize = 16;
        }
		m_nBuckets = nSize;
		m_aT = (TITEM**)memalloc(sizeof(TITEM*) * nSize);
		memset(m_aT, 0, nSize * sizeof(TITEM*));
	}

	CStdStringPtrMap::~CStdStringPtrMap()
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
			memfree(m_aT);
			m_aT = NULL;
		}
	}

	void CStdStringPtrMap::RemoveAll()
	{
		this->Resize(m_nBuckets);
	}

	void CStdStringPtrMap::Resize(int nSize)
	{
		if( m_aT ) {
			int len = m_nBuckets;
			while( len-- ) {
				TITEM* pItem = m_aT[len];
				while( pItem ) {
					TITEM* pKill = pItem;
					pItem = pItem->pNext;
					delete pKill;
				}
			}
			memfree(m_aT);
			m_aT = NULL;
		}

		if( nSize < 0 ) nSize = 0;
		if( nSize > 0 ) {
			m_aT = (TITEM**)memalloc(sizeof(TITEM*) * nSize);
			memset(m_aT, 0, nSize * sizeof(TITEM*));
		} 
		m_nBuckets = nSize;
		m_nCount = 0;
	}

	LPVOID CStdStringPtrMap::Find(const String& key, bool optimize) const
	{
        if (m_nBuckets == 0 || GetSize() == 0) {
            return NULL;
        }

		UINT slot = HashKey(key) % m_nBuckets;
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

	bool CStdStringPtrMap::Insert(const String& key, LPVOID pData)
	{
        if (m_nBuckets == 0 ) {
            return false;
        }
        if (Find(key)) {
            return false;
        }

		// Add first in bucket
		UINT slot = HashKey(key) % m_nBuckets;
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

	LPVOID CStdStringPtrMap::Set(const String& key, LPVOID pData)
	{
        if (m_nBuckets == 0) {
            return pData;
        }

		if (GetSize() > 0) {
			UINT slot = HashKey(key) % m_nBuckets;
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

	bool CStdStringPtrMap::Remove(const String& key)
	{
        if (m_nBuckets == 0 || GetSize() == 0) {
            return false;
        }

		UINT slot = HashKey(key) % m_nBuckets;
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

	int CStdStringPtrMap::GetSize() const
	{
#if 0//def _DEBUG
		int nCount = 0;
		int len = m_nBuckets;
		while( len-- ) {
			for( const TITEM* pItem = m_aT[len]; pItem; pItem = pItem->pNext ) nCount++;
		}
		ASSERT(m_nCount==nCount);
#endif
		return m_nCount;
	}

	LPCTSTR CStdStringPtrMap::GetAt(int iIndex) const
	{
		if( m_nBuckets == 0 || GetSize() == 0 ) return false;

		int pos = 0;
		int len = m_nBuckets;
		while( len-- ) {
			for( TITEM* pItem = m_aT[len]; pItem; pItem = pItem->pNext ) {
				if( pos++ == iIndex ) {
					return pItem->Key.toWideCharPointer();
				}
			}
		}

		return NULL;
	}

	LPCTSTR CStdStringPtrMap::operator[] (int nIndex) const
	{
		return GetAt(nIndex);
	}


	/////////////////////////////////////////////////////////////////////////////////////
	//
	//

	CWaitCursor::CWaitCursor()
	{
		m_hOrigCursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
	}

	CWaitCursor::~CWaitCursor()
	{
		::SetCursor(m_hOrigCursor);
	}

} // namespace zgui