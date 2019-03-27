#ifndef __ZGUI_CHARPOINTER_ASCII_H_
#define __ZGUI_CHARPOINTER_ASCII_H_

/**
    Wraps a pointer to a null-terminated ASCII character string, and provides
    various methods to operate on the data.

    A valid ASCII string is assumed to not contain any characters above 127.

    @see CharPointer_UTF8, CharPointer_UTF16, CharPointer_UTF32
*/
class CharPointer_ASCII
{
public:
    typedef char CharType;

    inline explicit CharPointer_ASCII (const CharType* const rawPointer) throw()
        : data (const_cast <CharType*> (rawPointer))
    {
    }

    inline CharPointer_ASCII (const CharPointer_ASCII& other) throw()
        : data (other.data)
    {
    }

    inline CharPointer_ASCII& operator= (const CharPointer_ASCII& other) throw()
    {
        data = other.data;
        return *this;
    }

    inline CharPointer_ASCII& operator= (const CharType* text) throw()
    {
        data = const_cast <CharType*> (text);
        return *this;
    }

    /** This is a pointer comparison, it doesn't compare the actual text. */
    inline bool operator== (const CharPointer_ASCII& other) const throw() { return data == other.data; }
    inline bool operator!= (const CharPointer_ASCII& other) const throw() { return data != other.data; }
    inline bool operator<= (const CharPointer_ASCII& other) const throw() { return data <= other.data; }
    inline bool operator<  (const CharPointer_ASCII& other) const throw() { return data <  other.data; }
    inline bool operator>= (const CharPointer_ASCII& other) const throw() { return data >= other.data; }
    inline bool operator>  (const CharPointer_ASCII& other) const throw() { return data >  other.data; }

    /** Returns the address that this pointer is pointing to. */
    inline CharType* getAddress() const throw()        { return data; }

    /** Returns the address that this pointer is pointing to. */
    inline operator const CharType*() const throw()    { return data; }

    /** Returns true if this pointer is pointing to a null character. */
    inline bool isEmpty() const throw()                { return *data == 0; }

    /** Returns the unicode character that this pointer is pointing to. */
    inline zgui_wchar operator*() const throw()        { return (zgui_wchar) (uint8_t) *data; }

    /** Moves this pointer along to the next character in the string. */
    inline CharPointer_ASCII& operator++() throw()
    {
        ++data;
        return *this;
    }

    /** Moves this pointer to the previous character in the string. */
    inline CharPointer_ASCII& operator--() throw()
    {
        --data;
        return *this;
    }

    /** Returns the character that this pointer is currently pointing to, and then
        advances the pointer to point to the next character. */
    inline zgui_wchar getAndAdvance() throw()  { return (zgui_wchar) (uint8_t) *data++; }

    /** Moves this pointer along to the next character in the string. */
    CharPointer_ASCII operator++ (int) throw()
    {
        CharPointer_ASCII temp (*this);
        ++data;
        return temp;
    }

    /** Moves this pointer forwards by the specified number of characters. */
    inline void operator+= (const int numToSkip) throw()
    {
        data += numToSkip;
    }

    inline void operator-= (const int numToSkip) throw()
    {
        data -= numToSkip;
    }

    /** Returns the character at a given character index from the start of the string. */
    inline zgui_wchar operator[] (const int characterIndex) const throw()
    {
        return (zgui_wchar) (unsigned char) data [characterIndex];
    }

    /** Returns a pointer which is moved forwards from this one by the specified number of characters. */
    CharPointer_ASCII operator+ (const int numToSkip) const throw()
    {
        return CharPointer_ASCII (data + numToSkip);
    }

    /** Returns a pointer which is moved backwards from this one by the specified number of characters. */
    CharPointer_ASCII operator- (const int numToSkip) const throw()
    {
        return CharPointer_ASCII (data - numToSkip);
    }

