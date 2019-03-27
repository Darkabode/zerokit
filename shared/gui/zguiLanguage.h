#ifndef __ZGUI_LANGUAGE_H_
#define __ZGUI_LANGUAGE_H_

namespace zgui {

class Language
{
public:
	Language();
	~Language();

	void addLanguage(const String& name, const String& fileName, const bool isDefault);
	void setLanguage(const String& name);
	void addTranslation(const String& lang, const String& key, const String& val);
	const StringArray& getAllLangs() const { return _langs; }

	const String& getName() const throw() { return _currentName; }
	const String& translate(const String& text);
	String scanTextAndTranslate(const String& text);

	EventSource _onLanguageLoaded;
	ZGUI_DECLARE_SINGLETON(Language);
private:
	ZGUI_DECLARE_NON_COPYABLE(Language);

	StringArray _langs;
	Array<StringPairArray*> _langMaps;
	String _currentName;
	StringPairArray* _currentMap;
};


}
 

#endif // __ZGUI_LANGUAGE_H_
