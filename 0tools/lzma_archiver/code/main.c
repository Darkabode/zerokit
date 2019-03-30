#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#define LZMARCH_VERSION "0.4.1"

#include "../../../shared/platform.h"
#include "../../../shared/native.h"
#include "../../../shared/types.h"
#include "../../../shared/config.h"
#include "../../../shared/utils_cli.h"
#include "../../../shared/lzma.h"

#define USE_LZMA_COMPRESSOR 1
#include "../../../shared/lzma.c"

#include "lzma_archiver.h"
#include "lzma_archiver.c"

typedef struct cmd_options
{
    char* inFile;
    char* outFile;
    int force;
    int remove;
} cmd_options_t, *pcmd_options_t;

cmd_options_t gCmdOptions;

cmd_line_info_t cmdLineInfo = {4,
{
    {{"i", "in"}, "Path to input file", "Specify nonempty path", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.inFile},
    {{"o", "out"}, "Path to archive file", "Specify nonempty path", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.outFile},
    {{"f", "force"}, "Force compress entry", "", OPT_FLAG_OPTIONAL, TYPE_CMDLINE | TYPE_BOOLEAN, (void*)&gCmdOptions.force},
    {{"r", "remove"}, "Remove entry from archive", "", OPT_FLAG_OPTIONAL, TYPE_CMDLINE | TYPE_BOOLEAN, (void*)&gCmdOptions.remove},
}
};

unsigned int compress_entry(uint8_t* inData, size_t inSize, uint8_t** pOutData, size_t* pOutSize, int force)
{
    int ret;
    unsigned int compressed = 0;
    CLzmaEncProps props;
    uint32_t outSize;
    uint8_t* outData;
    unsigned propsSize = LZMA_PROPS_SIZE;

    *pOutData = NULL;

    LzmaEncProps_Init(&props);
    props.dictSize = 1048576; // 1 MB

    outSize = inSize + inSize/3 + 128 - LZMA_PROPS_SIZE;
    outData = malloc(outSize);

    ret = LzmaEncode(&outData[LZMA_PROPS_SIZE], &outSize, inData, inSize, &props, outData, &propsSize);

    if (ret != ERR_OK) {
        return 0;
    }

    if ((outSize + propsSize) < inSize || force) {
        compressed = LAF_ENTRY_COMPRESSED;
        *pOutSize = outSize + propsSize;

        free(inData);
        *pOutData = outData;
    }
    else {
        free(outData);
        *pOutData = inData;
        *pOutSize = inSize;
    }

    return compressed;
}

