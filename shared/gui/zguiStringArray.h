#ifndef __ZGUI_STRINGARRAY_H_
#define __ZGUI_STRINGARRAY_H_

namespace zgui
{

class StringArray
{
public:
	StringArray();
	StringArray(const StringArray&);
	explicit StringArray(const String& firstValue);
	StringArray(const String* strings, int numberOfStrings);
	StringArray(const char* const* strings, int numberOfStrings);
	explicit StringArray(const char* const* strings);
	explicit StringArray(const wchar_t* const* strings);
	StringArray(const wchar_t* const* strings, int numberOfStrings);
	~StringArray();

	StringArray& operator= (const StringArray&);

	void swapWith(StringArray&);

	bool operator== (const StringArray&) const;
	bool operator!= (const StringArray&) const;

	inline int size() const{ return strings.size(); };

	const String& operator[] (int index) const;

	String& getReference(int index);

	inline String* begin() const { return strings.begin(); }
	inline String* end() const { return strings.end(); }

	bool contains(const String& stringToLookFor, bool ignoreCase = false) const;
	int indexOf(const String& stringToLookFor, bool ignoreCase = false, int startIndex = 0) const;
	void add(const String& stringToAdd);
	void insert(int index, const String& stringToAdd);
	void addIfNotAlreadyThere(const String& stringToAdd, bool ignoreCase = false);
	void set(int index, const String& newString);
	void addArray(const StringArray& other, int startIndex = 0, int numElementsToAdd = -1);
	int addTokens(const String& stringToTokenise, bool preserveQuotedStrings);
	int addTokens(const String& stringToTokenise, const String& breakCharacters, const String& quoteCharacters);
	int addLines(const String& stringToBreakUp);

	static StringArray fromTokens(const String& stringToTokenise, bool preserveQuotedStrings);
	static StringArray fromTokens(const String& stringToTokenise, const String& breakCharacters, const String& quoteCharacters);
	static StringArray fromLines(const String& stringToBreakUp);

	void clear();
	void clearQuick();
	void remove(int index);
	void removeString(const String& stringToRemove, bool ignoreCase = false);
	void removeRange(int startIndex, int numberToRemove);
	void removeDuplicates(bool ignoreCase);
	void removeEmptyStrings(bool removeWhitespaceStrings = true);
	void move(int currentIndex, int newIndex);
	void trim();

	void appendNumbersToDuplicates(bool ignoreCaseWhenComparing, bool appendNumberToFirstInstance, CharPointer_UTF8 preNumberString = CharPointer_UTF8(0), CharPointer_UTF8 postNumberString = CharPointer_UTF8(0));

	String joinIntoString(const String& separatorString, int startIndex = 0, int numberOfElements = -1) const;

	void sort(bool ignoreCase);

	void ensureStorageAllocated(int minNumElements);

	void minimiseStorageOverheads();

	Array<String> strings;
};

}

#endif // __ZGUI_STRINGARRAY_H_
