#ifndef __ZGUI_CHARACTERFUNCTIONS_H_
#define __ZGUI_CHARACTERFUNCTIONS_H_

typedef uint32_t zgui_wchar;

/** This macro is deprecated, but preserved for compatibility with old code. */
#define JUCE_T(stringLiteral)   (L##stringLiteral)

#if JUCE_DEFINE_T_MACRO
 /** The 'T' macro is an alternative for using the "L" prefix in front of a string literal.

     This macro is deprecated, but available for compatibility with old code if you set
     JUCE_DEFINE_T_MACRO = 1. The fastest, most portable and best way to write your string
     literals is as standard char strings, using escaped utf-8 character sequences for extended
     characters, rather than trying to store them as wide-char strings.
 */
 #define T(stringLiteral)   JUCE_T(stringLiteral)
#endif

//==============================================================================
/**
    A collection of functions for manipulating characters and character strings.

    Most of these methods are designed for internal use by the String and CharPointer
    classes, but some of them may be useful to call directly.

    @see String, CharPointer_UTF8, CharPointer_UTF16, CharPointer_UTF32
*/
class CharacterFunctions
{
public:
    //==============================================================================
    /** Converts a character to upper-case. */
    static zgui_wchar toUpperCase (zgui_wchar character) throw();
    /** Converts a character to lower-case. */
    static zgui_wchar toLowerCase (zgui_wchar character) throw();

    /** Checks whether a unicode character is upper-case. */
    static bool isUpperCase (zgui_wchar character) throw();
    /** Checks whether a unicode character is lower-case. */
    static bool isLowerCase (zgui_wchar character) throw();

    /** Checks whether a character is whitespace. */
    static bool isWhitespace (char character) throw();
    /** Checks whether a character is whitespace. */
    static bool isWhitespace (zgui_wchar character) throw();

    /** Checks whether a character is a digit. */
    static bool isDigit (char character) throw();
    /** Checks whether a character is a digit. */
    static bool isDigit (zgui_wchar character) throw();

    /** Checks whether a character is alphabetic. */
    static bool isLetter (char character) throw();
    /** Checks whether a character is alphabetic. */
    static bool isLetter (zgui_wchar character) throw();

    /** Checks whether a character is alphabetic or numeric. */
    static bool isLetterOrDigit (char character) throw();
    /** Checks whether a character is alphabetic or numeric. */
    static bool isLetterOrDigit (zgui_wchar character) throw();

    /** Returns 0 to 16 for '0' to 'F", or -1 for characters that aren't a legal hex digit. */
    static int getHexDigitValue (zgui_wchar digit) throw();

    //==============================================================================
    /** Parses a character string, to read an integer value. */
    template <typename IntType, typename CharPointerType>
    static IntType getIntValue (const CharPointerType& text) throw()
    {
        IntType v = 0;
        CharPointerType s (text.findEndOfWhitespace());

        const bool isNeg = *s == '-';
        if (isNeg)
            ++s;

        for (;;)
        {
            const zgui_wchar c = s.getAndAdvance();

            if (c >= '0' && c <= '9')
                v = v * 10 + (IntType) (c - '0');
            else
                break;
        }

        return isNeg ? -v : v;
    }

    //==============================================================================
    /** Counts the number of characters in a given string, stopping if the count exceeds
        a specified limit. */
    template <typename CharPointerType>
    static size_t lengthUpTo (CharPointerType text, const size_t maxCharsToCount) throw()
    {
        size_t len = 0;

        while (len < maxCharsToCount && text.getAndAdvance() != 0)
            ++len;

        return len;
    }

    /** Counts the number of characters in a given string, stopping if the count exceeds
        a specified end-pointer. */
    template <typename CharPointerType>
    static size_t lengthUpTo (CharPointerType start, const CharPointerType& end) throw()
    {
        size_t len = 0;

        while (start < end && start.getAndAdvance() != 0)
            ++len;

        return len;
    }