int __cdecl main(int argc, char** argv)
{
    int ret = EXIT_FAILURE;
    uint8_t* fData = NULL;
    size_t fSize = 0;
    uint8_t* archBuffer = NULL;
    size_t archSize;
    plzma_arch_header_t pArchHeader;
    unsigned int entryOffset;
    unsigned int entrySize = 0, entryFlags = 0;
    size_t enLen;
    const char* newEntryName;

    memset(&gCmdOptions, 0, sizeof(gCmdOptions));

    if (config_parse_cmdline(argc, argv, &cmdLineInfo, LZMARCH_VERSION) != 0) {
        return ret; 
    }

    newEntryName = utils_get_base_name(gCmdOptions.inFile);

    if (utils_file_exists(gCmdOptions.outFile) == ERR_OK) {
        if (utils_read_file(gCmdOptions.outFile, &archBuffer, &archSize) != ERR_OK) {
            printf("Cannot read %s\n", gCmdOptions.outFile);
            goto exit;
        }

        pArchHeader = (plzma_arch_header_t)archBuffer;
        if (pArchHeader->signature != 0x79977997 || pArchHeader->totalSize != archSize) {
            printf("Incorrect archive signature or archive is currupted\n");
            goto exit;
        }

        entryOffset = lzma_arch_get_entry_offset(pArchHeader, newEntryName, &entrySize, &entryFlags);
        if (entryOffset != (unsigned int)-1) {
            // Элемент с таким именем найден.

            if (gCmdOptions.remove) {
                // Удаляем элемент.
                char* nextEntry = (char*)pArchHeader + entryOffset + strlen((char*)pArchHeader + entryOffset) + 1 + 2*sizeof(unsigned int) + entrySize;
                memcpy((char*)pArchHeader + entryOffset, nextEntry, archSize - (nextEntry - (char*)pArchHeader));
                pArchHeader->totalSize -= entrySize + strlen((char*)pArchHeader + entryOffset) + 1 + 2*sizeof(unsigned int);
            }
            else {
                // Заменяем элемент.
                unsigned int oldSize = *(unsigned int*)((char*)pArchHeader + entryOffset + strlen((char*)pArchHeader + entryOffset) + 1) + 2 * sizeof(unsigned int) + strlen((char*)pArchHeader + entryOffset) + 1;
                unsigned int newSize;

                if (utils_read_file(gCmdOptions.inFile, &fData, &fSize) != ERR_OK) {
                    printf("Cannot read %s\n", gCmdOptions.inFile);
                    goto exit;
                }

                entryFlags = compress_entry(fData, fSize, &fData, &fSize, gCmdOptions.force);
                if (fData == NULL) {
                    printf( "Cannot compress pack\n\n");
                    goto exit;
                }

                newSize = fSize + 2 * sizeof(unsigned int) + strlen(newEntryName) + 1;

                if (oldSize == newSize && memcmp(fData, (char*)pArchHeader + entryOffset + strlen((char*)pArchHeader + entryOffset) + 1 + 2 * sizeof(unsigned int), fSize) == 0) {
                    printf("File already in archive\n");
                }
                else {
                    plzma_arch_header_t pNewBuffer;
                    unsigned int remainSize = pArchHeader->totalSize - (entryOffset + oldSize);
                    char* remainEntries = NULL;
                    
                    if (remainSize > 0) {
                        remainEntries = (char*)malloc(remainSize);
                        // Сохраняем все элементы, находящиеся после заменяемого.
                        memcpy(remainEntries, (char*)pArchHeader + entryOffset + oldSize, remainSize);
                    }
                    pNewBuffer = malloc(entryOffset + newSize + remainSize);
                    if (pNewBuffer == NULL) {
                        printf("Cannot allocate memory for new entry\n");
                        goto exit;
                    }
                    memcpy(pNewBuffer, (char*)pArchHeader, entryOffset);
                    //memcpy((char*)pNewBuffer + entryOffset, fData, entryOffset);
                    enLen = strlen(newEntryName) + 1;
                    memcpy((char*)pNewBuffer + entryOffset, newEntryName, enLen);
                    *(unsigned int*)((char*)pNewBuffer + entryOffset + enLen) = (unsigned int)fSize;
                    *(unsigned int*)((char*)pNewBuffer + entryOffset + enLen + sizeof(unsigned int)) = entryFlags;
                    memcpy((char*)pNewBuffer + entryOffset + enLen + 2 * sizeof(unsigned int), fData, fSize);
                    if (remainSize > 0) {
                        memcpy((char*)pNewBuffer + entryOffset + enLen + 2 * sizeof(unsigned int) + fSize, remainEntries, remainSize);
                    }

                    pArchHeader = pNewBuffer;
                    pArchHeader->totalSize -= oldSize;
                    pArchHeader->totalSize += newSize;
                }
            }

            if (utils_save_file(gCmdOptions.outFile, (const uint8_t*)pArchHeader, pArchHeader->totalSize) != ERR_OK) {
                printf("Cannot save %s\n", gCmdOptions.outFile);
                goto exit;
            }

            ret = EXIT_SUCCESS;
            goto exit;
        }
    }

//    archBuffer = realloc(archBuffer, )
    if (gCmdOptions.inFile == NULL) {
        printf("You must specify file name to be archived\n");
        goto exit;
    }

    if (utils_read_file(gCmdOptions.inFile, &fData, &fSize) != ERR_OK) {
        printf("Cannot read %s\n", gCmdOptions.inFile);
        goto exit;
    }

    entryFlags = compress_entry(fData, fSize, &fData, &fSize, gCmdOptions.force);
    if (fData == NULL) {
        printf( "Cannot compress pack\n\n");
        goto exit;
    }

    enLen = strlen(newEntryName) + 1;

    if (archBuffer == NULL) {
        entrySize = sizeof(lzma_arch_header_t) + enLen + 2 * sizeof(unsigned int) + fSize;
        pArchHeader = (plzma_arch_header_t)malloc(entrySize);
        pArchHeader->signature = 0x79977997;
        pArchHeader->numFiles = 1;
        pArchHeader->totalSize = entrySize;
    }
    else {
        plzma_arch_header_t pNewBuffer;

        entrySize = enLen + 2 * sizeof(unsigned int) + fSize;
        ++pArchHeader->numFiles;
        pArchHeader->totalSize += entrySize;
        pNewBuffer = realloc(pArchHeader, pArchHeader->totalSize);
        if (pNewBuffer == NULL) {
            printf("Cannot reallocate memory for new entry\n");
            goto exit;
        }
        pArchHeader = pNewBuffer;
        entrySize = pArchHeader->totalSize;
    }

    memcpy(((char*)pArchHeader + entrySize - fSize - enLen - 2 * sizeof(unsigned int)), newEntryName, enLen);
    *(unsigned int*)((char*)pArchHeader + entrySize - fSize - 2 * sizeof(unsigned int)) = (unsigned int)fSize;
    *(unsigned int*)((char*)pArchHeader + entrySize - fSize - sizeof(unsigned int)) = entryFlags;
    memcpy((char*)pArchHeader + entrySize - fSize, fData, fSize);

    if (utils_save_file(gCmdOptions.outFile, (const uint8_t*)pArchHeader, entrySize) != ERR_OK) {
        printf("Cannot save %s\n", gCmdOptions.outFile);
        goto exit;
    }

    ret = EXIT_SUCCESS;
exit:

    if (ret == EXIT_SUCCESS) {
        printf("%s - OK\n", newEntryName);
    }

    if (fData != NULL) {
        free(fData);
    }

    return ret;
}
