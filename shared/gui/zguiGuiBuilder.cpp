#include "zgui.h"

namespace zgui {

GuiBuilder::GuiBuilder() :
_pCallback(0)
{

}

Control* GuiBuilder::create(const String& xml, IDialogBuilderCallback* pCallback, PaintManager* pManager, Control* pParent)
{
	if (!_xml.IsValid()) {
		if (!_xml.Load(xml)) {
			return 0;
		}
	}
	else if (xml != _xml.getXmlName()) {
		if (!_xml.Load(xml)) {
			return 0;
		}
	}

    return create(pCallback, pManager, pParent);
}

Control* GuiBuilder::create(IDialogBuilderCallback* pCallback, PaintManager* pManager, Control* pParent)
{
    MarkupNode root = _xml.GetRoot();

    if (!root.IsValid()) {
        return 0;
    }

    _pCallback = pCallback;

    if (pManager != 0) {
        String strClass;
        int nAttributes = 0;
        String strName;
        String strValue;
        for (MarkupNode node = root.GetChild() ; node.IsValid(); node = node.GetSibling()) {
            strClass = node.GetName();
			nAttributes = node.GetAttributeCount();
            if (strClass =="Image") {
                String pImageName;
                for (int i = 0; i < nAttributes; ++i) {
                    strName = node.GetAttributeName(i);
                    strValue = node.GetAttributeValue(i);
                    if (strName == "name") {
                        pImageName = strValue;
                    }
                }
                if (!pImageName.isEmpty()) {
                    pManager->addImage(pImageName);
                }
            }
            else if (strClass == "Font") {
                String fontName;
                int size = 12;
                bool bold = false;
                bool underline = false;
                bool italic = false;
                bool defaultfont = false;
                for (int i = 0; i < nAttributes; ++i) {
                    strName = node.GetAttributeName(i);
                    strValue = node.GetAttributeValue(i);
                    if (strName == "name") {
                        fontName = strValue;
                    }
                    else if (strName == "size") {
                        size = strValue.getIntValue();
                    }
                    else if (strName == "bold") {
                        bold = (strValue == "true");
                    }
                    else if (strName == "underline") {
                        underline = (strValue == "true");
                    }
                    else if (strName == "italic") {
                        italic = (strValue == "true");
                    }
                    else if (strName == "default") {
                        defaultfont = (strValue == "true");
                    } 
                }
                if (!fontName.isEmpty()) {
                    pManager->AddFont(fontName, size, bold, underline, italic);
                    if (defaultfont) {
                        pManager->SetDefaultFont(fontName, size, bold, underline, italic);
                    }
                } 
            }
			else if (strClass == "Lang") {
				String langName;
				String langFileName;
				bool isDefault = false;
				for (int i = 0; i < nAttributes; ++i) {
					strName = node.GetAttributeName(i);
					strValue = node.GetAttributeValue(i);
					if (strName == "name") {
						langName = strValue;
					}
					else if (strName == "file") {
						langFileName = strValue;
					}
					else if (strName == "default") {
						isDefault = (strValue == "true");
					}
				}
				if (!langName.isEmpty() && !langFileName.isEmpty()) {
					Language::getInstance()->addLanguage(langName, langFileName, isDefault);
				}
			}
            else if (strClass == "Default") { 
                String controlName; 
                String controlValue; 

                for (int i = 0; i < nAttributes; ++i) { 
                    strName = node.GetAttributeName(i);
                    strValue = node.GetAttributeValue(i);
                    if (strName == "name") {
                        controlName = strValue;
                    }
                    else if (strName == "value") {
                        controlValue = strValue;
                    }
                }
                if (!controlName.isEmpty()) {
                    pManager->addDefaultAttributeList(controlName, controlValue);
                }
            }
			else if (strClass == "Style") {
				String styleName;
				StringPairArray params;

				for (int i = 0; i < nAttributes; ++i) {
					strName = node.GetAttributeName(i);
					strValue = node.GetAttributeValue(i);
					if (strName == "name") {
						styleName = strValue;
					}
					else {
						params.set(strName, strValue);
					}
				}

				if (params.size() != 0) {
					StringPairArray* pNewParams = new StringPairArray(params);
					pManager->addStyle(styleName, pNewParams);
				}
			}
        }

        strClass = root.GetName();
        if (strClass == "Window") {
            if (pManager->GetPaintWindow()) {
                int nAttributes = root.GetAttributeCount();
                for (int i = 0; i < nAttributes; ++i) {
                    strName = root.GetAttributeName(i);
                    strValue = root.GetAttributeValue(i);
                    if (strName == "size") {
                        int cx, cy;
                        LPRECT pMonArea = pManager->getMonitorArea();
                        if (Helper::splitString(strValue, ",", String::empty, cx, cy)) {
                            if (cx < 0) {
                                cx *= -1;
                                cx = (cx * (pMonArea->right - pMonArea->left)) / 100;
                            }
                            if (cy < 0) {
                                cy *= -1;
                                cy = (cy * (pMonArea->bottom - pMonArea->top)) / 100;
                            }
                            pManager->SetInitSize(cx, cy);
                        }
                    } 
                    else if (strName == "sizebox") {
                        RECT rcSizeBox;
                        if (Helper::splitString(strValue, ",", String::empty, (int&)rcSizeBox.left, (int&)rcSizeBox.top, (int&)rcSizeBox.right, (int&)rcSizeBox.bottom)) {
                            pManager->SetSizeBox(rcSizeBox);
                        }
                    }
                    else if (strName == "caption") {
                        RECT rcCaption;
                        if (Helper::splitString(strValue, ",", String::empty, (int&)rcCaption.left, (int&)rcCaption.top, (int&)rcCaption.right, (int&)rcCaption.bottom)) {
                            pManager->SetCaptionRect(rcCaption);
                        }
                    }
                    else if (strName == "roundcorner") {
                        int cx, cy;
                        if (Helper::splitString(strValue, ",", String::empty, cx, cy)) {
                            pManager->SetRoundCorner(cx, cy);
                        }
                    } 
                    else if (strName == "mininfo") {
                        int cx, cy;
                        if (Helper::splitString(strValue, ",", String::empty, cx, cy)) {
                            pManager->SetMinInfo(cx, cy);
                        }
                    }
                    else if (strName == "maxinfo") {
                        int cx, cy;
                        if (Helper::splitString(strValue, ",", String::empty, cx, cy)) {
                            pManager->SetMaxInfo(cx, cy);
                        }
                    }
                    else if (strName == "showdirty") {
                        pManager->SetShowUpdateRect(strValue == "true");
                    } 
                    else if (strName == "alpha") {
                        pManager->SetTransparent(strValue.getIntValue());
                    } 
                    else if (strName == "bktrans") {
                        pManager->SetBackgroundTransparent(strValue == "true");
                    } 
                    else if (strName == "disabledfontcolor") {
                        pManager->SetDefaultDisabledColor((uint32_t)strValue.getHexValue32());
                    } 
                    else if (strName == "defaultfontcolor") {
                        pManager->SetDefaultFontColor((uint32_t)strValue.getHexValue32());
                    }
                    else if (strName == "linkfontcolor") {
                        pManager->SetDefaultLinkFontColor((uint32_t)strValue.getHexValue32());
                    } 
                    else if (strName == "linkhoverfontcolor") {
                        pManager->SetDefaultLinkHoverFontColor((uint32_t)strValue.getHexValue32());
                    } 
                    else if (strName == "selectedcolor") {
                        pManager->SetDefaultSelectedBkColor((uint32_t)strValue.getHexValue32());
                    } 
                }
            }
        }
    }
    return parse(&root, pParent, pManager);
}

void GuiBuilder::getLastErrorMessage(LPTSTR pstrMessage, SIZE_T cchMax) const
{
    return _xml.GetLastErrorMessage(pstrMessage, cchMax);
}

void GuiBuilder::getLastErrorLocation(LPTSTR pstrSource, SIZE_T cchMax) const
{
    return _xml.GetLastErrorLocation(pstrSource, cchMax);
}

Control* GuiBuilder::parse(MarkupNode* pRoot, Control* pParent, PaintManager* pManager)
{
    IContainer* pContainer = 0;
    Control* pReturn = 0;
    for (MarkupNode node = pRoot->GetChild() ; node.IsValid(); node = node.GetSibling()) {
        String className = node.GetName();
		if (className == "Image" || className == "Font" || className == "Lang" || className == "Default" || className == "Style") {
            continue;
        }

        Control* pControl = 0;
        if (className == "Include") {
            if (!node.HasAttributes()) {
                continue;
            }
            int count = 1;
            String szValue;
            if (node.GetAttributeValue(L"count", szValue)) {
                count = szValue.getIntValue();
            }
            if (!node.GetAttributeValue(L"source", szValue)) {
                continue;
            }
            for (int i = 0; i < count; ++i) {
                GuiBuilder builder;
                pControl = builder.create(String(szValue), _pCallback, pManager, pParent);
            }
            continue;
        }
#ifdef ZGUI_USE_TREEVIEW
		else if (className == ZGUI_TREENODE) {
			TreeNode* pParentNode = static_cast<TreeNode*>(pParent->getInterface(className));
			TreeNode* pNode = new TreeNode();
			if (pParentNode != 0) {
				if (!pParentNode->Add(pNode)) {
					delete pNode;
					continue;
				}
			}

			if (pManager != 0) {
				pNode->SetManager(pManager, 0, false);
				const String& pDefaultAttributes = pManager->getDefaultAttributeList(className);
				if (!pDefaultAttributes.isEmpty()) {
					pNode->applyAttributeList(pDefaultAttributes);
				}
			}

			if (node.HasAttributes()) {
				// Set ordinary attributes
				int nAttributes = node.GetAttributeCount();
				for (int i = 0; i < nAttributes; i++) {
					pNode->setAttribute(node.GetAttributeName(i), node.GetAttributeValue(i));
				}
			}

			if (node.HasChildren()){
				Control* pSubControl = parse(&node, pNode, pManager);
				if (pSubControl && pSubControl->getClass() != ZGUI_TREENODE)
				{
					// 					pSubControl->SetFixedWidth(30);
					// 					HorizontalLayout* pHorz = pNode->GetTreeNodeHoriznotal();
					// 					pHorz->Add(new Edit());
					// 					continue;
				}
			}

			if (pParentNode == 0){
				TreeView* pTreeView = static_cast<TreeView*>(pParent->getInterface(ZGUI_TREEVIEW));
				zgui_assert(pTreeView);
				if (pTreeView == 0) {
					return 0;
				}
				if (!pTreeView->add(pNode)) {
					delete pNode;
					continue;
				}
			}
			continue;
		}
#endif // ZGUI_USE_TREEVIEW
        else {
            SIZE_T cchLen = className.length();
            switch (cchLen) {
                case 4:
#ifdef ZGUI_USE_EDIT
					if (className == ZGUI_EDIT) {
                        pControl = new Edit;
                    }
                    else
#endif // ZGUI_USE_EDIT
#ifdef ZGUI_USE_LIST
					if (className == ZGUI_LIST) {
                        pControl = new List;
                    }
                    else
#endif // ZGUI_USE_LIST    
#ifdef ZGUI_USE_TEXT
					if (className == ZGUI_TEXT) {
                        pControl = new Text;
                    }
#endif // ZGUI_USE_TEXT
                    break;
                case 5:
#ifdef ZGUI_USE_COMBO
					if (className == ZGUI_COMBO) {
                        pControl = new Combo;
                    }
                    else
#endif // ZGUI_USE_COMBO    
#ifdef ZGUI_USE_LABEL
					if (className == ZGUI_LABEL) {
                        pControl = new Label;
                    }
#endif // ZGUI_USE_LABEL
//                 else if (pstrClass == ZGUI_FLASH) {
//                     pControl = new CFlashUI;
//                 }
                    break;
                case 6:
#ifdef ZGUI_USE_LABEL
#ifdef ZGUI_USE_BUTTON
					if (className == ZGUI_BUTTON) {
                        pControl = new Button;
                    }
#endif // ZGUI_USE_BUTTON
#endif // ZGUI_USE_LABEL
#ifdef ZGUI_USE_SLIDER
					else if (className == ZGUI_SLIDER) {
                        pControl = new CSliderUI;
                    }
#endif // ZGUI_USE_SLIDER
#ifdef ZGUI_USE_WEBCAMERA
					else if (className == ZGUI_WEBCAM) {
                        pControl = new WebCamera;
                    }
#endif // ZGUI_USE_WEBCAMERA
					break;
                case 7:
					if (className == ZGUI_CONTROL) {
                        pControl = new Control;
                    }
                    else
#ifdef ZGUI_USE_ACTIVEX
					if (className == ZGUI_ACTIVEX) {
                        pControl = new ActiveX;
                    }
#endif // ZGUI_USE_ACTIVEX
                    break;
                case 8:
#ifdef ZGUI_USE_PROGRESS
					if (className == ZGUI_PROGRESS) {
                        pControl = new Progress;
                    }
                    else
#endif // ZGUI_USE_PROGRESS
#ifdef ZGUI_USE_RICHEDIT
					if (className == ZGUI_RICHEDIT) {
                        pControl = new CRichEditUI;
                    }
                    else
#endif // ZGUI_USE_RICHEDIT
#ifdef ZGUI_USE_DATETIME
					if (className == ZGUI_DATETIME) {
                        pControl = new CDateTimeUI;
                    }
#endif // ZGUI_USE_DATETIME
#ifdef ZGUI_USE_TREEVIEW
					if (className == ZGUI_TREEVIEW) {
						pControl = new TreeView;
					}
#endif // ZGUI_USE_DATETIME
                    break;
                case 9:
					if (className == ZGUI_CONTAINER) {
                        pControl = new Container;
                    }
                    else
#ifdef ZGUI_USE_TABLAYOUT
					if (className == ZGUI_TABLAYOUT) {
                        pControl = new TabLayout;
                    }
                    else
#endif // ZGUI_USE_TABLAYOUT
#ifdef ZGUI_USE_SCROLLBAR
					if (className == ZGUI_SCROLLBAR) {
                        pControl = new ScrollBar;
                    }
#endif // ZGUI_USE_SCROLLBAR
                    break;
                case 10:
#ifdef ZGUI_USE_LIST
					if (className == ZGUI_LISTHEADER) {
                        pControl = new ListHeader;
                    }
                    else
#endif // ZGUI_USE_LIST
#ifdef ZGUI_USE_TILELAYOUT
					if (className == ZGUI_TILELAYOUT) {
                        pControl = new CTileLayoutUI;
                    }
                    else
#endif // ZGUI_USE_TILELAYOUT
#ifdef ZGUI_USE_WEBBROWSER
					if (className == ZGUI_WEBBROWSER) {
                        pControl = new WebBrowser;
                    }
#endif // ZGUI_USE_WEBBROWSER
                    break;
			    case 11:
#ifdef ZGUI_USE_CHILDLAYOUT
					if (className == ZGUI_CHILDLAYOUT) {
                        pControl=new CChildLayoutUI;
                    }
#endif // ZGUI_USE_CHILDLAYOUT
				    break;
                case 14:
					if (className == ZGUI_VERTICALLAYOUT) {
                        pControl = new VerticalLayout;
                    }
                    else
#ifdef ZGUI_USE_LIST
					if (className == ZGUI_LISTHEADERITEM) {
                        pControl = new ListHeaderItem;
                    }
#endif // ZGUI_USE_LIST
                    break;
                case 15:
#ifdef ZGUI_USE_LIST
					if (className == ZGUI_LISTTEXTELEMENT) {
                        pControl = new ListTextElement;
                    }
#endif // ZGUI_USE_LIST
                    break;
                case 16:
					if (className == ZGUI_HORIZONTALLAYOUT) {
                        pControl = new HorizontalLayout;
                    }
                    else
#ifdef ZGUI_USE_LIST
					if (className == ZGUI_LISTLABELELEMENT) {
                        pControl = new ListLabelElement;
                    }
#endif // ZGUI_USE_LIST
                    break;
                case 20:
#ifdef ZGUI_USE_LIST
					if (className == ZGUI_LISTCONTAINERELEMENT) {
                        pControl = new ListContainerElement;
                    }
#endif // ZGUI_USE_LIST
                    break;
            }
            if (pControl == 0 && _pCallback != 0) {
                pControl = _pCallback->createControl(className, _pCallback, pManager);
            }
        }

#ifndef _DEBUG
        zgui_assert(pControl);
#endif // _DEBUG
		if (pControl == 0) {
#ifdef _DEBUG
			TRACE(L"Unknown control: %s", className);
#else
			continue;
#endif
		}

        // Attach to parent
        if (pParent != 0) {
            if (pContainer == 0) {
                pContainer = static_cast<IContainer*>(pParent->getInterface("IContainer"));
            }
            zgui_assert(pContainer);
            if (pContainer == 0) {
                return 0;
            }
            if (!pContainer->add(pControl) ) {
                delete pControl;
                continue;
            }
        }

		// Add children
		if (node.HasChildren()) {
			parse(&node, pControl, pManager);
		}

        // Init default attributes
        if (pManager != 0) {
            pControl->SetManager(pManager, pParent, false);
            const String& defaultAttributes = pManager->getDefaultAttributeList(className);
            if (!defaultAttributes.isEmpty()) {
                pControl->applyAttributeList(defaultAttributes);
            }
        }
        // Process attributes
        if (node.HasAttributes()) {
            // Set ordinary attributes
            int nAttributes = node.GetAttributeCount();
            for (int i = 0; i < nAttributes; ++i) {
				String nodeName = node.GetAttributeName(i);
				String nodeValue = node.GetAttributeValue(i);
				if (nodeName == "style") {
					pManager->acceptStyle(pControl, nodeValue);
				}
				else {
					pControl->setAttribute(nodeName, nodeValue);
				}
            }
        }

        // Return first item
        if (pReturn == 0) {
            pReturn = pControl;
        }
    }
    return pReturn;
}

} // namespace zgui