    /** Copies null-terminated characters from one string to another. */
    template <typename DestCharPointerType, typename SrcCharPointerType>
    static void copyAll (DestCharPointerType& dest, SrcCharPointerType src) throw()
    {
        for (;;)
        {
            const zgui_wchar c = src.getAndAdvance();

            if (c == 0)
                break;

            dest.write (c);
        }

        dest.writeNull();
    }

    /** Copies characters from one string to another, up to a null terminator
        or a given byte size limit. */
    template <typename DestCharPointerType, typename SrcCharPointerType>
    static int copyWithDestByteLimit (DestCharPointerType& dest, SrcCharPointerType src, int maxBytes) throw()
    {
        typename DestCharPointerType::CharType const* const startAddress = dest.getAddress();
        maxBytes -= sizeof (typename DestCharPointerType::CharType); // (allow for a terminating null)

        for (;;)
        {
            const zgui_wchar c = src.getAndAdvance();
            const int bytesNeeded = (int) DestCharPointerType::getBytesRequiredFor (c);

            maxBytes -= bytesNeeded;
            if (c == 0 || maxBytes < 0)
                break;

            dest.write (c);
        }

        dest.writeNull();

        return (int) (getAddressDifference (dest.getAddress(), startAddress) + sizeof (typename DestCharPointerType::CharType));
    }

    /** Copies characters from one string to another, up to a null terminator
        or a given maximum number of characters. */
    template <typename DestCharPointerType, typename SrcCharPointerType>
    static void copyWithCharLimit (DestCharPointerType& dest, SrcCharPointerType src, int maxChars) throw()
    {
        while (--maxChars > 0)
        {
            const zgui_wchar c = src.getAndAdvance();
            if (c == 0)
                break;

            dest.write (c);
        }

        dest.writeNull();
    }

    /** Compares two null-terminated character strings. */
    template <typename CharPointerType1, typename CharPointerType2>
    static int compare (CharPointerType1 s1, CharPointerType2 s2) throw()
    {
        for (;;)
        {
            const int c1 = (int) s1.getAndAdvance();
            const int c2 = (int) s2.getAndAdvance();

            const int diff = c1 - c2;
            if (diff != 0)
                return diff < 0 ? -1 : 1;
            else if (c1 == 0)
                break;
        }

        return 0;
    }

    /** Compares two null-terminated character strings, up to a given number of characters. */
    template <typename CharPointerType1, typename CharPointerType2>
    static int compareUpTo (CharPointerType1 s1, CharPointerType2 s2, int maxChars) throw()
    {
        while (--maxChars >= 0)
        {
            const int c1 = (int) s1.getAndAdvance();
            const int c2 = (int) s2.getAndAdvance();

            const int diff = c1 - c2;
            if (diff != 0)
                return diff < 0 ? -1 : 1;
            else if (c1 == 0)
                break;
        }

        return 0;
    }

    /** Compares two null-terminated character strings, using a case-independant match. */
    template <typename CharPointerType1, typename CharPointerType2>
    static int compareIgnoreCase (CharPointerType1 s1, CharPointerType2 s2) throw()
    {
        for (;;)
        {
            int c1 = (int) s1.toUpperCase();
            int c2 = (int) s2.toUpperCase();
            ++s1;
            ++s2;

            const int diff = c1 - c2;
            if (diff != 0)
                return diff < 0 ? -1 : 1;
            else if (c1 == 0)
                break;
        }

        return 0;
    }

    /** Compares two null-terminated character strings, using a case-independent match. */
    template <typename CharPointerType1, typename CharPointerType2>
    static int compareIgnoreCaseUpTo (CharPointerType1 s1, CharPointerType2 s2, int maxChars) throw()
    {
        while (--maxChars >= 0)
        {
            int c1 = s1.toUpperCase();
            int c2 = s2.toUpperCase();
            ++s1;
            ++s2;

            const int diff = c1 - c2;
            if (diff != 0)
                return diff < 0 ? -1 : 1;
            else if (c1 == 0)
                break;
        }

        return 0;
    }

