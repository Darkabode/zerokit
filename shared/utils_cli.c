#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <memory.h>
#include <errno.h>
#include <fcntl.h>

#ifdef WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#ifdef USE_UTILS_PIPE_LAUNCHING
#include <io.h> 
#endif // USE_UTILS_PIPE_LAUNCHING


#include "types.h"
#include "utils_cli.h"

int utils_read_file(const char* filePath, uint8_t** pData, size_t* pSize)
{
#define READ_BLOCK_SIZE 1 * 1024
    FILE* pFile = NULL;
    size_t fSize, readed, totalRead;
    uint8_t* fileData;
    uint8_t* itr;
    int ret = ERR_BAD;

    *pData = NULL;

    do {
        pFile = fopen(filePath, "rb");
        if (pFile == NULL) {
            break;
        }

        if (fseek(pFile, 0, SEEK_END) != 0 || (fSize = ftell(pFile)) <= 0 || fseek(pFile, 0, SEEK_SET) != 0) {
            break;
        }

        fileData = (uint8_t*)calloc(1, fSize + 1);

        if (fileData == NULL) {
            break;
        }

        itr = fileData;
        readed = totalRead = 0;
        while (!feof(pFile)) {
            readed = fread(itr, 1, READ_BLOCK_SIZE, pFile);
            itr += readed;
            totalRead += readed;
        }

        if (totalRead != fSize) {
            free(fileData);
            break;
        }

        *pData = fileData;
        *pSize = totalRead;

        ret = ERR_OK;
    } while (0);

    if (pFile != NULL) {
        fclose(pFile);
    }

    return ret;
}

int utils_save_file(const char* filePath, const uint8_t* data, size_t size)
{
#define WRITE_BLOCK_SIZE 8 * 1024
    FILE* pFile = NULL;
    size_t remaining, wrSize = 0;
    uint8_t* itr;
    int ret = ERR_BAD;

    do {
        pFile = fopen(filePath, "wb");
        if (pFile == NULL) {
            break;
        }

        remaining = size;
        itr = data;
        while (remaining > 0) {
            wrSize = remaining > WRITE_BLOCK_SIZE ? WRITE_BLOCK_SIZE : remaining;
            wrSize = fwrite(itr, 1, wrSize, pFile);
            remaining -= wrSize;
            itr += wrSize;
        }

        ret = ERR_OK;
    } while (0);

    if (pFile != NULL) {
        fclose(pFile);
    }

    return ret;
}

