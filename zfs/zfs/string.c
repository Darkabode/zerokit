// void zfs_toupper(char* string, uint32_t strLen)
// {
// 	uint32_t i;
// 	for (i = 0; i < strLen; i++) {
//         if (string[i] >= 'a' && string[i] <= 'z') {
// 			string[i] -= 32;
//         }
//         if (string[i] == '\0') {
// 			break;
//         }
// 	}
// }

void zfs_tolower(char* string, uint32_t strLen)
{
	uint32_t i;
	for (i = 0; i < strLen; i++) {
        if (string[i] >= 'A' && string[i] <= 'Z') {
			string[i] += 32;
        }
        if (string[i] == '\0') {
			break;
        }
	}
}

char zfs_strmatch(const char* str1, const char* str2, uint16_t len)
{
	uint16_t i;
	char char1, char2;
    USE_GLOBAL_BLOCK

	if (!len) {
		if (STRLEN(str1) != STRLEN(str2)) {
			return FALSE;
		}
		len = (uint16_t)STRLEN(str1);
	}
	
	for (i = 0; i < len; i++) {
		char1 = str1[i];
		char2 = str2[i];
		if (char1 >= 'A' && char1 <= 'Z') {
			char1 += 32;
		}
		if (char2 >= 'A' && char2 <= 'Z') {
			char2 += 32;
		}

		if (char1 != char2) {
			return FALSE;
		}
	}

	return TRUE;
}

char* zfs_strtok(const char* string, char* token, uint16_t *tokenNumber, char* last, uint16_t Length)
{
	uint16_t strLen = Length;
	uint16_t i,y, tokenStart, tokenEnd = 0;
    USE_GLOBAL_BLOCK

	i = 0;
	y = 0;

	if (string[i] == '\\' || string[i] == '/') {
		i++;
	}

	tokenStart = i;

	while (i < strLen) {
		if (string[i] == '\\' || string[i] == '/') {
			y++;
			if (y == *tokenNumber) {
				tokenStart = (uint16_t)(i + 1);
			}
			if (y == (*tokenNumber + 1)) {
				tokenEnd = i;
				break;
			}
		}
		i++;
	}

	if (!tokenEnd) {
		if (*last == TRUE) {
			return NULL;
		}
        else {
            *last = TRUE;
		}
		tokenEnd = i;
	}
	if ((tokenEnd - tokenStart) <= ZFS_MAX_FILENAME) {
		MEMCPY(token, (string + tokenStart), (uint32_t)(tokenEnd - tokenStart));
		token[tokenEnd - tokenStart] = '\0';
	}
    else {
		MEMCPY(token, (string + tokenStart), ZFS_MAX_FILENAME + 1);
		token[ZFS_MAX_FILENAME] = '\0';
	}
	//token[tokenEnd - tokenStart] = '\0';
    *tokenNumber += 1;

	return token;	
}

char zfs_wildcompare(const char* pszWildCard, const char* pszString)
{
    const char* pszWc = NULL;
	const char* pszStr = NULL;	// Encourage the string pointers to be placed in memory.
    do {
        if (*pszWildCard == '*') {
			while (*(1 + pszWildCard++) == '*'); // Eat up multiple '*''s
			pszWc = (pszWildCard - 1);
            pszStr = pszString;
        }

		if (*pszWildCard == '?' && !*pszString) {
			return FALSE;	// False when the string is ended, yet a ? charachter is demanded.
		}
        if (*pszWildCard != '?' && *pszWildCard != *pszString) {
			if (pszWc == NULL) {
				return FALSE;
			}
            pszWildCard = pszWc;
            pszString = pszStr++;
        }
    } while (*pszWildCard++ && *pszString++);

	while (*pszWildCard == '*') {
		pszWildCard++;
	}

	if (!*(pszWildCard - 1)) {	// WildCard is at the end. (Terminated)
		return TRUE;	// Therefore this must be a match.
	}

	return FALSE;	// If not, then return FALSE!
}