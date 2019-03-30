#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <Windows.h>

#define RC4CRYPTOR_VERSION "0.4.2"

#include "../../../shared/platform.h"
#include "../../../shared/native.h"
#include "../../../shared/types.h"
#include "../../../shared/config.h"
#include "../../../shared/utils_cli.h"
#include "../../../shared/arc4.h"


typedef struct cmd_options
{
    char* inPath;
    char* outPath;
    uint16_t algo;
} cmd_options_t, *pcmd_options_t;

cmd_options_t gCmdOptions;

cmd_line_info_t cmdLineInfo = {3,
{
    {{"i", "in"}, "Path to input file", "Specify correct path to input file", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.inPath},
    {{"o", "out"}, "Path to signed file (with file name)", "Specify correct path to signed file", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.outPath},
    {{"a", "algo"}, "AlgoID (0 - RC4, 1 - XOR)", "", OPT_FLAG_OPTIONAL, TYPE_CMDLINE | TYPE_UINT16, (void*)&gCmdOptions.algo}
}
};

uint32_t __stdcall RtlRandomEx(uint32_t* Seed);

int __cdecl main(int argc, char** argv)
{
    uint8_t key[250];
    int i, k, keySize, prefixSize;
    int rndPos;
    int ret = EXIT_FAILURE;
    uint8_t* fData = NULL;
    uint8_t* outData;
    size_t fSize = 0;
    uint32_t seed = GetTickCount();

    memset(&gCmdOptions, 0, sizeof(gCmdOptions));

    if (config_parse_cmdline(argc, argv, &cmdLineInfo, RC4CRYPTOR_VERSION) != 0) {
        return ret; 
    }

    printf("Checking file %s for existence... ", gCmdOptions.inPath);
    if (utils_file_exists(gCmdOptions.inPath) != ERR_OK) {
        printf("Not exists\n");
        goto exit;
    }

    printf("OK\nReading file %s... ", gCmdOptions.inPath);
    if (utils_read_file(gCmdOptions.inPath, &fData, &fSize) != ERR_OK) {
        printf("Failed\n");
        goto exit;
    }

    printf("OK\nCryping file... ");

    keySize = 25 + (RtlRandomEx(&seed) % 215);
    for (i = 0; i < keySize; ++i) {
        key[i] = (uint8_t)(RtlRandomEx(&seed) % 256);
    }

    if (gCmdOptions.algo == 0) {
        arc4_crypt_self(fData, fSize, key, keySize);
    }
    else if (gCmdOptions.algo == 1) {
        for (k = 0, i = 0; i < fSize; ++i, ++k) {
            *(fData + i) ^= key[k % keySize];
        }
    }
    else {
        printf("Unknown algo\n");
        goto exit;
    }

    prefixSize = keySize * 7 + 2;
    outData = malloc(fSize + prefixSize);
    memcpy(outData + prefixSize, fData, fSize);
    rndPos = RtlRandomEx(&seed) % 7;
    outData[0] = (uint8_t)rndPos;
    outData[1] = (uint8_t)keySize;
    for (k = 0, i = 2; i < prefixSize; ++i) {
        if ((i - 2) % 7 == rndPos) {
            outData[i] = key[k++];
        }        
        else {
            outData[i] = (uint8_t)(RtlRandomEx(&seed) % 256);
        }
    }

    printf("OK\nSaving Pack to %s... ", gCmdOptions.outPath);
    if (utils_save_file(gCmdOptions.outPath, outData, fSize + prefixSize) != ERR_OK) {
        printf("Failed\n");
        goto exit;
    }

    printf("OK\n");
    ret = EXIT_SUCCESS;

exit:
    if (fData != NULL) {
        free(fData);
    }

    return ret;
}
