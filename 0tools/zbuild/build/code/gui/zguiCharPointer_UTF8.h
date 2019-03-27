#ifndef __ZGUI_CHARPOINTER_UTF8_H_
#define __ZGUI_CHARPOINTER_UTF8_H_

/**
    Wraps a pointer to a null-terminated UTF-8 character string, and provides
    various methods to operate on the data.
    @see CharPointer_UTF16, CharPointer_UTF32
*/
class CharPointer_UTF8
{
public:
    typedef char CharType;

    inline explicit CharPointer_UTF8 (const CharType* const rawPointer) throw()
        : data (const_cast <CharType*> (rawPointer))
    {
    }

    inline CharPointer_UTF8 (const CharPointer_UTF8& other) throw()
        : data (other.data)
    {
    }

    inline CharPointer_UTF8& operator= (const CharPointer_UTF8& other) throw()
    {
        data = other.data;
        return *this;
    }

    inline CharPointer_UTF8& operator= (const CharType* text) throw()
    {
        data = const_cast <CharType*> (text);
        return *this;
    }

    /** This is a pointer comparison, it doesn't compare the actual text. */
    inline bool operator== (const CharPointer_UTF8& other) const throw() { return data == other.data; }
    inline bool operator!= (const CharPointer_UTF8& other) const throw() { return data != other.data; }
    inline bool operator<= (const CharPointer_UTF8& other) const throw() { return data <= other.data; }
    inline bool operator<  (const CharPointer_UTF8& other) const throw() { return data <  other.data; }
    inline bool operator>= (const CharPointer_UTF8& other) const throw() { return data >= other.data; }
    inline bool operator>  (const CharPointer_UTF8& other) const throw() { return data >  other.data; }

    /** Returns the address that this pointer is pointing to. */
    inline CharType* getAddress() const throw()        { return data; }

    /** Returns the address that this pointer is pointing to. */
    inline operator const CharType*() const throw()    { return data; }

    /** Returns true if this pointer is pointing to a null character. */
    inline bool isEmpty() const throw()                { return *data == 0; }

    /** Returns the unicode character that this pointer is pointing to. */
    zgui_wchar operator*() const throw()
    {
        const signed char byte = (signed char) *data;

        if (byte >= 0)
            return (zgui_wchar) (uint8_t) byte;

        uint32_t n = (uint32_t) (uint8_t) byte;
        uint32_t mask = 0x7f;
        uint32_t bit = 0x40;
        size_t numExtraValues = 0;

        while ((n & bit) != 0 && bit > 0x10)
        {
            mask >>= 1;
            ++numExtraValues;
            bit >>= 1;
        }

        n &= mask;

        for (size_t i = 1; i <= numExtraValues; ++i)
        {
            const uint8_t nextByte = (uint8_t) data [i];

            if ((nextByte & 0xc0) != 0x80)
                break;

            n <<= 6;
            n |= (nextByte & 0x3f);
        }

        return (zgui_wchar) n;
    }

    /** Moves this pointer along to the next character in the string. */
    CharPointer_UTF8& operator++() throw()
    {
        const signed char n = (signed char) *data++;

        if (n < 0)
        {
            zgui_wchar bit = 0x40;

            while ((n & bit) != 0 && bit > 0x8)
            {
                ++data;
                bit >>= 1;
            }
        }

        return *this;
    }

    /** Moves this pointer back to the previous character in the string. */
    CharPointer_UTF8& operator--() throw()
    {
        const char n = *--data;

        if ((n & 0xc0) == 0xc0)
        {
            int count = 3;

            do
            {
                --data;
            }
            while ((*data & 0xc0) == 0xc0 && --count >= 0);
        }

        return *this;
    }

    /** Returns the character that this pointer is currently pointing to, and then
        advances the pointer to point to the next character. */
    zgui_wchar getAndAdvance() throw()
    {
        const signed char byte = (signed char) *data++;

        if (byte >= 0)
            return (zgui_wchar) (uint8_t) byte;

        uint32_t n = (uint32_t) (uint8_t) byte;
        uint32_t mask = 0x7f;
        uint32_t bit = 0x40;
        int numExtraValues = 0;

        while ((n & bit) != 0 && bit > 0x8)
        {
            mask >>= 1;
            ++numExtraValues;
            bit >>= 1;
        }

        n &= mask;

        while (--numExtraValues >= 0)
        {
            const uint32_t nextByte = (uint32_t) (uint8_t) *data++;

            if ((nextByte & 0xc0) != 0x80)
                break;

            n <<= 6;
            n |= (nextByte & 0x3f);
        }

        return (zgui_wchar) n;
    }

    /** Moves this pointer along to the next character in the string. */
    CharPointer_UTF8 operator++ (int) throw()
    {
        CharPointer_UTF8 temp (*this);
        ++*this;
        return temp;
    }

