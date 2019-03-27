#include "zgui.h"

#ifndef TRACE
#define TRACE
#endif

namespace zgui {

MarkupNode::MarkupNode() :
m_pOwner(0)
{
}

MarkupNode::MarkupNode(Markup* pOwner, int iPos) :
m_pOwner(pOwner),
m_iPos(iPos),
m_nAttributes(0)
{
}

MarkupNode MarkupNode::GetSibling()
{
	if (m_pOwner == 0) {
		return MarkupNode();
	}
    ULONG iPos = m_pOwner->_pElements[m_iPos].iNext;
	if (iPos == 0) {
		return MarkupNode();
	}
    return MarkupNode(m_pOwner, iPos);
}

bool MarkupNode::HasSiblings() const
{
    if( m_pOwner == 0 ) return false;
    ULONG iPos = m_pOwner->_pElements[m_iPos].iNext;
    return iPos > 0;
}

MarkupNode MarkupNode::GetChild()
{
    if( m_pOwner == 0 ) return MarkupNode();
    ULONG iPos = m_pOwner->_pElements[m_iPos].iChild;
    if( iPos == 0 ) return MarkupNode();
    return MarkupNode(m_pOwner, iPos);
}

MarkupNode MarkupNode::GetChild(LPCTSTR pstrName)
{
    if (m_pOwner == 0) {
        return MarkupNode();
    }

    ULONG iPos = m_pOwner->_pElements[m_iPos].iChild;
    while (iPos != 0) {
        if (fn_lstrcmpW(m_pOwner->_xml + m_pOwner->_pElements[iPos].iStart, pstrName) == 0) {
            return MarkupNode(m_pOwner, iPos);
        }
        iPos = m_pOwner->_pElements[iPos].iNext;
    }
    return MarkupNode();
}

bool MarkupNode::HasChildren() const
{
    if( m_pOwner == 0 ) return false;
    return m_pOwner->_pElements[m_iPos].iChild != 0;
}

MarkupNode MarkupNode::GetParent()
{
    if( m_pOwner == 0 ) return MarkupNode();
    ULONG iPos = m_pOwner->_pElements[m_iPos].iParent;
    if( iPos == 0 ) return MarkupNode();
    return MarkupNode(m_pOwner, iPos);
}

bool MarkupNode::IsValid() const
{
    return m_pOwner != 0;
}

LPCTSTR MarkupNode::GetName() const
{
    if( m_pOwner == 0 ) return 0;
    return m_pOwner->_xml + m_pOwner->_pElements[m_iPos].iStart;
}

LPCTSTR MarkupNode::GetValue() const
{
    if( m_pOwner == 0 ) return 0;
    return m_pOwner->_xml + m_pOwner->_pElements[m_iPos].iData;
}

LPCTSTR MarkupNode::GetAttributeName(int iIndex)
{
    if (m_pOwner == 0) {
        return 0;
    }
    if (m_nAttributes == 0) {
        _MapAttributes();
    }
    if (iIndex < 0 || iIndex >= m_nAttributes) {
        return L"";
    }
    return m_pOwner->_xml + m_aAttributes[iIndex].iName;
}

LPCTSTR MarkupNode::GetAttributeValue(int iIndex)
{
    if (m_pOwner == 0) {
        return 0;
    }
    if (m_nAttributes == 0) {
        _MapAttributes();
    }
    if (iIndex < 0 || iIndex >= m_nAttributes) {
        return L"";
    }
    return m_pOwner->_xml + m_aAttributes[iIndex].iValue;
}

LPCTSTR MarkupNode::GetAttributeValue(LPCTSTR pstrName)
{
    if (m_pOwner == 0) {
        return 0;
    }
    if (m_nAttributes == 0) {
        _MapAttributes();
    }
    for (int i = 0; i < m_nAttributes; ++i) {
        if (fn_lstrcmpW(m_pOwner->_xml + m_aAttributes[i].iName, pstrName) == 0) {
            return m_pOwner->_xml + m_aAttributes[i].iValue;
        }
    }
    return L"";
}

bool MarkupNode::GetAttributeValue(int iIndex, LPTSTR pstrValue, SIZE_T cchMax)
{
    if (m_pOwner == 0) {
        return false;
    }
    if (m_nAttributes == 0) {
        _MapAttributes();
    }
    if (iIndex < 0 || iIndex >= m_nAttributes) {
        return false;
    }
    
    fn_lstrcpynW(pstrValue, m_pOwner->_xml + m_aAttributes[iIndex].iValue, cchMax);
    return true;
}

bool MarkupNode::GetAttributeValue(LPCTSTR pstrName, String& strValue)
{
    if (m_pOwner == 0) {
        return false;
    }
    if (m_nAttributes == 0) {
        _MapAttributes();
    }
    for (int i = 0; i < m_nAttributes; ++i) {
        if (fn_lstrcmpW(m_pOwner->_xml + m_aAttributes[i].iName, pstrName) == 0) {
            strValue = m_pOwner->_xml + m_aAttributes[i].iValue;
            return true;
        }
    }
    return false;
}

int MarkupNode::GetAttributeCount()
{
    if( m_pOwner == 0 ) return 0;
    if( m_nAttributes == 0 ) _MapAttributes();
    return m_nAttributes;
}

bool MarkupNode::HasAttributes()
{
    if( m_pOwner == 0 ) return false;
    if( m_nAttributes == 0 ) _MapAttributes();
    return m_nAttributes > 0;
}

bool MarkupNode::HasAttribute(LPCTSTR pstrName)
{
    if( m_pOwner == 0 ) return false;
    if( m_nAttributes == 0 ) _MapAttributes();
    for( int i = 0; i < m_nAttributes; i++ ) {
        if (fn_lstrcmpW(m_pOwner->_xml + m_aAttributes[i].iName, pstrName) == 0) {
            return true;
        }
    }
    return false;
}

void MarkupNode::_MapAttributes()
{
    m_nAttributes = 0;
    LPCTSTR pstr = m_pOwner->_xml + m_pOwner->_pElements[m_iPos].iStart;
    LPCTSTR pstrEnd = m_pOwner->_xml + m_pOwner->_pElements[m_iPos].iData;
    pstr += fn_lstrlenW(pstr) + 1;
    while (pstr < pstrEnd) {
        pstr = Markup::_SkipWhitespace(pstr);
        m_aAttributes[m_nAttributes].iName = pstr - m_pOwner->_xml;
        pstr += fn_lstrlenW(pstr) + 1;
        pstr = Markup::_SkipWhitespace(pstr);
        if (*pstr++ != L'\"') {
            return; // if( *pstr != _T('\"') ) { pstr = ::CharNext(pstr); return; }
        }
        
        m_aAttributes[m_nAttributes++].iValue = pstr - m_pOwner->_xml;
        if (m_nAttributes >= MAX_XML_ATTRIBUTES) {
            return;
        }
        pstr += fn_lstrlenW(pstr) + 1;
    }
}


///////////////////////////////////////////////////////////////////////////////////////
//
//
//

Markup::Markup(const char* xmlName)
{
    _xml = 0;
    _pElements = 0;
    m_nElements = 0;
    m_bPreserveWhitespace = true;
    if (xmlName != 0) {
        Load(xmlName);
    }
}

Markup::~Markup()
{
    Release();
}

bool Markup::IsValid() const
{
    return _pElements != 0;
}

void Markup::SetPreserveWhitespace(bool bPreserve)
{
    m_bPreserveWhitespace = bPreserve;
}

bool Markup::Load(const String& xmlName)
{
    MemoryBlock xmlBlock;

	_xmlName = xmlName;
   
    Release();

    if (!Resources::getInstance()->getBinary(xmlName, xmlBlock)) {
        return false;
    }

    String sXml = String::createStringFromData(xmlBlock.getData(), (int)xmlBlock.getSize());

    _xml = static_cast<LPTSTR>(fn_memalloc((sXml.length() + 1) * sizeof(wchar_t)));
    __movsb((uint8_t*)_xml, (const uint8_t*)sXml.toWideCharPointer(), (sXml.length() + 1) * sizeof(wchar_t));

    bool bRes = _Parse();
    if (!bRes) {
        Release();
    }

    return bRes;
}

void Markup::Release()
{
    if (_xml != 0) {
        fn_memfree(_xml);
    }
    
    if (_pElements != 0) {
        fn_memfree(_pElements);
    }

    _xml = 0;
    _pElements = 0;
    m_nElements = 0;
}

void Markup::GetLastErrorMessage(LPTSTR pstrMessage, SIZE_T cchMax) const
{
    fn_lstrcpynW(pstrMessage, m_szErrorMsg, cchMax);
}

void Markup::GetLastErrorLocation(LPTSTR pstrSource, SIZE_T cchMax) const
{
    fn_lstrcpynW(pstrSource, m_szErrorXML, cchMax);
}

MarkupNode Markup::GetRoot()
{
    if( m_nElements == 0 ) return MarkupNode();
    return MarkupNode(this, 1);
}

bool Markup::_Parse()
{
    _ReserveElement(); // Reserve index 0 for errors
    __stosb((uint8_t*)m_szErrorMsg, 0, sizeof(m_szErrorMsg));
    __stosb((uint8_t*)m_szErrorXML, 0, sizeof(m_szErrorXML));
    _pstrXML = _xml;
    return _Parse(0);
}

bool Markup::_Parse(ULONG iParent)
{
    _SkipWhitespace();
    ULONG iPrevious = 0;
    for ( ; ; ) {
        if (*_pstrXML == L'\0' && iParent <= 1) {
            return true;
        }
        _SkipWhitespace();
        if (*_pstrXML != L'<') {
            return _Failed(L"Expected start tag", _pstrXML);
        }
        if (_pstrXML[1] == L'/') {
            return true;
        }
        *(_pstrXML++) = L'\0';
        _SkipWhitespace();
        // Skip comment or processing directive
        if (*_pstrXML == L'!' || *_pstrXML == L'?') {
            wchar_t ch = *_pstrXML;
            if (*_pstrXML == L'!') {
                ch = L'-';
            }
            while (*_pstrXML != L'\0' && !(*_pstrXML == ch && *(_pstrXML + 1) == L'>')) {
                _pstrXML = fn_CharNextW(_pstrXML);
            }
            if (*_pstrXML != L'\0') {
                _pstrXML += 2;
            }
            _SkipWhitespace();
            continue;
        }
        _SkipWhitespace();
        // Fill out element structure
        XMLELEMENT* pEl = _ReserveElement();
        ULONG iPos = pEl - _pElements;
        pEl->iStart = _pstrXML - _xml;
        pEl->iParent = iParent;
        pEl->iNext = pEl->iChild = 0;
        if (iPrevious != 0) {
            _pElements[iPrevious].iNext = iPos;
        }
        else if (iParent > 0) {
            _pElements[iParent].iChild = iPos;
        }
        iPrevious = iPos;
        // Parse name
        LPCTSTR pstrName = _pstrXML;
        _SkipIdentifier();
        LPTSTR pstrNameEnd = _pstrXML;
        if (*_pstrXML == L'\0') {
            return _Failed(L"Error parsing element name", _pstrXML);
        }
        // Parse attributes
        if (!_ParseAttributes()) {
            return false;
        }
        _SkipWhitespace();
        if (_pstrXML[0] == L'/' && _pstrXML[1] == L'>') {
            pEl->iData = _pstrXML - _xml;
            *_pstrXML = L'\0';
            _pstrXML += 2;
        }
        else {
            if (*_pstrXML != L'>') {
                return _Failed(L"Expected start-tag closing", _pstrXML);
            }
            // Parse node data
            pEl->iData = ++_pstrXML - _xml;
            _pstrDest = _pstrXML;
            if (!_ParseData(L'<')) {
                return false;
            }

            // Determine type of next element
            if (*_pstrXML == L'\0' && iParent <= 1) {
                return true;
            }
            if (*_pstrXML != L'<') {
                return _Failed(L"Expected end-tag start", _pstrXML);
            }
            if (_pstrXML[0] == L'<' && _pstrXML[1] != L'/') {
                if (!_Parse(iPos)) {
                    return false;
                }
            }
            if (_pstrXML[0] == L'<' && _pstrXML[1] == L'/') {
                *_pstrDest = L'\0';
                *_pstrXML = L'\0';
                _pstrXML += 2;
                _SkipWhitespace();
                SIZE_T cchName = pstrNameEnd - pstrName;
                if (fn_StrCmpNW(_pstrXML, pstrName, cchName) != 0) {
                    return _Failed(L"Unmatched closing tag", _pstrXML);
                }
                _pstrXML += cchName;
                _SkipWhitespace();
                if (*(_pstrXML++) != L'>') {
                    return _Failed(L"Unmatched closing tag", _pstrXML);
                }
            }
        }
        *pstrNameEnd = L'\0';
        _SkipWhitespace();
    }
}

Markup::XMLELEMENT* Markup::_ReserveElement()
{
    if (m_nElements == 0) {
        m_nReservedElements = 0;
    }
    if (m_nElements >= m_nReservedElements) {
        m_nReservedElements += (m_nReservedElements / 2) + 500;
        
        _pElements = static_cast<XMLELEMENT*>(fn_memrealloc(_pElements, m_nReservedElements * sizeof(XMLELEMENT)));
    }

    return &_pElements[m_nElements++];
}

void Markup::_SkipWhitespace()
{
    while (*_pstrXML > L'\0' && *_pstrXML <= L' ') {
        _pstrXML = fn_CharNextW(_pstrXML);
    }
}

LPCTSTR Markup::_SkipWhitespace(LPCTSTR pStr)
{
    while (*pStr > L'\0' && *pStr <= L' ') {
        pStr = fn_CharNextW(pStr);
    }

    return pStr;
}

void Markup::_SkipIdentifier()
{
    while (*_pstrXML != L'\0' && (*_pstrXML == L'_' || *_pstrXML == L':' || isalnum(*_pstrXML))) {
        _pstrXML = fn_CharNextW(_pstrXML);
    }
}

bool Markup::_ParseAttributes()
{   
    if (*_pstrXML == L'>') {
        return true;
    }
    *(_pstrXML++) = L'\0';
    _SkipWhitespace();
    while (*_pstrXML != L'\0' && *_pstrXML != L'>' && *_pstrXML != L'/') {
        _SkipIdentifier();
        LPTSTR pstrIdentifierEnd = _pstrXML;
        _SkipWhitespace();
        if (*_pstrXML != L'=') {
            return _Failed(L"Error while parsing attributes", _pstrXML);
        }
        *(_pstrXML++) = L' ';
        *pstrIdentifierEnd = L'\0';
        _SkipWhitespace();
        if (*(_pstrXML++) != L'\"') {
            return _Failed(L"Expected attribute value", _pstrXML);
        }
        _pstrDest = _pstrXML;
        if (!_ParseData(L'\"')) {
            return false;
        }

        if (*_pstrXML == L'\0') {
            return _Failed(L"Error while parsing attribute string", _pstrXML);
        }
        *_pstrDest = L'\0';
        if (_pstrXML != _pstrDest) {
            *_pstrXML = L' ';
        }
        _pstrXML++;
        _SkipWhitespace();
    }
    return true;
}

bool Markup::_ParseData(wchar_t cEnd)
{
    while (*_pstrXML != L'\0' && *_pstrXML != cEnd) {
		if (*_pstrXML == L'&') {
			while (*_pstrXML == L'&') {
                if (_pstrXML[1] == L'a' && _pstrXML[2] == L'm' && _pstrXML[3] == L'p' && _pstrXML[4] == L';') {
                    *(_pstrDest++) = L'&';
                    _pstrXML += 5;
                }
                else if (_pstrXML[1] == L'l' && _pstrXML[2] == L't' && _pstrXML[3] == L';') {
                    *(_pstrDest++) = L'<';
                    _pstrXML += 4;
                }
                else if (_pstrXML[1] == L'g' && _pstrXML[2] == L't' && _pstrXML[3] == L';') {
                    *(_pstrDest++) = L'>';
                    _pstrXML += 4;
                }
                else if (_pstrXML[1] == L'q' && _pstrXML[2] == L'u' && _pstrXML[3] == L'o' && _pstrXML[4] == L't' && _pstrXML[5] == L';') {
                    *(_pstrDest++) = L'"';
                    _pstrXML += 6;
                }
                else if (_pstrXML[1] == L'a' && _pstrXML[2] == L'p' && _pstrXML[3] == L'o' && _pstrXML[4] == L's' && _pstrXML[5] == L';') {
                    *(_pstrDest++) = L'\'';
                    _pstrXML += 6;
                }
                else {
                    *(_pstrDest++) = L'&';
                }
			}
            if (*_pstrXML == cEnd) {
				break;
            }
		}

        if (*_pstrXML == L' ') {
            *(_pstrDest++) = *(_pstrXML++);
            if (!m_bPreserveWhitespace) {
                _SkipWhitespace();
            }
        }
        else {
            LPTSTR pstrTemp = fn_CharNextW(_pstrXML);
            while (_pstrXML < pstrTemp) {
                *(_pstrDest++) = *(_pstrXML++);
            }
        }
    }
    // Make sure that MapAttributes() works correctly when it parses
    // over a value that has been transformed.
    LPTSTR pstrFill = _pstrDest + 1;
    while (pstrFill < _pstrXML) {
        *(pstrFill++) = L' ';
    }
    return true;
}

bool Markup::_Failed(LPCTSTR pstrError, LPCTSTR pstrLocation)
{
    // Register last error
    TRACE(L"XML Error: %s", pstrError);
    if (pstrLocation != 0) {
        TRACE(pstrLocation);
    }
    fn_lstrcpynW(m_szErrorMsg, pstrError, (sizeof(m_szErrorMsg) / sizeof(m_szErrorMsg[0])) - 1);
    fn_lstrcpynW(m_szErrorXML, pstrLocation != 0 ? pstrLocation : L"", lengthof(m_szErrorXML) - 1);
    return false; // Always return 'false'
}

} // namespace zgui
