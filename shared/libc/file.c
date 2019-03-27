#include <Windows.h>
#include <stdio.h>
#include "libct.h"

extern HANDLE gHeap;

#undef malloc
#define malloc(sz) HeapAlloc(gHeap, 0, sz)

#undef free
#define free(ptr) HeapFree(gHeap, 0, ptr)


/* FILE, as defined in stdio.h
struct _iobuf {
        char *_ptr;
        int   _cnt;
        char *_base;				Used to store HANDLE
        int   _flag;
        int   _file;
        int   _charbuf;
        int   _bufsiz;
        char *_tmpfname;
        };
typedef struct _iobuf FILE;
*/

//_flag values (not the ones used by the normal CRT
#define _FILE_TEXT		0x0001
#define _FILE_EOF		0x0002
#define _FILE_ERROR		0x0004

// struct _FILE : public FILE
// {
// 	void set_handle(HANDLE h) {_base = (char*)h;};
// 	HANDLE get_handle() const {return (HANDLE)_base;};
// };

// used directly by the stdin, stdout, and stderr macros
FILE __iob[3];
FILE* __cdecl __iob_func() {
    return (FILE*)__iob;
}

void _init_file()
{
	// STDIN
	__iob[0]._base = GetStdHandle(STD_INPUT_HANDLE);
	__iob[0]._flag = _FILE_TEXT;

	// STDOUT
	__iob[1]._base = GetStdHandle(STD_OUTPUT_HANDLE);
	__iob[1]._flag = _FILE_TEXT;

	// STDERR
	__iob[2]._base =GetStdHandle(STD_ERROR_HANDLE);
	__iob[2]._flag = _FILE_TEXT;
}


/*int _fileno(FILE *fp)
{
	return (int)fp;			// FIXME:  This doesn't work under Win64
}

HANDLE _get_osfhandle(int i)
{
	return (HANDLE)i;		// FIXME:  This doesn't work under Win64
}*/

