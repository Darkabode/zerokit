#include "zgui.h"

#ifndef TRACE
#define TRACE
#endif

namespace zgui {

CMarkupNode::CMarkupNode() :
m_pOwner(NULL)
{
}

CMarkupNode::CMarkupNode(CMarkup* pOwner, int iPos) :
m_pOwner(pOwner),
m_iPos(iPos),
m_nAttributes(0)
{
}

CMarkupNode CMarkupNode::GetSibling()
{
    if( m_pOwner == NULL ) return CMarkupNode();
    ULONG iPos = m_pOwner->_pElements[m_iPos].iNext;
    if( iPos == 0 ) return CMarkupNode();
    return CMarkupNode(m_pOwner, iPos);
}

bool CMarkupNode::HasSiblings() const
{
    if( m_pOwner == NULL ) return false;
    ULONG iPos = m_pOwner->_pElements[m_iPos].iNext;
    return iPos > 0;
}

CMarkupNode CMarkupNode::GetChild()
{
    if( m_pOwner == NULL ) return CMarkupNode();
    ULONG iPos = m_pOwner->_pElements[m_iPos].iChild;
    if( iPos == 0 ) return CMarkupNode();
    return CMarkupNode(m_pOwner, iPos);
}

CMarkupNode CMarkupNode::GetChild(LPCTSTR pstrName)
{
    if (m_pOwner == NULL) {
        return CMarkupNode();
    }

    ULONG iPos = m_pOwner->_pElements[m_iPos].iChild;
    while (iPos != 0) {
        if (lstrcmp(m_pOwner->_xml + m_pOwner->_pElements[iPos].iStart, pstrName) == 0) {
            return CMarkupNode(m_pOwner, iPos);
        }
        iPos = m_pOwner->_pElements[iPos].iNext;
    }
    return CMarkupNode();
}

bool CMarkupNode::HasChildren() const
{
    if( m_pOwner == NULL ) return false;
    return m_pOwner->_pElements[m_iPos].iChild != 0;
}

CMarkupNode CMarkupNode::GetParent()
{
    if( m_pOwner == NULL ) return CMarkupNode();
    ULONG iPos = m_pOwner->_pElements[m_iPos].iParent;
    if( iPos == 0 ) return CMarkupNode();
    return CMarkupNode(m_pOwner, iPos);
}

bool CMarkupNode::IsValid() const
{
    return m_pOwner != NULL;
}

LPCTSTR CMarkupNode::GetName() const
{
    if( m_pOwner == NULL ) return NULL;
    return m_pOwner->_xml + m_pOwner->_pElements[m_iPos].iStart;
}

LPCTSTR CMarkupNode::GetValue() const
{
    if( m_pOwner == NULL ) return NULL;
    return m_pOwner->_xml + m_pOwner->_pElements[m_iPos].iData;
}

LPCTSTR CMarkupNode::GetAttributeName(int iIndex)
{
    if( m_pOwner == NULL ) return NULL;
    if( m_nAttributes == 0 ) _MapAttributes();
    if( iIndex < 0 || iIndex >= m_nAttributes ) return _T("");
    return m_pOwner->_xml + m_aAttributes[iIndex].iName;
}

LPCTSTR CMarkupNode::GetAttributeValue(int iIndex)
{
    if( m_pOwner == NULL ) return NULL;
    if( m_nAttributes == 0 ) _MapAttributes();
    if( iIndex < 0 || iIndex >= m_nAttributes ) return _T("");
    return m_pOwner->_xml + m_aAttributes[iIndex].iValue;
}

LPCTSTR CMarkupNode::GetAttributeValue(LPCTSTR pstrName)
{
    if( m_pOwner == NULL ) return NULL;
    if( m_nAttributes == 0 ) _MapAttributes();
    for( int i = 0; i < m_nAttributes; i++ ) {
        if( lstrcmp(m_pOwner->_xml + m_aAttributes[i].iName, pstrName) == 0 ) return m_pOwner->_xml + m_aAttributes[i].iValue;
    }
    return _T("");
}

bool CMarkupNode::GetAttributeValue(int iIndex, LPTSTR pstrValue, SIZE_T cchMax)
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
    
    _tcsncpy(pstrValue, m_pOwner->_xml + m_aAttributes[iIndex].iValue, cchMax);
    return true;
}