    /** Writes a unicode character to this string, and advances this pointer to point to the next position. */
    inline void write (const zgui_wchar charToWrite) throw()
    {
        *data++ = (char) charToWrite;
    }

    inline void replaceChar (const zgui_wchar newChar) throw()
    {
        *data = (char) newChar;
    }

    /** Writes a null character to this string (leaving the pointer's position unchanged). */
    inline void writeNull() const throw()
    {
        *data = 0;
    }

    /** Returns the number of characters in this string. */
    size_t length() const throw()
    {
        return (size_t)lstrlenA(data);
    }

    /** Returns the number of characters in this string, or the given value, whichever is lower. */
    size_t lengthUpTo (const size_t maxCharsToCount) const throw()
    {
        return CharacterFunctions::lengthUpTo(*this, maxCharsToCount);
    }

    /** Returns the number of characters in this string, or up to the given end pointer, whichever is lower. */
    size_t lengthUpTo (const CharPointer_ASCII& end) const throw()
    {
        return CharacterFunctions::lengthUpTo (*this, end);
    }

    /** Returns the number of bytes that are used to represent this string.
        This includes the terminating null character.
    */
    size_t sizeInBytes() const throw()
    {
        return length() + 1;
    }

    /** Returns the number of bytes that would be needed to represent the given
        unicode character in this encoding format.
    */
    static inline size_t getBytesRequiredFor (const zgui_wchar) throw()
    {
        return 1;
    }

    /** Returns the number of bytes that would be needed to represent the given
        string in this encoding format.
        The value returned does NOT include the terminating null character.
    */
    template <class CharPointer>
    static size_t getBytesRequiredFor (const CharPointer& text) throw()
    {
        return text.length();
    }

    /** Returns a pointer to the null character that terminates this string. */
    CharPointer_ASCII findTerminatingNull() const throw()
    {
        return CharPointer_ASCII (data + length());
    }

    /** Copies a source string to this pointer, advancing this pointer as it goes. */
    template <typename CharPointer>
    void writeAll (const CharPointer& src) throw()
    {
        CharacterFunctions::copyAll(*this, src);
    }

    /** Copies a source string to this pointer, advancing this pointer as it goes. */
    void writeAll (const CharPointer_ASCII& src) throw()
    {
        lstrcpyA(data, src.data);
    }

    /** Copies a source string to this pointer, advancing this pointer as it goes.
        The maxDestBytes parameter specifies the maximum number of bytes that can be written
        to the destination buffer before stopping.
    */
    template <typename CharPointer>
    int writeWithDestByteLimit (const CharPointer& src, const int maxDestBytes) throw()
    {
        return CharacterFunctions::copyWithDestByteLimit (*this, src, maxDestBytes);
    }

    /** Copies a source string to this pointer, advancing this pointer as it goes.
        The maxChars parameter specifies the maximum number of characters that can be
        written to the destination buffer before stopping (including the terminating null).
    */
    template <typename CharPointer>
    void writeWithCharLimit (const CharPointer& src, const int maxChars) throw()
    {
        CharacterFunctions::copyWithCharLimit (*this, src, maxChars);
    }

    /** Compares this string with another one. */
    template <typename CharPointer>
    int compare (const CharPointer& other) const throw()
    {
        return CharacterFunctions::compare (*this, other);
    }

    /** Compares this string with another one. */
    int compare (const CharPointer_ASCII& other) const throw()
    {
        return lstrcmpA(data, other.data);
    }

    /** Compares this string with another one, up to a specified number of characters. */
    template <typename CharPointer>
    int compareUpTo (const CharPointer& other, const int maxChars) const throw()
    {
        return CharacterFunctions::compareUpTo (*this, other, maxChars);
    }

    /** Compares this string with another one, up to a specified number of characters. */
    int compareUpTo (const CharPointer_ASCII& other, const int maxChars) const throw()
    {
        return strncmp(data, other.data, (size_t) maxChars);
    }

