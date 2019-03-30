#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USE_PRIME_NUMBERS 1

#include "../../../shared_code/platform.h"
#include "../../../shared_code/types.h"
#include "../../../shared_code/config.h"
#include "../../../shared_code/native.h"
#include "../../../shared_code/base64.h"
#include "../../../shared_code/havege.h"
#include "../../../shared_code/crc64.h"
#include "../../../shared_code/utils.h"
#include "../../../shared_code/bignum.h"
#include "../../../shared_code/bn_mul.h"
#include "../../../shared_code/rsa.h"
#include "../../../shared_code/arc4.h"
#include "../../../shared_code/sha1.h"

#include "../../../shared_code/havege.c"
#include "../../../shared_code/arc4.c"
#include "../../../shared_code/sha1.c"
#include "../../../shared_code/bignum.c"
#include "../../../shared_code/rsa.c"

#define SKEYGEN_VERSION "0.9.5"

#define KEY_SIZE 1024
#define EXPONENT 65537

char* findDataBlock(char* data, size_t size, char* blockName, size_t blockSize)
{
    char* itr = data;
    char* blockItr;
    char* end = data + size - blockSize;

    for ( ; itr < end; ++itr) {
        if (*itr == blockName[0]) {
            if (itr[1] == blockName[1] && itr[2] == blockName[2]) {
                blockItr = blockName + 3;
                itr += 3;

                for ( ; *blockItr != 0; ++blockItr, ++itr) {
                    if (*blockItr != *itr)
                        break;
                }

                if (*blockItr == 0)
                    return (itr - blockSize);
            }
        }
    }

    return 0;
}

typedef struct cmd_options
{
    uint32_t sid;      // Идентификатор клиента
} cmd_options_t, *pcmd_options_t;

cmd_options_t gCmdOptions;

cmd_line_info_t cmdLineInfo = {1,
{
    {{"s", "sid"}, "ServerId for generate server configuration", "Specify integer value for sid", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_UINT32, (void*)&gCmdOptions.sid},
}
};
int main(int argc, char** argv)
{
    int i;
    size_t sz;
    rsa_context_t rsa;
    havege_state_t hs;
    char rootPath[MAX_PATH];
    char sidsPath[MAX_PATH];
    char tmp[32];
    uint8_t* ptr;
    uint8_t skey[1024];
    char filePath[512];
    int ret = EXIT_FAILURE;

    GetModuleFileName(NULL, rootPath, MAX_PATH);

    memset(&gCmdOptions, 0, sizeof(cmd_options_t));

    if (config_parse_cmdline(argc, argv, &cmdLineInfo, SKEYGEN_VERSION) != ERR_OK) {
        return EXIT_FAILURE;
    }

    printf("\n--- skeygen ---\n\n");

    rsa_init(&rsa, RSA_PKCS_V15, 0);

     do {
        ptr = rootPath + strlen(rootPath);
        // Получаем путь \bin.
        for ( ; *ptr != '\\' && *ptr != '/'; --ptr);
        // Получаем корень.
        *ptr = '\0';

        // Делаем текущей директорией путь, откуда запустились.
        if (utils_set_current_directory(rootPath) != ERR_OK) {
            printf(("Failed\n  ! Can't set current directory\n\n"));
            break;
        }

        ptr -=3;
        // Проверям нашу папку, если это не bin, то скорее всего нас запустиил не с той папки.
        if (_stricmp(ptr, "bin") != 0) {
            printf(("Failed\n  ! Path of builder is incorrect\n\n"));
            break;
        }

        *ptr = '\0';

        // Генерируем путь к папке сервернх конфигураций.
        utils_build_path(sidsPath, rootPath, "sids\\", 0);
        _itoa(gCmdOptions.sid, tmp, 10);
        strcat_s(sidsPath, MAX_PATH, tmp);
        strcat_s(sidsPath, MAX_PATH, "\\");

        crc64_buildtable();

        havege_init(&hs);

        printf("Found!\nGenerating the RSA key (%d-bit)... ", KEY_SIZE);
        if ((ret = rsa_gen_key(&rsa, havege_rand, &hs, KEY_SIZE, EXPONENT)) != 0) {
            printf("Failed\n  ! rsa_gen_key returned %d\n\n", ret);
            break;
        }

        printf("OK\nPatching handler... ");
        // Сохраняем закрытый RSA-ключ.
        ptr =skey;
        sz = mpi_size(&rsa.N); *(uint16_t*)ptr = sz; ptr += 2; mpi_write_binary(&rsa.N, ptr, sz); ptr += sz;
        sz = mpi_size(&rsa.E); *(uint16_t*)ptr = sz; ptr += 2; mpi_write_binary(&rsa.E, ptr, sz); ptr += sz;
        sz = mpi_size(&rsa.D); *(uint16_t*)ptr = sz; ptr += 2; mpi_write_binary(&rsa.D, ptr, sz); ptr += sz;
        sz = mpi_size(&rsa.P); *(uint16_t*)ptr = sz; ptr += 2; mpi_write_binary(&rsa.P, ptr, sz); ptr += sz;
        sz = mpi_size(&rsa.Q); *(uint16_t*)ptr = sz; ptr += 2; mpi_write_binary(&rsa.Q, ptr, sz); ptr += sz;
        sz = mpi_size(&rsa.DP); *(uint16_t*)ptr = sz; ptr += 2; mpi_write_binary(&rsa.DP, ptr, sz); ptr += sz;
        sz = mpi_size(&rsa.DQ); *(uint16_t*)ptr = sz; ptr += 2; mpi_write_binary(&rsa.DQ, ptr, sz); ptr += sz;
        sz = mpi_size(&rsa.QP); *(uint16_t*)ptr = sz; ptr += 2; mpi_write_binary(&rsa.QP, ptr, sz); ptr += sz;

        printf("OK\nSaving RSA Private Server Key... ");
        utils_build_path(filePath, sidsPath, "skey.private", 0);
        if (utils_save_file(filePath, skey, ptr - (uint8_t*)skey) != ERR_OK) {
            printf("Failed\n  ! Can't save %s\n\n", filePath);
            break;
        }

        ptr =skey;
        sz = mpi_size(&rsa.N); *(uint16_t*)ptr = sz; ptr += 2; mpi_write_binary(&rsa.N, ptr, sz); ptr += sz;
        sz = mpi_size(&rsa.E); *(uint16_t*)ptr = sz; ptr += 2; mpi_write_binary(&rsa.E, ptr, sz); ptr += sz;

        printf("OK\nSaving RSA Public Server Key... ");
        utils_build_path(filePath, sidsPath, "skey.public", 0);
        if (utils_save_file(filePath, skey, ptr - (uint8_t*)skey) != ERR_OK) {
            printf("Failed\n  ! Can't save %s\n\n", filePath);
            break;
        }
        printf("OK\n");


        ret = EXIT_SUCCESS;
    } while (0);

    rsa_free(&rsa);

    fprintf(stdout, "\n--- skeygen ---\n");

    return ret;
}