    /** Moves this pointer forwards by the specified number of characters. */
    void operator+= (int numToSkip) throw()
    {
        if (numToSkip < 0)
        {
            while (++numToSkip <= 0)
                --*this;
        }
        else
        {
            while (--numToSkip >= 0)
                ++*this;
        }
    }

    /** Moves this pointer backwards by the specified number of characters. */
    void operator-= (int numToSkip) throw()
    {
        operator+= (-numToSkip);
    }

    /** Returns the character at a given character index from the start of the string. */
    zgui_wchar operator[] (int characterIndex) const throw()
    {
        CharPointer_UTF8 p (*this);
        p += characterIndex;
        return *p;
    }

    /** Returns a pointer which is moved forwards from this one by the specified number of characters. */
    CharPointer_UTF8 operator+ (int numToSkip) const throw()
    {
        CharPointer_UTF8 p (*this);
        p += numToSkip;
        return p;
    }

    /** Returns a pointer which is moved backwards from this one by the specified number of characters. */
    CharPointer_UTF8 operator- (int numToSkip) const throw()
    {
        CharPointer_UTF8 p (*this);
        p += -numToSkip;
        return p;
    }

    /** Returns the number of characters in this string. */
    size_t length() const throw()
    {
        const CharType* d = data;
        size_t count = 0;

        for (;;)
        {
            const uint32_t n = (uint32_t) (uint8_t) *d++;

            if ((n & 0x80) != 0)
            {
                uint32_t bit = 0x40;

                while ((n & bit) != 0)
                {
                    ++d;
                    bit >>= 1;

                    if (bit == 0)
                        break; // illegal utf-8 sequence
                }
            }
            else if (n == 0)
                break;

            ++count;
        }

        return count;
    }

    /** Returns the number of characters in this string, or the given value, whichever is lower. */
    size_t lengthUpTo (const size_t maxCharsToCount) const throw()
    {
        return CharacterFunctions::lengthUpTo (*this, maxCharsToCount);
    }

    /** Returns the number of characters in this string, or up to the given end pointer, whichever is lower. */
    size_t lengthUpTo (const CharPointer_UTF8& end) const throw()
    {
        return CharacterFunctions::lengthUpTo (*this, end);
    }

    /** Returns the number of bytes that are used to represent this string.
        This includes the terminating null character.
    */
    size_t sizeInBytes() const throw()
    {
        return lstrlenA(data) + 1;
    }

    /** Returns the number of bytes that would be needed to represent the given
        unicode character in this encoding format.
    */
    static size_t getBytesRequiredFor (const zgui_wchar charToWrite) throw()
    {
        size_t num = 1;
        const uint32_t c = (uint32_t) charToWrite;

        if (c >= 0x80)
        {
            ++num;
            if (c >= 0x800)
            {
                ++num;
                if (c >= 0x10000)
                    ++num;
            }
        }

        return num;
    }

    /** Returns the number of bytes that would be needed to represent the given
        string in this encoding format.
        The value returned does NOT include the terminating null character.
    */
    template <class CharPointer>
    static size_t getBytesRequiredFor (CharPointer text) throw()
    {
        size_t count = 0;
        zgui_wchar n;

        while ((n = text.getAndAdvance()) != 0)
            count += getBytesRequiredFor (n);

        return count;
    }

    /** Returns a pointer to the null character that terminates this string. */
    CharPointer_UTF8 findTerminatingNull() const throw()
    {
        return CharPointer_UTF8 (data + lstrlenA(data));
    }

    /** Writes a unicode character to this string, and advances this pointer to point to the next position. */
    void write (const zgui_wchar charToWrite) throw()
    {
        const uint32_t c = (uint32_t) charToWrite;

        if (c >= 0x80)
        {
            int numExtraBytes = 1;
            if (c >= 0x800)
            {
                ++numExtraBytes;
                if (c >= 0x10000)
                    ++numExtraBytes;
            }

            *data++ = (CharType) ((uint32_t) (0xff << (7 - numExtraBytes)) | (c >> (numExtraBytes * 6)));

            while (--numExtraBytes >= 0)
                *data++ = (CharType) (0x80 | (0x3f & (c >> (numExtraBytes * 6))));
        }
        else
        {
            *data++ = (CharType) c;
        }
    }

    /** Writes a null character to this string (leaving the pointer's position unchanged). */
    inline void writeNull() const throw()
    {
        *data = 0;
    }

    /** Copies a source string to this pointer, advancing this pointer as it goes. */
    template <typename CharPointer>
    void writeAll (const CharPointer& src) throw()
    {
        CharacterFunctions::copyAll (*this, src);
    }

