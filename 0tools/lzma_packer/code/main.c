#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#define LZMAPACKER_VERSION "0.5.0"

#include "../../../shared/platform.h"
#include "../../../shared/native.h"
#include "../../../shared/types.h"
#include "../../../shared/config.h"
#include "../../../shared/utils_cli.h"
#include "../../../shared/lzma.h"

#define USE_LZMA_COMPRESSOR 1
#include "../../../shared/lzma.c"

#include "../../../0kit/userio_api/zuserio/code/zuserio.h"
#include "../../../0kit/mod_shared/pack_protect.h"

typedef struct cmd_options
{
    char* inPath;
    char* outPath;
} cmd_options_t, *pcmd_options_t;

cmd_options_t gCmdOptions;

cmd_line_info_t cmdLineInfo = {2,
{
    {{"i", "in"}, "Path to input file", "Specify correct path to input file", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.inPath},
    {{"o", "out"}, "Path to signed file (with file name)", "Specify correct path to signed file", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.outPath},
}
};

int __cdecl main(int argc, char** argv)
{
    int ret = EXIT_FAILURE;
    uint8_t* fData = NULL;
    size_t origSize = 0, fSize = 0, packSize = 0;
    char* packBuffer = NULL;
    CLzmaEncProps props;
    uint32_t outSize;
    uint8_t* outData;
    unsigned propsSize = LZMA_PROPS_SIZE; 

    memset(&gCmdOptions, 0, sizeof(gCmdOptions));

    if (config_parse_cmdline(argc, argv, &cmdLineInfo, LZMAPACKER_VERSION) != 0) {
        return ret; 
    }

    printf("Checking file %s for existence...", gCmdOptions.inPath);
    if (utils_file_exists(gCmdOptions.inPath) != ERR_OK) {
        printf("Failed: not exists\n\n");
        goto exit;
    }

    printf("OK\nReading file %s... ", gCmdOptions.inPath);
    if (utils_read_file(gCmdOptions.inPath, &fData, &fSize) != ERR_OK) {
        printf("Failed: cannot read\n\n");
        goto exit;
    }

    printf("OK\nCompressing file...");

    LzmaEncProps_Init(&props);
    props.dictSize = 1048576; // 1 MB

    outSize = fSize + fSize/3 + 128 - LZMA_PROPS_SIZE;
    outData = malloc(outSize);

    ret = LzmaEncode(&outData[LZMA_PROPS_SIZE], &outSize, fData, fSize, &props, outData, &propsSize);

    if (ret != ERR_OK) {
        printf( "Failed\n\n");
        goto exit;
    }

    fSize = outSize + propsSize;

    free(fData);
    fData = outData;

    printf("OK\nSaving to %s...", gCmdOptions.outPath);
    if (utils_save_file(gCmdOptions.outPath, fData, fSize) != ERR_OK) {
        printf("Failed\n");
    }

    printf("OK\n");
    ret = EXIT_SUCCESS;

exit:
    if (fData != NULL) {
        free(fData);
    }

    if (packBuffer != NULL) {
        free(packBuffer);
    }

    return ret;
}