FILE* __cdecl fopen(const char *path, const char *attrs)
{
    FILE *file;
    HANDLE hFile;
	DWORD access, disp;
	if (strchr(attrs, 'w'))
	{
		access = GENERIC_WRITE;
		disp = CREATE_ALWAYS;
	}
	else
	{
		access = GENERIC_READ;
		disp = OPEN_EXISTING;
	}

	hFile = CreateFileA(path, access, 0, 0, disp, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return 0;

	file = malloc(sizeof(FILE));
	__stosb(file, 0, sizeof(FILE));
	file->_base = hFile;

    if (strchr(attrs, 't')) {
		file->_flag |= _FILE_TEXT;
    }

	return file;
}

// FILE* __cdecl _wfopen(const wchar_t *path, const wchar_t *attrs)
// {
//     HANDLE hFile;
// 	DWORD access, disp;
//     FILE* file;
// 
// 	if (wcschr(attrs, L'w')) {
// 		access = GENERIC_WRITE;
// 		disp = CREATE_ALWAYS;
// 	}
// 	else
// 	{
// 		access = GENERIC_READ;
// 		disp = OPEN_EXISTING;
// 	}
// 
// 	hFile = CreateFileW(path, access, 0, 0, disp, 0, 0);
//     if (hFile == INVALID_HANDLE_VALUE) {
// 		return 0;
//     }
// 
// 	file = malloc(sizeof(FILE));
// 	__stosb(file, 0, sizeof(FILE));
// 	file->_base = hFile;
// 
//     if (wcschr(attrs, L't')) {
// 		file->_flag |= _FILE_TEXT;
//     }
// 
// 	return file;
// }


int __cdecl fprintf(FILE *fp, const char *s, ...)
{
	va_list args;
    char bfr[1024];
    int len;

	va_start(args, s);
	
	len = wvsprintfA(bfr, s, args);
	va_end(args);

	fwrite(bfr, len+1, sizeof(char), fp);
	return len;
}

int __cdecl vfprintf(FILE *fp, const char *s, va_list args)
{
    char bfr[1024];
    int len;

    len = wvsprintfA(bfr, s, args);

    fwrite(bfr, len+1, sizeof(char), fp);
    return len;
}
// 
// int __cdecl fwprintf(FILE *fp, const wchar_t *s, ...)
// {
// 	va_list args;
//     wchar_t bfr[1024];
//     int len;
//     char ansibfr[1024];
// 
// 	va_start(args, s);
// 	
// 	len = wvsprintfW(bfr, s, args);
// 
// 	va_end(args);
// 	
// 	WideCharToMultiByte(CP_ACP, 0, bfr, -1, ansibfr, sizeof(ansibfr), 0, 0);
// 
// 	fwrite(ansibfr, len+1, sizeof(char), fp);
// 	return len;
// }


int __cdecl fclose(FILE *fp)
{
	CloseHandle(fp->_base);
	free(fp);
	return 0;
}

int __cdecl feof(FILE *fp)
{
	return (fp->_flag & _FILE_EOF) ? 1 : 0;
}

int __cdecl fseek(FILE *str, long offset, int origin)
{
	DWORD meth = FILE_BEGIN;
	if (origin == SEEK_CUR)
		meth = FILE_CURRENT;
	else if (origin == SEEK_END)
		meth = FILE_END;
	SetFilePointer(str->_base, offset, 0, meth);
	str->_flag &= ~_FILE_EOF;
	return 0;
}

long __cdecl ftell(FILE *fp)
{
	return SetFilePointer(fp->_base, 0, 0, FILE_CURRENT);
}

size_t __cdecl fread(void *buffer, size_t size, size_t count, FILE *str)
{
    HANDLE hFile;
    int textMode;
    char *src;
    DWORD br;

	if (size*count == 0)
		return 0;
	if (feof(str))
		return 0;

	hFile = str->_base;
	textMode = str->_flag & _FILE_TEXT;

	if (textMode)
		src = (char*)malloc(size*count);
	else
		src = (char*)buffer;

	if (!ReadFile(hFile, src, (DWORD)(size*count), &br, 0))
		str->_flag |= _FILE_ERROR;
	else if (!br)		// nonzero return value and no bytes read = EOF
		str->_flag |= _FILE_EOF;

	if (!br)
		return 0;

	// Text-mode translation is always ANSI
    if (textMode) { // text mode: must translate CR -> LF
		char *dst = (char*)buffer;
        DWORD i;
		
        for (i = 0; i < br; i++) {
			if (src[i] != '\r')
			{
				*dst++ = src[i];
				continue;
			}

			// If next char is LF -> convert CR to LF
			if (i+1 < br)
			{
				if (src[i+1] == '\n')
				{
					*dst++ = '\n';
					i++;
				}
				else
					*dst++ = src[i];
			}
			else if (br > 1)
			{
				// This is the hard part: must peek ahead one byte
				DWORD br2 = 0;
				char peekChar = 0;
				ReadFile(hFile, &peekChar, 1, &br2, 0);
				if (!br2)
					*dst++ = src[i];
				else if (peekChar == '\n')
					*dst++ = '\n';
				else
				{
					fseek(str, -1, SEEK_CUR);
					*dst++ = src[i];
				}
			}
			else
				*dst++ = src[i];
		}

		free(src);
	}

	return br/size;
}

size_t __cdecl fwrite(const void *buffer, size_t size, size_t count, FILE *str)
{
	DWORD bw = 0, bw2 = 0;
    HANDLE hFile;
    int textMode;

	if (size*count == 0)
		return 0;

	hFile = str->_base;
	textMode = str->_flag & _FILE_TEXT;

	// Text-mode translation is always ANSI!
	if (textMode)			// text mode -> translate LF -> CRLF
	{
		const char *src = (const char*)buffer;
		size_t startpos = 0, i = 0;
		for (i = 0; i < size*count; i++) {
            const char *crlf;

			if (src[i] != '\n')
				continue;
			if (i > 0 && src[i-1] == '\r')		// don't translate CRLF
				continue;

			if (i > startpos)
			{
				WriteFile(hFile, &src[startpos], i-startpos, &bw2, 0);
				bw += bw2;
			}

			crlf = "\r\n";
			WriteFile(hFile, crlf, 2, &bw2, 0);
			bw++;		// one '\n' written

			startpos = i+1;
		}

		if (i > startpos)
		{
			WriteFile(hFile, &src[startpos], i-startpos, &bw2, 0);
			bw += bw2;
		}
	}
	else
		WriteFile(hFile, buffer, (DWORD)(size*count), &bw, 0);
	return bw/size;
}

char* __cdecl fgets(char *str, int n, FILE *s)
{
    int i;

	if (feof(s))
		return 0;

	for (i = 0; i < n-1; i++)
	{
		if (!fread(&str[i], 1, sizeof(char), s))
			break;
		if (str[i] == '\r')
		{
			i--;
			continue;
		}
		if (str[i] == '\n')
		{
			i++;
			break;
		}
	}

	str[i] = 0;
	return str;
}
// 
// wchar_t* __cdecl fgetws(wchar_t *str, int n, FILE *s)
// {
// 	int i;
//     // Text-mode fgetws converts MBCS->Unicode
// 	if (s->_flag & _FILE_TEXT)
// 	{
// 		char *bfr = (char*)malloc(n);
// 		fgets(bfr, n, s);
// 		MultiByteToWideChar(CP_ACP, 0, bfr, -1, str, n);
// 		free(bfr);
// 		return str;
// 	}
// 
// 	// Binary fgetws reads as Unicode
// 
// 	if (feof(s))
// 		return 0;
// 
// 	for (i = 0; i < n-1; i++)
// 	{
// 		if (!fread(&str[i], 1, sizeof(wchar_t), s))
// 			break;
// 		if (str[i] == L'\r')
// 		{
// 			i--;
// 			continue;	// does i++
// 		}
// 		if (str[i] == L'\n')
// 		{
// 			i++;
// 			break;
// 		}
// 	}
// 
// 	str[i] = 0;
// 	return str;
// }

int __cdecl fgetc(FILE *s)
{
    char c;

	if (s == 0 || feof(s))
		return EOF;

	fread(&c, 1, sizeof(char), s);

	return (int)c;
}

int __cdecl fputc(int ch, FILE *s)
{
    char c;

    if (s == 0 || feof(s))
        return EOF;

    fwrite(&c, 1, sizeof(char), s);

    return (int)c;
}

// wint_t __cdecl fgetwc(FILE *s)
// {
//     wint_t c;
// 
// 	if (s == 0 || feof(s))
// 		return (wint_t)EOF;
// 
// 	// text-mode fgetwc reads and converts MBCS
// 	if (s->_flag & _FILE_TEXT)
// 	{
// 		char ch = (char)fgetc(s);
// 		wint_t wch;
// 		
//         MultiByteToWideChar(CP_ACP, 0, &ch, 1, (LPWSTR)&wch, 1);
// 		return wch;
// 	}
// 
// 	// binary fgetwc reads unicode
// 
// 	fread(&c, 1, sizeof(wint_t), s);
// 
// 	return c;
// }

int __cdecl fflush(FILE* stream)
{
    return (FlushFileBuffers(stream->_base) ? 0 : 1);
}

int __cdecl fileno(FILE* stream)
{
    //_VALIDATE_RETURN((stream != NULL), EINVAL, -1);
    return stream->_file;
}