bool CMarkupNode::GetAttributeValue(LPCTSTR pstrName, LPTSTR pstrValue, SIZE_T cchMax)
{
    if( m_pOwner == NULL ) return false;
    if( m_nAttributes == 0 ) _MapAttributes();
    for( int i = 0; i < m_nAttributes; i++ ) {
        if( lstrcmp(m_pOwner->_xml + m_aAttributes[i].iName, pstrName) == 0 ) {
            _tcsncpy(pstrValue, m_pOwner->_xml + m_aAttributes[i].iValue, cchMax);
            return true;
        }
    }
    return false;
}

int CMarkupNode::GetAttributeCount()
{
    if( m_pOwner == NULL ) return 0;
    if( m_nAttributes == 0 ) _MapAttributes();
    return m_nAttributes;
}

bool CMarkupNode::HasAttributes()
{
    if( m_pOwner == NULL ) return false;
    if( m_nAttributes == 0 ) _MapAttributes();
    return m_nAttributes > 0;
}

bool CMarkupNode::HasAttribute(LPCTSTR pstrName)
{
    if( m_pOwner == NULL ) return false;
    if( m_nAttributes == 0 ) _MapAttributes();
    for( int i = 0; i < m_nAttributes; i++ ) {
        if( lstrcmp(m_pOwner->_xml + m_aAttributes[i].iName, pstrName) == 0 ) return true;
    }
    return false;
}

void CMarkupNode::_MapAttributes()
{
    m_nAttributes = 0;
    LPCTSTR pstr = m_pOwner->_xml + m_pOwner->_pElements[m_iPos].iStart;
    LPCTSTR pstrEnd = m_pOwner->_xml + m_pOwner->_pElements[m_iPos].iData;
    pstr += lstrlen(pstr) + 1;
    while (pstr < pstrEnd) {
        pstr = CMarkup::_SkipWhitespace(pstr);
        m_aAttributes[m_nAttributes].iName = pstr - m_pOwner->_xml;
        pstr += lstrlen(pstr) + 1;
        pstr = CMarkup::_SkipWhitespace(pstr);
        if (*pstr++ != _T('\"')) {
            return; // if( *pstr != _T('\"') ) { pstr = ::CharNext(pstr); return; }
        }
        
        m_aAttributes[m_nAttributes++].iValue = pstr - m_pOwner->_xml;
        if (m_nAttributes >= MAX_XML_ATTRIBUTES) {
            return;
        }
        pstr += lstrlen(pstr) + 1;
    }
}


///////////////////////////////////////////////////////////////////////////////////////
//
//
//

CMarkup::CMarkup(const char* xmlName)
{
    _xml = NULL;
    _pElements = NULL;
    m_nElements = 0;
    m_bPreserveWhitespace = true;
    if (xmlName != 0) {
        Load(xmlName);
    }
}

CMarkup::~CMarkup()
{
    Release();
}

bool CMarkup::IsValid() const
{
    return _pElements != NULL;
}

void CMarkup::SetPreserveWhitespace(bool bPreserve)
{
    m_bPreserveWhitespace = bPreserve;
}

bool CMarkup::Load(const char* xmlName)
{
    MemoryBlock xmlBlock;
    int off = 0;
    
    Release();

    if (!Resources::getInstance()->getBinary(xmlName, xmlBlock)) {
        return false;
    }

    String sXml = String::createStringFromData(xmlBlock.getData(), (int)xmlBlock.getSize());

    _xml = static_cast<LPTSTR>(memalloc((sXml.length() + 1) * sizeof(wchar_t)));
    memcpy((unsigned char*)_xml, (const unsigned char*)sXml.toWideCharPointer(), (sXml.length() + 1) * sizeof(wchar_t));

    bool bRes = _Parse();
    if (!bRes) {
        Release();
    }

    return bRes;
}

void CMarkup::Release()
{
    if (_xml != 0) {
        memfree(_xml);
    }
    
    if (_pElements != 0) {
        memfree(_pElements);
    }

    _xml = 0;
    _pElements = 0;
    m_nElements;
}

void CMarkup::GetLastErrorMessage(LPTSTR pstrMessage, SIZE_T cchMax) const
{
    _tcsncpy(pstrMessage, m_szErrorMsg, cchMax);
}

void CMarkup::GetLastErrorLocation(LPTSTR pstrSource, SIZE_T cchMax) const
{
    _tcsncpy(pstrSource, m_szErrorXML, cchMax);
}

CMarkupNode CMarkup::GetRoot()
{
    if( m_nElements == 0 ) return CMarkupNode();
    return CMarkupNode(this, 1);
}

