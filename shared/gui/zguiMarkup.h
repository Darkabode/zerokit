#ifndef __ZGUI_MARKUP_H_
#define __ZGUI_MARKUP_H_

namespace zgui {

enum
{
    XMLFILE_ENCODING_UTF8 = 0,
    XMLFILE_ENCODING_UNICODE = 1,
    XMLFILE_ENCODING_ASNI = 2,
};

class Markup;
class MarkupNode;


class Markup
{
    friend class MarkupNode;
public:
    Markup(const char* xmlName = 0);
    ~Markup();

    bool Load(const String& xmlName);
    void Release();
    bool IsValid() const;

	const String& getXmlName() const { return _xmlName; }

    void SetPreserveWhitespace(bool bPreserve = true);
    void GetLastErrorMessage(LPTSTR pstrMessage, SIZE_T cchMax) const;
    void GetLastErrorLocation(LPTSTR pstrSource, SIZE_T cchMax) const;

    MarkupNode GetRoot();

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
    wchar_t m_szErrorMsg[100];
    wchar_t m_szErrorXML[50];
    bool m_bPreserveWhitespace;

private:
    bool _Parse();
    bool _Parse(ULONG iParent);
    XMLELEMENT* _ReserveElement();
    void _SkipWhitespace();
    static LPCTSTR _SkipWhitespace(LPCTSTR pStr);
    void _SkipIdentifier();
    bool _ParseData(wchar_t cEnd);
    bool _ParseAttributes();
    bool _Failed(LPCTSTR pstrError, LPCTSTR pstrLocation = NULL);

	String _xmlName;
    LPTSTR _pstrXML;
    LPTSTR _pstrDest;
};


class MarkupNode
{
    friend class Markup;
private:
    MarkupNode();
    MarkupNode(Markup* pOwner, int iPos);

public:
    bool IsValid() const;

    MarkupNode GetParent();
    MarkupNode GetSibling();
    MarkupNode GetChild();
    MarkupNode GetChild(LPCTSTR pstrName);

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
    bool GetAttributeValue(LPCTSTR pstrName, String& pstrValue);

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
    Markup* m_pOwner;
};

} // namespace zgui

#endif // __ZGUI_MARKUP_H_
