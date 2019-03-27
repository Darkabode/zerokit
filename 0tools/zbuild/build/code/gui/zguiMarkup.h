#ifndef __UIMARKUP_H__
#define __UIMARKUP_H__

#pragma once

namespace zgui {

enum
{
    XMLFILE_ENCODING_UTF8 = 0,
    XMLFILE_ENCODING_UNICODE = 1,
    XMLFILE_ENCODING_ASNI = 2,
};

class CMarkup;
class CMarkupNode;


class CMarkup
{
    friend class CMarkupNode;
public:
    CMarkup(const char* xmlName = 0);
    ~CMarkup();

    bool Load(const char* xmlName);
//     bool LoadFromMem(BYTE* pByte, DWORD dwSize, int encoding = XMLFILE_ENCODING_UTF8);
//     bool LoadFromFile(LPCTSTR pstrFilename, int encoding = XMLFILE_ENCODING_UTF8);
    void Release();
    bool IsValid() const;

    void SetPreserveWhitespace(bool bPreserve = true);
    void GetLastErrorMessage(LPTSTR pstrMessage, SIZE_T cchMax) const;
    void GetLastErrorLocation(LPTSTR pstrSource, SIZE_T cchMax) const;

    CMarkupNode GetRoot();

private:
    typedef struct tagXMLELEMENT
    {
        ULONG iStart;
        ULONG iChild;
        ULONG iNext;
        ULONG iParent;
        ULONG iData;
    } XMLELEMENT;

    LPTSTR _xml;
    XMLELEMENT* _pElements;
    ULONG m_nElements;
    ULONG m_nReservedElements;
    TCHAR m_szErrorMsg[100];
    TCHAR m_szErrorXML[50];
    bool m_bPreserveWhitespace;

private:
    bool _Parse();
    bool _Parse(ULONG iParent);
    XMLELEMENT* _ReserveElement();
    void _SkipWhitespace();
    static LPCTSTR _SkipWhitespace(LPCTSTR pStr);
    void _SkipIdentifier();
    bool _ParseData(TCHAR cEnd);
    bool _ParseAttributes();
    bool _Failed(LPCTSTR pstrError, LPCTSTR pstrLocation = NULL);

    LPTSTR _pstrXML;
    LPTSTR _pstrDest;
};


class CMarkupNode
{
    friend class CMarkup;
private:
    CMarkupNode();
    CMarkupNode(CMarkup* pOwner, int iPos);

public:
    bool IsValid() const;

    CMarkupNode GetParent();
    CMarkupNode GetSibling();
    CMarkupNode GetChild();
    CMarkupNode GetChild(LPCTSTR pstrName);

    bool HasSiblings() const;
    bool HasChildren() const;
    LPCTSTR GetName() const;
    LPCTSTR GetValue() const;

    bool HasAttributes();
    bool HasAttribute(LPCTSTR pstrName);
    int GetAttributeCount();
    LPCTSTR GetAttributeName(int iIndex);
    LPCTSTR GetAttributeValue(int iIndex);
    LPCTSTR GetAttributeValue(LPCTSTR pstrName);
    bool GetAttributeValue(int iIndex, LPTSTR pstrValue, SIZE_T cchMax);
    bool GetAttributeValue(LPCTSTR pstrName, LPTSTR pstrValue, SIZE_T cchMax);

private:
    void _MapAttributes();

    enum { MAX_XML_ATTRIBUTES = 64 };

    typedef struct
    {
        ULONG iName;
        ULONG iValue;
    } XMLATTRIBUTE;

    int m_iPos;
    int m_nAttributes;
    XMLATTRIBUTE m_aAttributes[MAX_XML_ATTRIBUTES];
    CMarkup* m_pOwner;
};

} // namespace zgui

#endif // __UIMARKUP_H__