bool CMarkup::_Parse()
{
    _ReserveElement(); // Reserve index 0 for errors
    ::ZeroMemory(m_szErrorMsg, sizeof(m_szErrorMsg));
    ::ZeroMemory(m_szErrorXML, sizeof(m_szErrorXML));
    _pstrXML = _xml;
    return _Parse(0);
}

bool CMarkup::_Parse(ULONG iParent)
{
    _SkipWhitespace();
    ULONG iPrevious = 0;
    for ( ; ; ) {
        if (*_pstrXML == _T('\0') && iParent <= 1) {
            return true;
        }
        _SkipWhitespace();
        if (*_pstrXML != _T('<')) {
            return _Failed(_T("Expected start tag"), _pstrXML);
        }
        if (_pstrXML[1] == _T('/')) {
            return true;
        }
        *(_pstrXML++) = _T('\0');
        _SkipWhitespace();
        // Skip comment or processing directive
        if (*_pstrXML == _T('!') || *_pstrXML == _T('?')) {
            TCHAR ch = *_pstrXML;
            if (*_pstrXML == _T('!')) {
                ch = _T('-');
            }
            while (*_pstrXML != _T('\0') && !(*_pstrXML == ch && *(_pstrXML + 1) == _T('>'))) {
                _pstrXML = ::CharNext(_pstrXML);
            }
            if (*_pstrXML != _T('\0')) {
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
        if (*_pstrXML == _T('\0')) {
            return _Failed(_T("Error parsing element name"), _pstrXML);
        }
        // Parse attributes
        if (!_ParseAttributes()) {
            return false;
        }
        _SkipWhitespace();
        if (_pstrXML[0] == _T('/') && _pstrXML[1] == _T('>')) {
            pEl->iData = _pstrXML - _xml;
            *_pstrXML = _T('\0');
            _pstrXML += 2;
        }
        else {
            if (*_pstrXML != _T('>')) {
                return _Failed(_T("Expected start-tag closing"), _pstrXML);
            }
            // Parse node data
            pEl->iData = ++_pstrXML - _xml;
            _pstrDest = _pstrXML;
            if (!_ParseData(_T('<'))) {
                return false;
            }

            // Determine type of next element
            if (*_pstrXML == _T('\0') && iParent <= 1) {
                return true;
            }
            if (*_pstrXML != _T('<')) {
                return _Failed(_T("Expected end-tag start"), _pstrXML);
            }
            if (_pstrXML[0] == _T('<') && _pstrXML[1] != _T('/')) {
                if (!_Parse(iPos)) {
                    return false;
                }
            }
            if (_pstrXML[0] == _T('<') && _pstrXML[1] == _T('/')) {
                *_pstrDest = _T('\0');
                *_pstrXML = _T('\0');
                _pstrXML += 2;
                _SkipWhitespace();
                SIZE_T cchName = pstrNameEnd - pstrName;
                if (_tcsncmp(_pstrXML, pstrName, cchName) != 0) {
                    return _Failed(_T("Unmatched closing tag"), _pstrXML);
                }
                _pstrXML += cchName;
                _SkipWhitespace();
                if (*(_pstrXML++) != _T('>')) {
                    return _Failed(_T("Unmatched closing tag"), _pstrXML);
                }
            }
        }
        *pstrNameEnd = _T('\0');
        _SkipWhitespace();
    }
}

CMarkup::XMLELEMENT* CMarkup::_ReserveElement()
{
    if (m_nElements == 0) {
        m_nReservedElements = 0;
    }
    if (m_nElements >= m_nReservedElements) {
        m_nReservedElements += (m_nReservedElements / 2) + 500;
        
        _pElements = static_cast<XMLELEMENT*>(cvec_reallocate(_pElements, m_nReservedElements * sizeof(XMLELEMENT)));
    }

    return &_pElements[m_nElements++];
}

void CMarkup::_SkipWhitespace()
{
    while (*_pstrXML > _T('\0') && *_pstrXML <= _T(' ')) {
        _pstrXML = ::CharNext(_pstrXML);
    }
}

LPCTSTR CMarkup::_SkipWhitespace(LPCTSTR pStr)
{
    while (*pStr > _T('\0') && *pStr <= _T(' ')) {
        pStr = ::CharNext(pStr);
    }

    return pStr;
}

void CMarkup::_SkipIdentifier()
{
    while (*_pstrXML != _T('\0') && (*_pstrXML == _T('_') || *_pstrXML == _T(':') || isalnum(*_pstrXML))) {
        _pstrXML = ::CharNext(_pstrXML);
    }
}

bool CMarkup::_ParseAttributes()
{   
    if (*_pstrXML == _T('>')) {
        return true;
    }
    *(_pstrXML++) = _T('\0');
    _SkipWhitespace();
    while (*_pstrXML != _T('\0') && *_pstrXML != _T('>') && *_pstrXML != _T('/')) {
        _SkipIdentifier();
        LPTSTR pstrIdentifierEnd = _pstrXML;
        _SkipWhitespace();
        if (*_pstrXML != _T('=')) {
            return _Failed(_T("Error while parsing attributes"), _pstrXML);
        }
        *(_pstrXML++) = _T(' ');
        *pstrIdentifierEnd = _T('\0');
        _SkipWhitespace();
        if (*(_pstrXML++) != _T('\"')) {
            return _Failed(_T("Expected attribute value"), _pstrXML);
        }
        _pstrDest = _pstrXML;
        if (!_ParseData(_T('\"'))) {
            return false;
        }

        if (*_pstrXML == _T('\0')) {
            return _Failed(_T("Error while parsing attribute string"), _pstrXML);
        }
        *_pstrDest = _T('\0');
        if (_pstrXML != _pstrDest) {
            *_pstrXML = _T(' ');
        }
        _pstrXML++;
        _SkipWhitespace();
    }
    return true;
}

bool CMarkup::_ParseData(TCHAR cEnd)
{
    while (*_pstrXML != _T('\0') && *_pstrXML != cEnd) {
		if (*_pstrXML == _T('&')) {
			while (*_pstrXML == _T('&')) {
                if (_pstrXML[1] == _T('a') && _pstrXML[2] == _T('m') && _pstrXML[3] == _T('p') && _pstrXML[4] == _T(';')) {
                    *(_pstrDest++) = _T('&');
                    _pstrXML += 5;
                }
                else if (_pstrXML[1] == _T('l') && _pstrXML[2] == _T('t') && _pstrXML[3] == _T(';')) {
                    *(_pstrDest++) = _T('<');
                    _pstrXML += 4;
                }
                else if (_pstrXML[1] == _T('g') && _pstrXML[2] == _T('t') && _pstrXML[3] == _T(';')) {
                    *(_pstrDest++) = _T('>');
                    _pstrXML += 4;
                }
                else if (_pstrXML[1] == _T('q') && _pstrXML[2] == _T('u') && _pstrXML[3] == _T('o') && _pstrXML[4] == _T('t') && _pstrXML[5] == _T(';')) {
                    *(_pstrDest++) = _T('"');
                    _pstrXML += 6;
                }
                else if (_pstrXML[1] == _T('a') && _pstrXML[2] == _T('p') && _pstrXML[3] == _T('o') && _pstrXML[4] == _T('s') && _pstrXML[5] == _T(';')) {
                    *(_pstrDest++) = _T('\'');
                    _pstrXML += 6;
                }
                else {
                    *(_pstrDest++) = _T('&');
                }
			}
            if (*_pstrXML == cEnd) {
				break;
            }
		}

        if (*_pstrXML == _T(' ')) {
            *(_pstrDest++) = *(_pstrXML++);
            if (!m_bPreserveWhitespace) {
                _SkipWhitespace();
            }
        }
        else {
            LPTSTR pstrTemp = ::CharNext(_pstrXML);
            while (_pstrXML < pstrTemp) {
                *(_pstrDest++) = *(_pstrXML++);
            }
        }
    }
    // Make sure that MapAttributes() works correctly when it parses
    // over a value that has been transformed.
    LPTSTR pstrFill = _pstrDest + 1;
    while (pstrFill < _pstrXML) {
        *(pstrFill++) = _T(' ');
    }
    return true;
}

bool CMarkup::_Failed(LPCTSTR pstrError, LPCTSTR pstrLocation)
{
    // Register last error
    TRACE(_T("XML Error: %s"), pstrError);
    if (pstrLocation != NULL) {
        TRACE(pstrLocation);
    }
    _tcsncpy(m_szErrorMsg, pstrError, (sizeof(m_szErrorMsg) / sizeof(m_szErrorMsg[0])) - 1);
    _tcsncpy(m_szErrorXML, pstrLocation != NULL ? pstrLocation : _T(""), lengthof(m_szErrorXML) - 1);
    return false; // Always return 'false'
}

} // namespace zgui
