#ifndef __ZGUI_DIRECTORYITERATOR_H_
#define __ZGUI_DIRECTORYITERATOR_H_

namespace zgui
{

class DirectoryIterator
{
public:
    enum TypesOfFileToFind {
        findDirectories= 1,
        findFiles,
        findFilesAndDirectories,
        ignoreHiddenFiles
    };

    DirectoryIterator(const String& directory, bool isRecursive, const String& wildCard = "*", int whatToLookFor = findFiles);
    ~DirectoryIterator();

    bool next();
    bool next(bool* isDirectory, bool* isHidden, int64_t* fileSize, bool* isReadOnly);

    const String& getFile() const;

    static String addTrailingSeparator(const String& path);

private:
    bool nextImpl(String& filenameFound, bool* isDirectory, bool* isHidden, int64_t* fileSize, bool* isReadOnly);

    StringArray _wildCards;
    const String _directoryWithWildCard;
    HANDLE _handle;
    String _wildCard;
    String _path;
    const int _whatToLookFor;
    const bool _isRecursive;
    bool _hasBeenAdvanced;
    DirectoryIterator* _pSubIterator;
    String _currentFile;

    static StringArray parseWildcards(const String& pattern);
    static bool fileMatches(const StringArray& wildCards, const String& filename);
private:
    DirectoryIterator(const DirectoryIterator&);
    const DirectoryIterator& operator=(const DirectoryIterator&);
};

}

#endif // __ZGUI_DIRECTORYITERATOR_H_
