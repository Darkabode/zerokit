#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#define BIN2HEX_VERSION "1.1.1"

#include "../../../shared/platform.h"
#include "../../../shared/native.h"
#include "../../../shared/types.h"
#include "../../../shared/config.h"
#include "../../../shared/utils_cli.h"

typedef struct cmd_options
{
    char* inPath;
    char* outPath;
    char* name;
    int isConst;
} cmd_options_t, *pcmd_options_t;

cmd_options_t gCmdOptions;

cmd_line_info_t cmdLineInfo = {4,
{
    {{"i", "in"}, "Path to input file", "Specify nonempty path", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.inPath},
    {{"o", "out"}, "Path to out file", "Specify nonempty path", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.outPath},
    {{"n", "name"}, "Name of C-variable", "Specify nonempty name", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.name},
    {{"c", "const"}, "Const C-array", "", OPT_FLAG_OPTIONAL, TYPE_CMDLINE | TYPE_BOOLEAN, (void*)&gCmdOptions.isConst},
}
};

int __cdecl main(int argc, char** argv)
{
    int ret = EXIT_FAILURE;
    uint8_t* inData = NULL;
    char* outData = NULL;
    char* itr;
    static const char newLine[] = "\n    ";
    size_t i, inSize = 0, outSize;
    char sConst[12];

    memset(sConst, 0, sizeof(sConst));
    memset(&gCmdOptions, 0, sizeof(gCmdOptions));
    if (config_parse_cmdline(argc, argv, &cmdLineInfo, BIN2HEX_VERSION) != 0) {
        return ret; 
    }

    printf("Checking file %s for existence... ", gCmdOptions.inPath);
    if (utils_file_exists(gCmdOptions.inPath) != ERR_OK) {
        printf("Failed: not exists\n\n");
        goto exit;
    }

    printf("OK\nReading file %s... ", gCmdOptions.inPath);
    if (utils_read_file(gCmdOptions.inPath, &inData, &inSize) != ERR_OK) {
        printf("Failed\n\n");
        goto exit;
    }

    printf("OK\nGenerating C-array... ");

    outSize = inSize * 5 + (inSize / 16) * 5 + 1024;
    outData = malloc(outSize);
    memset(outData, 0, outSize);
    itr = outData;

    if (gCmdOptions.isConst) {
        strcpy(sConst, "const ");
    }

    itr += wsprintfA(itr, "%sunsigned char %s[%u] = {\n    ", sConst, gCmdOptions.name, inSize);
    for (i = 0; i < inSize; ++i) {
        itr += wsprintfA(itr, "0x%02X", inData[i]);
        if ((i + 1) < inSize) {
            *(itr++) = ',';
            if (i > 0 && (i + 1) % 16 == 0) {
                strcpy(itr, newLine);
                itr += strlen(newLine);
            }
        }
    }
    strcpy(itr, "};"); itr += strlen("};");

    printf("OK\nSaving C-array to %s... ", gCmdOptions.outPath);
    if (utils_save_file(gCmdOptions.outPath, outData, itr - outData) != ERR_OK) {
        printf("Failed\n\n");
    }

    printf("OK\n");
    ret = EXIT_SUCCESS;

exit:
    if (inData != NULL) {
        free(inData);
    }

    if (outData != NULL) {
        free(outData);
    }

    return ret;
}
