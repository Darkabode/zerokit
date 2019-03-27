#include "zgui.h"

namespace zgui
{

DirectoryIterator::DirectoryIterator(const String& directory, bool recursive, const String& wildCard, const int type) :
_wildCards(parseWildcards(wildCard)),
_path(addTrailingSeparator(directory)),
_directoryWithWildCard(addTrailingSeparator(directory) + ((recursive || _wildCards.size() > 1) ? "*" : wildCard)),
_handle(INVALID_HANDLE_VALUE),
_wildCard(wildCard),
_whatToLookFor(type),
_isRecursive(recursive),
_hasBeenAdvanced(false),
_pSubIterator(0)
{
}

DirectoryIterator::~DirectoryIterator()
{
    if (_handle != INVALID_HANDLE_VALUE) {
        fn_FindClose(_handle);
    }
}

StringArray DirectoryIterator::parseWildcards(const String& pattern)
{
    StringArray s;
    s.addTokens(pattern, ";,", "\"'");
    s.trim();
    s.removeEmptyStrings();
    return s;
}

String DirectoryIterator::addTrailingSeparator(const String& path)
{
    return path.endsWithChar(L'\\') ? path : (path + L'\\');
}

bool DirectoryIterator::fileMatches(const StringArray& wildCards, const String& filename)
{
    for (int i = 0; i < wildCards.size(); ++i) {
        if (filename.matchesWildcard(wildCards[i], true)) {
            return true;
        }
    }

    return false;
}

bool DirectoryIterator::next()
{
    return next(0, 0, 0, 0);
}

bool DirectoryIterator::next(bool* const pIsDirResult, bool* const pIsHiddenResult, int64_t* const pFileSize, bool* const pIsReadOnly)
{
    _hasBeenAdvanced = true;

    if (_pSubIterator != 0) {
        if (_pSubIterator->next(pIsDirResult, pIsHiddenResult, pFileSize, pIsReadOnly)) {
            return true;
        }

        delete _pSubIterator;
        _pSubIterator = 0;
    }

    String filename;
    bool isDirectory, isHidden = false;

    while (nextImpl(filename, &isDirectory, (pIsHiddenResult != 0 || (_whatToLookFor & ignoreHiddenFiles) != 0) ? &isHidden : 0, pFileSize, pIsReadOnly)) {
        if (!filename.containsOnly(".")) {
            bool matches = false;

            if (isDirectory) {
                if (_isRecursive && ((_whatToLookFor & ignoreHiddenFiles) == 0 || !isHidden)) {
                    _pSubIterator = new DirectoryIterator(_path + filename, true, _wildCard, _whatToLookFor);
                }

                matches = (_whatToLookFor & findDirectories) != 0;
            }
            else {
                matches = (_whatToLookFor & findFiles) != 0;
            }

            // if we're not relying on the OS iterator to do the wildcard match, do it now..
            if (matches && (_isRecursive || _wildCards.size() > 1)) {
                matches = fileMatches(_wildCards, filename);
            }

            if (matches && (_whatToLookFor & ignoreHiddenFiles) != 0) {
                matches = !isHidden;
            }

            if (matches) {
                _currentFile = _path + filename;
                if (pIsHiddenResult != 0) {
                    *pIsHiddenResult = isHidden;
                }
                if (pIsDirResult != 0) {
                    *pIsDirResult = isDirectory;
                }

                return true;
            }

            if (_pSubIterator != 0) {
                return next(pIsDirResult, pIsHiddenResult, pFileSize, pIsReadOnly);
            }
        }
    }

    return false;
}

const String& DirectoryIterator::getFile() const
{
    if (_pSubIterator != 0 && _pSubIterator->_hasBeenAdvanced) {
        return _pSubIterator->getFile();
    }

    return _currentFile;
}

bool DirectoryIterator::nextImpl(String& filenameFound, bool* const pIsDir, bool* const pIsHidden, int64_t* const pFileSize, bool* const pIsReadOnly)
{
    WIN32_FIND_DATAW findData;

    if (_handle == INVALID_HANDLE_VALUE) {
        _handle = fn_FindFirstFileW(_directoryWithWildCard.toWideCharPointer(), &findData);

        if (_handle == INVALID_HANDLE_VALUE) {
            return false;
        }
    }
    else {
        if (fn_FindNextFileW(_handle, &findData) == 0) {
            return false;
        }
    }

    filenameFound = findData.cFileName;

    if (pIsDir != 0) {
        *pIsDir = ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
    }
    if (pIsHidden != 0) {
        *pIsHidden = ((findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0);
    }
    if (pIsReadOnly != 0) {
        *pIsReadOnly = ((findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0);
    }
    if (pFileSize != 0) {
        *pFileSize = findData.nFileSizeLow + (((int64_t)findData.nFileSizeHigh) << 32);
    }

    return true;
}

}