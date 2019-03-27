#include "zgui.h"

namespace zgui {

ZGUI_IMPLEMENT_SINGLETON(Language);

Language::Language()
{

}

Language::~Language()
{

}

void Language::addLanguage(const String& name, const String& fileName, const bool isDefault)
{
	StringPairArray pairs;
	String content = Resources::getInstance()->getString(fileName);
	StringArray lines;
	lines.addLines(content);
	lines.trim();

	for (int i = lines.size(); --i >= 0; ) {
		StringArray pair;
		pair.addTokens(lines[i], ":", "'");
		pair.trim();
		if (pair.size() == 2) {
			String key = pair[0];
			String val = pair[1];

			key = key.unquoted();
			val = val.unquoted();
			val = val.replace("<br/>", "\r\n", true);
			val = val.replace("&#39;", "'");
//			val = val.replace()

			if (!key.isEmpty()) {
				pairs.set(key, val);
			}
		}
	}
	
	if (pairs.size() > 0) {
		StringPairArray* pNewMap = new StringPairArray(pairs);
		_langs.add(name);
		_langMaps.add(pNewMap);

		if (isDefault) {
			_currentName = name;
			_currentMap = pNewMap;
		}

		_onLanguageLoaded(pNewMap);
	}
}

void Language::setLanguage(const String& name)
{
	int i = _langs.indexOf(name);
	if (i >= 0) {
		_currentName = name;
		_currentMap = _langMaps[i];
	}
}

void Language::addTranslation(const String& lang, const String& key, const String& val)
{
	if (_langs.contains(lang)) {
		StringPairArray* pLang = _langMaps.getUnchecked(_langs.indexOf(lang));
		if (pLang != 0) {
			pLang->set(key, val);
		}
	}
}

const String& Language::translate(const String& text)
{
	if (_currentMap == 0) {
		return text;
	}
	return _currentMap->getValue(text, text);
}

String Language::scanTextAndTranslate(const String& text)
{
	String newText;

	if (_currentMap == 0) {
		return text;
	}

	String::CharPointerType itr = text.getCharPointer();
	String::CharPointerType end = text.getCharPointer() + text.length();

	for (; itr != end; ++itr) {
		if (*itr != '$') {
			newText += *itr;
		}
		else {
			String part;

			++itr;
			for (; itr != end; ++itr) {
				if (*itr == '$') {
					break;
				}
				part += *itr;
			}
			part = translate(part);
			newText += part;
		}
	}

	return newText;
}

}