    /** Compares this string with another one. */
    template <typename CharPointer>
    int compareIgnoreCase (const CharPointer& other) const
    {
        return CharacterFunctions::compareIgnoreCase (*this, other);
    }

    int compareIgnoreCase (const CharPointer_ASCII& other) const
    {
        return _stricmp(data, other.data);
    }

    /** Compares this string with another one, up to a specified number of characters. */
    template <typename CharPointer>
    int compareIgnoreCaseUpTo (const CharPointer& other, const int maxChars) const throw()
    {
        return CharacterFunctions::compareIgnoreCaseUpTo (*this, other, maxChars);
    }

    /** Returns the character index of a substring, or -1 if it isn't found. */
    template <typename CharPointer>
    int indexOf (const CharPointer& stringToFind) const throw()
    {
        return CharacterFunctions::indexOf (*this, stringToFind);
    }

    /** Returns the character index of a unicode character, or -1 if it isn't found. */
    int indexOf (const zgui_wchar charToFind) const throw()
    {
        int i = 0;

        while (data[i] != 0)
        {
            if (data[i] == (char) charToFind)
                return i;

            ++i;
        }

        return -1;
    }

    /** Returns the character index of a unicode character, or -1 if it isn't found. */
    int indexOf (const zgui_wchar charToFind, const bool ignoreCase) const throw()
    {
        return ignoreCase ? CharacterFunctions::indexOfCharIgnoreCase (*this, charToFind)
                          : CharacterFunctions::indexOfChar (*this, charToFind);
    }

    /** Returns true if the first character of this string is whitespace. */
    bool isWhitespace() const               { return CharacterFunctions::isWhitespace (*data) != 0; }
    /** Returns true if the first character of this string is a digit. */
    bool isDigit() const                    { return CharacterFunctions::isDigit (*data) != 0; }
    /** Returns true if the first character of this string is a letter. */
    bool isLetter() const                   { return CharacterFunctions::isLetter (*data) != 0; }
    /** Returns true if the first character of this string is a letter or digit. */
    bool isLetterOrDigit() const            { return CharacterFunctions::isLetterOrDigit (*data) != 0; }
    /** Returns true if the first character of this string is upper-case. */
    bool isUpperCase() const                { return CharacterFunctions::isUpperCase ((zgui_wchar) (uint8_t) *data) != 0; }
    /** Returns true if the first character of this string is lower-case. */
    bool isLowerCase() const                { return CharacterFunctions::isLowerCase ((zgui_wchar) (uint8_t) *data) != 0; }

    /** Returns an upper-case version of the first character of this string. */
    zgui_wchar toUpperCase() const throw() { return CharacterFunctions::toUpperCase ((zgui_wchar) (uint8_t) *data); }
    /** Returns a lower-case version of the first character of this string. */
    zgui_wchar toLowerCase() const throw() { return CharacterFunctions::toLowerCase ((zgui_wchar) (uint8_t) *data); }

    /** Parses this string as a 32-bit integer. */
    int getIntValue32() const throw()      { return atoi (data); }

    /** Parses this string as a 64-bit integer. */
    int64_t getIntValue64() const throw()
    {
        return _atoi64 (data);
    }

    /** Returns the first non-whitespace character in the string. */
    CharPointer_ASCII findEndOfWhitespace() const throw()   { return CharacterFunctions::findEndOfWhitespace (*this); }

    /** Returns true if the given unicode character can be represented in this encoding. */
    static bool canRepresent (zgui_wchar character) throw()
    {
        return ((unsigned int) character) < (unsigned int) 128;
    }

    /** Returns true if this data contains a valid string in this encoding. */
    static bool isValidString (const CharType* dataToTest, int maxBytesToRead)
    {
        while (--maxBytesToRead >= 0)
        {
            if (((signed char) *dataToTest) <= 0)
                return *dataToTest == 0;

            ++dataToTest;
        }

        return true;
    }

private:
    CharType* data;
};


#endif // __ZGUI_CHARPOINTER_ASCII_H_
