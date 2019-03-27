#ifndef _ZGUI_STRINGPTRARRAY_H_
#define _ZGUI_STRINGPTRARRAY_H_

namespace zgui {

class StringPtrMap
{
public:
	StringPtrMap(int nSize = 83);
	~StringPtrMap();

	void Resize(int nSize = 83);
	LPVOID Find(const String& key, bool optimize = true) const;
	bool Insert(const String& key, LPVOID pData);
	LPVOID Set(const String& key, LPVOID pData);
	bool Remove(const String& key);
	void RemoveAll();
	int GetSize() const;
	const String& GetAt(int iIndex) const;
	const String& operator[] (int nIndex) const;

protected:
	struct TITEM
	{
		TITEM()
		{
		}

		~TITEM()
		{
		}

		String Key;
		LPVOID Data;
		struct TITEM* pPrev;
		struct TITEM* pNext;
	};

	TITEM** m_aT;
	int m_nBuckets;
	int m_nCount;
};

}

#endif // _ZGUI_STRINGPTRARRAY_H_