void utils_md5_to_str(const uint8_t md5Buff[16], char outStr[33])
{
    static char alphas[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    int i;

    for (i = 0; i < 16; ++i) {
        outStr[2 * i] = alphas[(md5Buff[i] >> 4) & 0x0F];
        outStr[2 * i + 1] = alphas[md5Buff[i] & 0x0F];
    }

    outStr[32] = 0;
}

int utils_file_exists(const char* filePath)
{
#ifdef _WIN32
    unsigned long attr = GetFileAttributes(filePath);
    if (attr == 0xFFFFFFFF) {
        return ERR_BAD;
    }

    return ERR_OK;
#else
    struct stat st;
    return (stat(filePath, &st) == 0 ? ERR_OK : ERR_BAD);
#endif
}

const char* utils_get_base_name(const char* filePath)
{
    const char* ptr = filePath + strlen(filePath);

    for ( ; ptr >= filePath && *ptr != '\\' && *ptr != '/'; --ptr);
    return (++ptr);
}

void utils_build_path(char* outPath, char* firstPart, char* secondPart, int replaceFilename)
{
    char* sep;

    memset(outPath, 0, MAX_PATH);
    strcpy(outPath, firstPart);

    sep = &outPath[strlen(outPath) - 1];
    if (*sep != '/' && *sep != '\\') {
        if (replaceFilename) {
            for ( ; *sep != '/' && *sep != '\\'; --sep);
            *(++sep) = '\0';
        }
        else {
            strcat(outPath, "/");
        }
    }
    strcat(outPath, secondPart);
}

void* utils_launch(char* commandLine)
{
#ifdef _WIN32
    HANDLE hProc;
    STARTUPINFO startupInfo;
    PROCESS_INFORMATION processInfo;
    BOOL rc;

    GetStartupInfo(&startupInfo); // take defaults from current process
    startupInfo.cb          = sizeof(STARTUPINFO);
    startupInfo.lpReserved  = NULL;
    startupInfo.lpDesktop   = NULL;
    startupInfo.lpTitle     = NULL;
    startupInfo.dwFlags     = STARTF_FORCEOFFFEEDBACK | STARTF_USESTDHANDLES;
    startupInfo.cbReserved2 = 0;
    startupInfo.lpReserved2 = NULL;

    hProc = GetCurrentProcess();
    DuplicateHandle(hProc, GetStdHandle(STD_INPUT_HANDLE), hProc, &startupInfo.hStdInput, 0, TRUE, DUPLICATE_SAME_ACCESS);
    DuplicateHandle(hProc, GetStdHandle(STD_OUTPUT_HANDLE), hProc, &startupInfo.hStdOutput, 0, TRUE, DUPLICATE_SAME_ACCESS);
    DuplicateHandle(hProc, GetStdHandle(STD_ERROR_HANDLE), hProc, &startupInfo.hStdError, 0, TRUE, DUPLICATE_SAME_ACCESS);
    
    rc = CreateProcess(NULL, commandLine, NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &processInfo);
    CloseHandle(startupInfo.hStdInput);
    CloseHandle(startupInfo.hStdOutput);
    CloseHandle(startupInfo.hStdError);

    return (void*)processInfo.hProcess;
#else
//    int pid = fork();
//    if (pid < 0) {
//        return ERR_BAD;
//    }
//    else if (pid == 0) {
//        // close all open file descriptors other than stdin, stdout, stderr
//        for (int i = 3; i < getdtablesize(); ++i)
//            close(i);
//            
//        char** argv = malloc(sizeof(char*) * (args.size() + 2));
//        int i = 0;
//        argv[i++] = (char*)(commandLine);
//        for (ArgsImpl::const_iterator it = args.begin(); it != args.end(); ++it) 
//            argv[i++] = const_cast<char*>(it->c_str());
//        argv[i] = NULL;
//        execvp(commandLine, argv);
//        _exit(72);
//    }
    return NULL;
#endif // _WIN32
}

#ifdef _WIN32

bool_t utils_launch_and_verify(const char* commandLine, uint32_t* pExitCode)
{
    void* hProcess;
    DWORD exitCode;

    // Запускаем команду.
    hProcess = utils_launch(commandLine);
    if (hProcess == NULL) {
        return FALSE;
    }

    WaitForSingleObject((HANDLE)hProcess, INFINITE);
    GetExitCodeProcess((HANDLE)hProcess, &exitCode);
    CloseHandle(hProcess);

    if (pExitCode != NULL) {
        *pExitCode = exitCode;
    }

    if (exitCode != 0) {
        return FALSE;
    }

    return TRUE;
}

#endif // #if _WIN32

int utils_create_directory(char* dir)
{
#ifdef _WIN32
    if (CreateDirectoryA(dir, 0) == 0)
        return ERR_BAD;
#else
    if (mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) 
        return ERR_BAD;
#endif // _WIN32
    return ERR_OK;

}

uint64_t utils_get_file_size(const char* fileName)
{
#ifdef _WIN32
    uint64_t size;
    BOOL ret;
    WIN32_FILE_ATTRIBUTE_DATA fdata;

    ret = GetFileAttributesExA(fileName, GetFileExInfoStandard, &fdata);
    size = fdata.nFileSizeLow;
    size |= (uint64_t)fdata.nFileSizeHigh << 32;

    if (ret) {
        return size;
    }
#else
    struct stat st;
    if (stat(fileName, &st) == 0)
        return (uint64_t)st.st_size;
#endif // _WIN32

    return 0;
}


int utils_remove(const char* pathName)
{
#ifdef _WIN32
    DWORD attr = GetFileAttributes(pathName);
    if (attr == 0xFFFFFFFF)
        return ERR_BAD;

    if ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        return (RemoveDirectoryA(pathName) == 0) ? ERR_BAD : ERR_OK;
    }
    else {
        return (DeleteFileA(pathName) == 0) ? ERR_BAD : ERR_OK;
    }
#else
    int rc;
    struct stat st;

    if (stat(pathName, &st) != 0)
        return ERR_BAD;

    if (S_ISDIR(st.st_mode))
        rc = rmdir(pathName);
    else
        rc = unlink(pathName);
    return rc ? ERR_BAD : ERR_OK;
#endif // _WIN32
}

int utils_copy_file(const char* srcPath, const char* destPath)
{
#ifdef _WIN32
    if (CopyFileA(srcPath, destPath, FALSE) != 0) {
        return ERR_OK;
    }
    return ERR_BAD;
#else
    return 4;
#endif // _WIN32
}