    /** Finds the character index of a given substring in another string.
        Returns -1 if the substring is not found.
    */
    template <typename CharPointerType1, typename CharPointerType2>
    static int indexOf (CharPointerType1 textToSearch, const CharPointerType2& substringToLookFor) throw()
    {
        int index = 0;
        const int substringLength = (int) substringToLookFor.length();

        for (;;)
        {
            if (textToSearch.compareUpTo (substringToLookFor, substringLength) == 0)
                return index;

            if (textToSearch.getAndAdvance() == 0)
                return -1;

            ++index;
        }
    }

    /** Returns a pointer to the first occurrence of a substring in a string.
        If the substring is not found, this will return a pointer to the string's
        null terminator.
    */
    template <typename CharPointerType1, typename CharPointerType2>
    static CharPointerType1 find (CharPointerType1 textToSearch, const CharPointerType2& substringToLookFor) throw()
    {
        const int substringLength = (int) substringToLookFor.length();

        while (textToSearch.compareUpTo (substringToLookFor, substringLength) != 0 && ! textToSearch.isEmpty()) {
            ++textToSearch;
        }

        return textToSearch;
    }

    /** Finds the character index of a given substring in another string, using
        a case-independent match.
        Returns -1 if the substring is not found.
    */
    template <typename CharPointerType1, typename CharPointerType2>
    static int indexOfIgnoreCase (CharPointerType1 haystack, const CharPointerType2& needle) throw()
    {
        int index = 0;
        const int needleLength = (int) needle.length();

        for (;;)
        {
            if (haystack.compareIgnoreCaseUpTo (needle, needleLength) == 0)
                return index;

            if (haystack.getAndAdvance() == 0)
                return -1;

            ++index;
        }
    }

    /** Finds the character index of a given character in another string.
        Returns -1 if the character is not found.
    */
    template <typename Type>
    static int indexOfChar (Type text, const zgui_wchar charToFind) throw()
    {
        int i = 0;

        while (! text.isEmpty())
        {
            if (text.getAndAdvance() == charToFind)
                return i;

            ++i;
        }

        return -1;
    }

    /** Finds the character index of a given character in another string, using
        a case-independent match.
        Returns -1 if the character is not found.
    */
    template <typename Type>
    static int indexOfCharIgnoreCase (Type text, zgui_wchar charToFind) throw()
    {
        charToFind = CharacterFunctions::toLowerCase (charToFind);
        int i = 0;

        while (! text.isEmpty())
        {
            if (text.toLowerCase() == charToFind)
                return i;

            ++text;
            ++i;
        }

        return -1;
    }

    /** Returns a pointer to the first non-whitespace character in a string.
        If the string contains only whitespace, this will return a pointer
        to its null terminator.
    */
    template <typename Type>
    static Type findEndOfWhitespace (const Type& text) throw()
    {
        Type p (text);

        while (p.isWhitespace())
            ++p;

        return p;
    }

    /** Returns a pointer to the first character in the string which is found in
        the breakCharacters string.
    */
    template <typename Type>
    static Type findEndOfToken (const Type& text, const Type& breakCharacters, const Type& quoteCharacters)
    {
        Type t (text);
        zgui_wchar currentQuoteChar = 0;

        while (! t.isEmpty())
        {
            const zgui_wchar c = t.getAndAdvance();

            if (currentQuoteChar == 0 && breakCharacters.indexOf (c) >= 0)
            {
                --t;
                break;
            }

            if (quoteCharacters.indexOf (c) >= 0)
            {
                if (currentQuoteChar == 0)
                    currentQuoteChar = c;
                else if (currentQuoteChar == c)
                    currentQuoteChar = 0;
            }
        }

        return t;
    }
};


#endif // __ZGUI_CHARACTERFUNCTIONS_H_
