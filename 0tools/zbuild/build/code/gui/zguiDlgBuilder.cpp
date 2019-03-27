#include "zgui.h"

namespace zgui {

GuiBuilder::GuiBuilder() : m_pCallback(NULL), m_pstrtype(NULL)
{

}

CControlUI* GuiBuilder::Create(const String& xml, LPCTSTR type, IDialogBuilderCallback* pCallback, CPaintManagerUI* pManager, CControlUI* pParent)
{
    if (!m_xml.Load(xml.toUTF8())) {
        return NULL;
    }

    return Create(pCallback, pManager, pParent);
}

CControlUI* GuiBuilder::Create(IDialogBuilderCallback* pCallback, CPaintManagerUI* pManager, CControlUI* pParent)
{
    CMarkupNode root = m_xml.GetRoot();

    if (!root.IsValid()) {
        return NULL;
    }

    m_pCallback = pCallback;

    if (pManager != 0) {
        LPCTSTR pstrClass = NULL;
        int nAttributes = 0;
        LPCTSTR pstrName = NULL;
        LPCTSTR pstrValue = NULL;
        LPTSTR pstr = NULL;
        for (CMarkupNode node = root.GetChild() ; node.IsValid(); node = node.GetSibling()) {
            pstrClass = node.GetName();
            if (lstrcmp(pstrClass, _T("Image")) == 0 ) {
                nAttributes = node.GetAttributeCount();
                LPCTSTR pImageName = NULL;
/*                LPCTSTR pImageResType = NULL;*/
                DWORD mask = 0;
                for( int i = 0; i < nAttributes; i++ ) {
                    pstrName = node.GetAttributeName(i);
                    pstrValue = node.GetAttributeValue(i);
                    if (lstrcmp(pstrName, _T("name")) == 0 ) {
                        pImageName = pstrValue;
                    }
//                     else if (lstrcmp(pstrName, _T("restype")) == 0 ) {
//                         pImageResType = pstrValue;
//                     }
                    else if (lstrcmp(pstrName, _T("mask")) == 0 ) {
                        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
                        mask = _tcstoul(pstrValue, &pstr, 16);
                    }
                }
                if (pImageName) {
                    pManager->AddImage(pImageName, /*pImageResType, */mask);
                }
            }
            else if (lstrcmp(pstrClass, _T("Font")) == 0) {
                nAttributes = node.GetAttributeCount();
                LPCTSTR pFontName = NULL;
                int size = 12;
                bool bold = false;
                bool underline = false;
                bool italic = false;
                bool defaultfont = false;
                for (int i = 0; i < nAttributes; ++i) {
                    pstrName = node.GetAttributeName(i);
                    pstrValue = node.GetAttributeValue(i);
                    if (lstrcmp(pstrName, _T("name")) == 0 ) {
                        pFontName = pstrValue;
                    }
                    else if (lstrcmp(pstrName, _T("size")) == 0) {
                        size = _tcstol(pstrValue, &pstr, 10);
                    }
                    else if (lstrcmp(pstrName, _T("bold")) == 0) {
                        bold = (lstrcmp(pstrValue, _T("true")) == 0);
                    }
                    else if (lstrcmp(pstrName, _T("underline")) == 0) {
                        underline = (lstrcmp(pstrValue, _T("true")) == 0);
                    }
                    else if (lstrcmp(pstrName, _T("italic")) == 0) {
                        italic = (lstrcmp(pstrValue, _T("true")) == 0);
                    }
                    else if (lstrcmp(pstrName, _T("default")) == 0) {
                        defaultfont = (lstrcmp(pstrValue, _T("true")) == 0);
                    } 
                }
                if (pFontName) {
                    pManager->AddFont(pFontName, size, bold, underline, italic);
                    if (defaultfont) {
                        pManager->SetDefaultFont(pFontName, size, bold, underline, italic);
                    }
                } 
            } 
            else if (lstrcmp(pstrClass, _T("Default")) == 0) {
                nAttributes = node.GetAttributeCount(); 
                LPCTSTR pControlName = 0; 
                LPCTSTR pControlValue = 0; 

                for (int i = 0; i < nAttributes; ++i) { 
                    pstrName = node.GetAttributeName(i);
                    pstrValue = node.GetAttributeValue(i);
                    if (lstrcmp(pstrName, _T("name")) == 0) {
                        pControlName = pstrValue;
                    }
                    else if (lstrcmp(pstrName, _T("value")) == 0) {
                        pControlValue = pstrValue;
                    }
                }
                if (pControlName) {
                    pManager->AddDefaultAttributeList(pControlName, pControlValue);
                }
            }
        }

        pstrClass = root.GetName();
        if (lstrcmp(pstrClass, _T("Window")) == 0) {
            if( pManager->GetPaintWindow() ) {
                int nAttributes = root.GetAttributeCount();
                for (int i = 0; i < nAttributes; ++i) {
                    pstrName = root.GetAttributeName(i);
                    pstrValue = root.GetAttributeValue(i);
                    if (lstrcmp(pstrName, _T("size")) == 0) {
                        LPTSTR pstr = NULL;
                        LPRECT pMonArea = pManager->getMonitorArea();

                        int cx = _tcstol(pstrValue, &pstr, 10); ASSERT(pstr);
                        if (cx < 0) {
                            cx *= -1;
                            cx = (cx * (pMonArea->right - pMonArea->left)) / 100;
                        }
                        int cy = _tcstol(pstr + 1, &pstr, 10); ASSERT(pstr); 
                        if (cy < 0) {
                            cy *= -1;
                            cy = (cy * (pMonArea->bottom - pMonArea->top)) / 100;
                        }
                        pManager->SetInitSize(cx, cy);
                    } 
                    else if( lstrcmp(pstrName, _T("sizebox")) == 0 ) {
                        RECT rcSizeBox = { 0 };
                        LPTSTR pstr = NULL;
                        rcSizeBox.left = _tcstol(pstrValue, &pstr, 10);  ASSERT(pstr);    
                        rcSizeBox.top = _tcstol(pstr + 1, &pstr, 10);    ASSERT(pstr);    
                        rcSizeBox.right = _tcstol(pstr + 1, &pstr, 10);  ASSERT(pstr);    
                        rcSizeBox.bottom = _tcstol(pstr + 1, &pstr, 10); ASSERT(pstr);    
                        pManager->SetSizeBox(rcSizeBox);
                    }
                    else if( lstrcmp(pstrName, _T("caption")) == 0 ) {
                        RECT rcCaption = { 0 };
                        LPTSTR pstr = NULL;
                        rcCaption.left = _tcstol(pstrValue, &pstr, 10);  ASSERT(pstr);    
                        rcCaption.top = _tcstol(pstr + 1, &pstr, 10);    ASSERT(pstr);    
                        rcCaption.right = _tcstol(pstr + 1, &pstr, 10);  ASSERT(pstr);    
                        rcCaption.bottom = _tcstol(pstr + 1, &pstr, 10); ASSERT(pstr);    
                        pManager->SetCaptionRect(rcCaption);
                    }
                    else if( lstrcmp(pstrName, _T("roundcorner")) == 0 ) {
                        LPTSTR pstr = NULL;
                        int cx = _tcstol(pstrValue, &pstr, 10);  ASSERT(pstr);    
                        int cy = _tcstol(pstr + 1, &pstr, 10);    ASSERT(pstr); 
                        pManager->SetRoundCorner(cx, cy);
                    } 
                    else if( lstrcmp(pstrName, _T("mininfo")) == 0 ) {
                        LPTSTR pstr = NULL;
                        int cx = _tcstol(pstrValue, &pstr, 10);  ASSERT(pstr);    
                        int cy = _tcstol(pstr + 1, &pstr, 10);    ASSERT(pstr); 
                        pManager->SetMinInfo(cx, cy);
                    }
                    else if( lstrcmp(pstrName, _T("maxinfo")) == 0 ) {
                        LPTSTR pstr = NULL;
                        int cx = _tcstol(pstrValue, &pstr, 10);  ASSERT(pstr);    
                        int cy = _tcstol(pstr + 1, &pstr, 10);    ASSERT(pstr); 
                        pManager->SetMaxInfo(cx, cy);
                    }
                    else if( lstrcmp(pstrName, _T("showdirty")) == 0 ) {
                        pManager->SetShowUpdateRect(lstrcmp(pstrValue, _T("true")) == 0);
                    } 
                    else if( lstrcmp(pstrName, _T("alpha")) == 0 ) {
                        pManager->SetTransparent(_ttoi(pstrValue));
                    } 
                    else if( lstrcmp(pstrName, _T("bktrans")) == 0 ) {
                        pManager->SetBackgroundTransparent(lstrcmp(pstrValue, _T("true")) == 0);
                    } 
                    else if( lstrcmp(pstrName, _T("disabledfontcolor")) == 0 ) {
                        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
                        LPTSTR pstr = NULL;
                        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
                        pManager->SetDefaultDisabledColor(clrColor);
                    } 
                    else if( lstrcmp(pstrName, _T("defaultfontcolor")) == 0 ) {
                        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
                        LPTSTR pstr = NULL;
                        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
                        pManager->SetDefaultFontColor(clrColor);
                    }
                    else if( lstrcmp(pstrName, _T("linkfontcolor")) == 0 ) {
                        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
                        LPTSTR pstr = NULL;
                        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
                        pManager->SetDefaultLinkFontColor(clrColor);
                    } 
                    else if( lstrcmp(pstrName, _T("linkhoverfontcolor")) == 0 ) {
                        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
                        LPTSTR pstr = NULL;
                        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
                        pManager->SetDefaultLinkHoverFontColor(clrColor);
                    } 
                    else if( lstrcmp(pstrName, _T("selectedcolor")) == 0 ) {
                        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
                        LPTSTR pstr = NULL;
                        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
                        pManager->SetDefaultSelectedBkColor(clrColor);
                    } 
                }
            }
        }
    }
    return _Parse(&root, pParent, pManager);
}

CMarkup* GuiBuilder::GetMarkup()
{
    return &m_xml;
}

void GuiBuilder::GetLastErrorMessage(LPTSTR pstrMessage, SIZE_T cchMax) const
{
    return m_xml.GetLastErrorMessage(pstrMessage, cchMax);
}

void GuiBuilder::GetLastErrorLocation(LPTSTR pstrSource, SIZE_T cchMax) const
{
    return m_xml.GetLastErrorLocation(pstrSource, cchMax);
}

CControlUI* GuiBuilder::_Parse(CMarkupNode* pRoot, CControlUI* pParent, CPaintManagerUI* pManager)
{
    IContainerUI* pContainer = NULL;
    CControlUI* pReturn = NULL;
    for (CMarkupNode node = pRoot->GetChild() ; node.IsValid(); node = node.GetSibling()) {
        LPCTSTR pstrClass = node.GetName();
        if (lstrcmp(pstrClass, _T("Image")) == 0 || lstrcmp(pstrClass, _T("Font")) == 0 || lstrcmp(pstrClass, _T("Default")) == 0) {
            continue;
        }

        CControlUI* pControl = NULL;
        if (lstrcmp(pstrClass, _T("Include")) == 0) {
            if (!node.HasAttributes()) {
                continue;
            }
            int count = 1;
            LPTSTR pstr = NULL;
            TCHAR szValue[500] = { 0 };
            SIZE_T cchLen = lengthof(szValue) - 1;
            if (node.GetAttributeValue(_T("count"), szValue, cchLen)) {
                count = _tcstol(szValue, &pstr, 10);
            }
            cchLen = lengthof(szValue) - 1;
            if (!node.GetAttributeValue(_T("source"), szValue, cchLen)) {
                continue;
            }
            for (int i = 0; i < count; ++i) {
                GuiBuilder builder;
                if (m_pstrtype != NULL) { // 使用资源dll，从资源中读取
                    WORD id = (WORD)_tcstol(szValue, &pstr, 10); 
                    pControl = builder.Create(String(id), m_pstrtype, m_pCallback, pManager, pParent);
                }
                else {
                    pControl = builder.Create(String(szValue), (UINT)0, m_pCallback, pManager, pParent);
                }
            }
            continue;
        }
        else {
            SIZE_T cchLen = lstrlen(pstrClass);
            switch (cchLen) {
            case 4:
#ifdef ZGUI_USE_EDIT
                if (lstrcmp(pstrClass, DUI_CTR_EDIT) == 0) {
                    pControl = new CEditUI;
                }
                else
#endif // ZGUI_USE_EDIT
#ifdef ZGUI_USE_LIST
                if (lstrcmp(pstrClass, DUI_CTR_LIST) == 0) {
                    pControl = new CListUI;
                }
                else
#endif // ZGUI_USE_LIST    
#ifdef ZGUI_USE_TEXT
                if (lstrcmp(pstrClass, DUI_CTR_TEXT) == 0) {
                    pControl = new CTextUI;
                }
#endif // ZGUI_USE_TEXT
                break;
            case 5:
#ifdef ZGUI_USE_COMBO
                if (lstrcmp(pstrClass, DUI_CTR_COMBO) == 0) {
                    pControl = new CComboUI;
                }
                else
#endif // ZGUI_USE_COMBO    
#ifdef ZGUI_USE_LABEL
                if (lstrcmp(pstrClass, DUI_CTR_LABEL) == 0) {
                    pControl = new CLabelUI;
                }
#endif // ZGUI_USE_LABEL
//                 else if (lstrcmp(pstrClass, DUI_CTR_FLASH) == 0) {
//                     pControl = new CFlashUI;
//                 }
                break;
            case 6:
#ifdef ZGUI_USE_LABEL
#ifdef ZGUI_USE_BUTTON
                if (lstrcmp(pstrClass, DUI_CTR_BUTTON) == 0) {
                    pControl = new CButtonUI;
                }
                else
#endif // ZGUI_USE_BUTTON
#endif // ZGUI_USE_LABEL
#ifdef ZGUI_USE_OPTION
                if (lstrcmp(pstrClass, DUI_CTR_OPTION) == 0) {
                    pControl = new COptionUI;
                }
                else
#endif // ZGUI_USE_OPTION
#ifdef ZGUI_USE_SLIDER
                if (lstrcmp(pstrClass, DUI_CTR_SLIDER) == 0) {
                    pControl = new CSliderUI;
                }
                else
#endif // ZGUI_USE_SLIDER
                if (lstrcmp(pstrClass, _T("Camera")) == 0) {
                    pControl = new Camera;
                }
                break;
            case 7:
                if (lstrcmp(pstrClass, DUI_CTR_CONTROL) == 0) {
                    pControl = new CControlUI;
                }
                else
#ifdef ZGUI_USE_ACTIVEX
                if (lstrcmp(pstrClass, DUI_CTR_ACTIVEX) == 0) {
                    pControl = new CActiveXUI;
                }
#endif // ZGUI_USE_ACTIVEX
                break;
            case 8:
#ifdef ZGUI_USE_PROGRESS
                if (lstrcmp(pstrClass, DUI_CTR_PROGRESS) == 0) {
                    pControl = new CProgressUI;
                }
                else
#endif // ZGUI_USE_PROGRESS
#ifdef ZGUI_USE_RICHEDIT
                if (lstrcmp(pstrClass, DUI_CTR_RICHEDIT) == 0) {
                    pControl = new CRichEditUI;
                }
                else
#endif // ZGUI_USE_RICHEDIT
				// add by:zjie
#ifdef ZGUI_USE_OPTION
#ifdef ZGUI_USE_CHECKBOX
                if (lstrcmp(pstrClass, DUI_CTR_CHECKBOX) == 0) {
                    pControl = new CCheckBoxUI;
                }
                else
#endif // ZGUI_USE_CHECKBOX
#endif // ZGUI_USE_OPTION
#ifdef ZGUI_USE_COMBOBOX
                if (lstrcmp(pstrClass, DUI_CTR_COMBOBOX) == 0) {
                    pControl = new CComboBoxUI;
                }
                else
#endif // ZGUI_USE_COMBOBOX
#ifdef ZGUI_USE_DATETIME
                if (lstrcmp(pstrClass, DUI_CTR_DATETIME) == 0) {
                    pControl = new CDateTimeUI;
                }
#endif // ZGUI_USE_DATETIME
				// add by:zjie
                break;
            case 9:
                if (lstrcmp(pstrClass, DUI_CTR_CONTAINER) == 0) {
                    pControl = new CContainerUI;
                }
                else
#ifdef ZGUI_USE_TABLAYOUT
                if (lstrcmp(pstrClass, DUI_CTR_TABLAYOUT) == 0) {
                    pControl = new CTabLayoutUI;
                }
                else
#endif // ZGUI_USE_TABLAYOUT
#ifdef ZGUI_USE_SCROLLBAR
                if (lstrcmp(pstrClass, DUI_CTR_SCROLLBAR) == 0) {
                    pControl = new CScrollBarUI;
                }
#endif // ZGUI_USE_SCROLLBAR
                break;
            case 10:
#ifdef ZGUI_USE_LIST
                if (lstrcmp(pstrClass, DUI_CTR_LISTHEADER) == 0) {
                    pControl = new CListHeaderUI;
                }
                else
#endif // ZGUI_USE_LIST
#ifdef ZGUI_USE_TILELAYOUT
                if (lstrcmp(pstrClass, DUI_CTR_TILELAYOUT) == 0) {
                    pControl = new CTileLayoutUI;
                }
                else
#endif // ZGUI_USE_TILELAYOUT
#ifdef ZGUI_USE_WEBBROWSER
                if (lstrcmp(pstrClass, DUI_CTR_WEBBROWSER) == 0) {
                    pControl = new CWebBrowserUI;
                }
#endif // ZGUI_USE_WEBBROWSER
                break;
			case 11:
#ifdef ZGUI_USE_CHILDLAYOUT
                if (lstrcmp(pstrClass, DUI_CTR_CHILDLAYOUT) == 0) {
                    pControl=new CChildLayoutUI;
                }
#endif // ZGUI_USE_CHILDLAYOUT
				break;
            case 14:
                if (lstrcmp(pstrClass, DUI_CTR_VERTICALLAYOUT) == 0) {
                    pControl = new CVerticalLayoutUI;
                }
                else
#ifdef ZGUI_USE_LIST
                if (lstrcmp(pstrClass, DUI_CTR_LISTHEADERITEM) == 0) {
                    pControl = new CListHeaderItemUI;
                }
#endif // ZGUI_USE_LIST
                break;
            case 15:
#ifdef ZGUI_USE_LIST
                if (lstrcmp(pstrClass, DUI_CTR_LISTTEXTELEMENT) == 0) {
                    pControl = new CListTextElementUI;
                }
#endif // ZGUI_USE_LIST
                break;
            case 16:
                if (lstrcmp(pstrClass, DUI_CTR_HORIZONTALLAYOUT) == 0) {
                    pControl = new CHorizontalLayoutUI;
                }
                else
#ifdef ZGUI_USE_LIST
                if (lstrcmp(pstrClass, DUI_CTR_LISTLABELELEMENT) == 0) {
                    pControl = new CListLabelElementUI;
                }
#endif // ZGUI_USE_LIST
                break;
            case 20:
#ifdef ZGUI_USE_LIST
                if (lstrcmp(pstrClass, DUI_CTR_LISTCONTAINERELEMENT) == 0) {
                    pControl = new CListContainerElementUI;
                }
#endif // ZGUI_USE_LIST
                break;
            }
            // User-supplied control factory
            if (pControl == NULL) {
                CStdPtrArray* pPlugins = CPaintManagerUI::GetPlugins();
                LPCREATECONTROL lpCreateControl = NULL;
                for (int i = 0; i < pPlugins->GetSize(); ++i) {
                    lpCreateControl = (LPCREATECONTROL)pPlugins->GetAt(i);
                    if (lpCreateControl != NULL) {
                        pControl = lpCreateControl(pstrClass);
                        if (pControl != NULL) {
                            break;
                        }
                    }
                }
            }
            if (pControl == NULL && m_pCallback != NULL) {
                pControl = m_pCallback->CreateControl(pstrClass);
            }
        }

#ifndef _DEBUG
        ASSERT(pControl);
#endif // _DEBUG
		if (pControl == NULL) {
#ifdef _DEBUG
			TRACE(_T("Unknown control: %s"), pstrClass);
#else
			continue;
#endif
		}

        // Attach to parent
        // 因为某些属性和父窗口相关，比如selected，必须先Add到父窗口
        if (pParent != NULL) {
            if (pContainer == NULL) {
                pContainer = static_cast<IContainerUI*>(pParent->GetInterface(_T("IContainer")));
            }
            ASSERT(pContainer);
            if (pContainer == NULL) {
                return NULL;
            }
            if (!pContainer->Add(pControl) ) {
                delete pControl;
                continue;
            }
        }
        // Init default attributes
        if (pManager) {
            pControl->SetManager(pManager, pParent, false);
            LPCTSTR pDefaultAttributes = pManager->GetDefaultAttributeList(pstrClass);
            if (pDefaultAttributes) {
                pControl->ApplyAttributeList(pDefaultAttributes);
            }
        }
        // Process attributes
        if (node.HasAttributes()) {
            TCHAR szValue[500] = { 0 };
            SIZE_T cchLen = lengthof(szValue) - 1;
            // Set ordinary attributes
            int nAttributes = node.GetAttributeCount();
            for (int i = 0; i < nAttributes; ++i) {
                pControl->SetAttribute(node.GetAttributeName(i), node.GetAttributeValue(i));
            }
        }
        if (pManager != 0) {
            pControl->SetManager(0, 0, false);
        }

        // Add children
        if (node.HasChildren()) {
            _Parse(&node, pControl, pManager);
        }

        // Return first item
        if (pReturn == 0) {
            pReturn = pControl;
        }
    }
    return pReturn;
}

} // namespace zgui