int utils_set_current_directory(const char* path)
{
#ifdef _WIN32
    return (SetCurrentDirectoryA(path) == TRUE) ? ERR_OK : ERR_BAD;
#else
    return 0;
#endif // _WIN32
}

#ifdef USE_UTILS_PIPE_LAUNCHING

HANDLE my_pipein[2], my_pipeout[2], my_pipeerr[2];
char my_popenmode = ' ';

int my_pipe(HANDLE *readwrite)
{
    SECURITY_ATTRIBUTES sa;

    sa.nLength = sizeof(sa);
    sa.bInheritHandle = 1;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&readwrite[0],&readwrite[1],&sa,1 << 13)) {
        errno = EMFILE;
        return -1;
    }

    return 0;
}

FILE* utils_plaunch(const char* cmd, const char* mode)
{
    FILE* fptr = NULL;
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    int success;

    my_pipein[0] = INVALID_HANDLE_VALUE;
    my_pipein[1] = INVALID_HANDLE_VALUE;
    my_pipeout[0] = INVALID_HANDLE_VALUE;
    my_pipeout[1] = INVALID_HANDLE_VALUE;
    my_pipeerr[0] = INVALID_HANDLE_VALUE;
    my_pipeerr[1] = INVALID_HANDLE_VALUE;

    if (!mode || !*mode) {
        goto exit;
    }

    my_popenmode = *mode;
    if (my_popenmode != 'r' && my_popenmode != 'w') {
        goto exit;
    }

    if (my_pipe(my_pipein)  == -1 || my_pipe(my_pipeout) == -1) {
        goto exit;
    }
    
    if (my_pipe(my_pipeerr) == -1) {
        goto exit;
    }

    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb           = sizeof(STARTUPINFO);
    siStartInfo.hStdInput    = my_pipein[0];
    siStartInfo.hStdOutput   = my_pipeout[1];
    siStartInfo.hStdError  = my_pipeout[1];
    siStartInfo.dwFlags    = STARTF_USESTDHANDLES;

    success = CreateProcess(NULL,
        (LPTSTR)cmd,       // command line 
        NULL,              // process security attributes 
        NULL,              // primary thread security attributes 
        TRUE,              // handles are inherited 
        DETACHED_PROCESS,  // creation flags: Ohne Fenster (?)
        NULL,              // use parent's environment 
        NULL,              // use parent's current directory 
        &siStartInfo,      // STARTUPINFO pointer 
        &piProcInfo);      // receives PROCESS_INFORMATION 

    if (!success) {
        goto exit;
    }

    CloseHandle(my_pipein[0]);  my_pipein[0]  = INVALID_HANDLE_VALUE;
    CloseHandle(my_pipeout[1]); my_pipeout[1] = INVALID_HANDLE_VALUE;
    CloseHandle(my_pipeerr[1]); my_pipeerr[1] = INVALID_HANDLE_VALUE;

    if (my_popenmode == 'r') {
        fptr = _fdopen(_open_osfhandle((long)my_pipeout[0], _O_BINARY), "r");
    }
    else {
        fptr = _fdopen(_open_osfhandle((long)my_pipein[1], _O_BINARY), "w");
    }

exit:
    if (!fptr) {
    if (my_pipein[0]  != INVALID_HANDLE_VALUE)
        CloseHandle(my_pipein[0]);
    if (my_pipein[1]  != INVALID_HANDLE_VALUE)
        CloseHandle(my_pipein[1]);
    if (my_pipeout[0] != INVALID_HANDLE_VALUE)
        CloseHandle(my_pipeout[0]);
    if (my_pipeout[1] != INVALID_HANDLE_VALUE)
        CloseHandle(my_pipeout[1]);
    if (my_pipeerr[0] != INVALID_HANDLE_VALUE)
        CloseHandle(my_pipeerr[0]);
    if (my_pipeerr[1] != INVALID_HANDLE_VALUE)
        CloseHandle(my_pipeerr[1]);
    }
    return fptr;
}

int utils_pdestroy(FILE *fle)
{
    if (fle) {
        fclose(fle);

        CloseHandle(my_pipeerr[0]);
        if (my_popenmode == 'r') {
            CloseHandle(my_pipein[1]);
        }
        else {
            CloseHandle(my_pipeout[0]);
        }
        return 0;
    }
    return -1;
}

#endif // USE_UTILS_PIPE_LAUNCHING
