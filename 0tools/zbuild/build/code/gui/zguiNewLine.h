#ifndef __JUCE_NEWLINE_JUCEHEADER__
#define __JUCE_NEWLINE_JUCEHEADER__


//==============================================================================
/** This class is used for represent a new-line character sequence.

    To write a new-line to a stream, you can use the predefined 'newLine' variable, e.g.
    @code
    myOutputStream << "Hello World" << newLine << newLine;
    @endcode

    The exact character sequence that will be used for the new-line can be set and
    retrieved with OutputStream::setNewLineString() and OutputStream::getNewLineString().
*/
class NewLine
{
public:
    /** Returns the default new-line sequence that the library uses.
        @see OutputStream::setNewLineString()
    */
    static const char* getDefault() throw()        { return "\r\n"; }

    /** Returns the default new-line sequence that the library uses.
        @see getDefault()
    */
    operator String() const                         { return getDefault(); }
};

//==============================================================================
/** A predefined object representing a new-line, which can be written to a string or stream.

    To write a new-line to a stream, you can use the predefined 'newLine' variable like this:
    @code
    myOutputStream << "Hello World" << newLine << newLine;
    @endcode
*/
extern NewLine newLine;

//==============================================================================
/** Writes a new-line sequence to a string.
    You can use the predefined object 'newLine' to invoke this, e.g.
    @code
    myString << "Hello World" << newLine << newLine;
    @endcode
*/
String& __stdcall operator<< (String& string1, const NewLine&);


#endif   // __JUCE_NEWLINE_JUCEHEADER__