    /** Copies a source string to this pointer, advancing this pointer as it goes. */
    void writeAll (const CharPointer_UTF8& src) throw()
    {
        const CharType* s = src.data;

        while ((*data = *s) != 0)
        {
            ++data;
            ++s;
        }
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

    /** Compares this string with another one, up to a specified number of characters. */
    template <typename CharPointer>
    int compareUpTo (const CharPointer& other, const int maxChars) const throw()
    {
        return CharacterFunctions::compareUpTo (*this, other, maxChars);
    }

    /** Compares this string with another one. */
    template <typename CharPointer>
    int compareIgnoreCase (const CharPointer& other) const throw()
    {
        return CharacterFunctions::compareIgnoreCase (*this, other);
    }

    /** Compares this string with another one. */
    int compareIgnoreCase (const CharPointer_UTF8& other) const throw()
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
        return CharacterFunctions::indexOfChar (*this, charToFind);
    }

    /** Returns the character index of a unicode character, or -1 if it isn't found. */
    int indexOf (const zgui_wchar charToFind, const bool ignoreCase) const throw()
    {
        return ignoreCase ? CharacterFunctions::indexOfCharIgnoreCase (*this, charToFind)
                          : CharacterFunctions::indexOfChar (*this, charToFind);
    }

    /** Returns true if the first character of this string is whitespace. */
    bool isWhitespace() const throw()      { return *data == ' ' || (*data <= 13 && *data >= 9); }
    /** Returns true if the first character of this string is a digit. */
    bool isDigit() const throw()           { return *data >= '0' && *data <= '9'; }
    /** Returns true if the first character of this string is a letter. */
    bool isLetter() const throw()          { return CharacterFunctions::isLetter (operator*()) != 0; }
    /** Returns true if the first character of this string is a letter or digit. */
    bool isLetterOrDigit() const throw()   { return CharacterFunctions::isLetterOrDigit (operator*()) != 0; }
    /** Returns true if the first character of this string is upper-case. */
    bool isUpperCase() const throw()       { return CharacterFunctions::isUpperCase (operator*()) != 0; }
    /** Returns true if the first character of this string is lower-case. */
    bool isLowerCase() const throw()       { return CharacterFunctions::isLowerCase (operator*()) != 0; }

    /** Returns an upper-case version of the first character of this string. */
    zgui_wchar toUpperCase() const throw() { return CharacterFunctions::toUpperCase (operator*()); }
    /** Returns a lower-case version of the first character of this string. */
    zgui_wchar toLowerCase() const throw() { return CharacterFunctions::toLowerCase (operator*()); }

    /** Parses this string as a 32-bit integer. */
    int getIntValue32() const throw()      { return atoi (data); }

    /** Parses this string as a 64-bit integer. */
    int64_t getIntValue64() const throw()
    {
        return _atoi64 (data);
    }

    /** Returns the first non-whitespace character in the string. */
    CharPointer_UTF8 findEndOfWhitespace() const throw()   { return CharacterFunctions::findEndOfWhitespace (*this); }

    /** Returns true if the given unicode character can be represented in this encoding. */
    static bool canRepresent (zgui_wchar character) throw()
    {
        return ((unsigned int) character) < (unsigned int) 0x10ffff;
    }

    /** Returns true if this data contains a valid string in this encoding. */
    static bool isValidString (const CharType* dataToTest, int maxBytesToRead)
    {
        while (--maxBytesToRead >= 0 && *dataToTest != 0)
        {
            const signed char byte = (signed char) *dataToTest;

            if (byte < 0)
            {
                uint32_t n = (uint32_t) (uint8_t) byte;
                uint32_t mask = 0x7f;
                uint32_t bit = 0x40;
                int numExtraValues = 0;

                while ((n & bit) != 0)
                {
                    if (bit <= 0x10)
                        return false;

                    mask >>= 1;
                    ++numExtraValues;
                    bit >>= 1;
                }

                n &= mask;

                while (--numExtraValues >= 0)
                {
                    const uint32_t nextByte = (uint32_t) (uint8_t) *dataToTest++;

                    if ((nextByte & 0xc0) != 0x80)
                        return false;
                }
            }
        }

        return true;
    }

    /** Atomically swaps this pointer for a new value, returning the previous value. */
    CharPointer_UTF8 atomicSwap (const CharPointer_UTF8& newValue)
    {
        return CharPointer_UTF8(reinterpret_cast <Atomic<CharType*>&>(data).exchange(newValue.data));
    }

    /** These values are the byte-order-mark (BOM) values for a UTF-8 stream. */
    enum
    {
        byteOrderMark1 = 0xef,
        byteOrderMark2 = 0xbb,
        byteOrderMark3 = 0xbf
    };

private:
    CharType* data;
};

#endif // __ZGUI_CHARPOINTER_UTF8_H_
