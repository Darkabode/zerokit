#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../shared_code/platform.h"
#include "../../../shared_code/native.h"
#include "../../../shared_code/types.h"
#include "../../../shared_code/crc64.h"
#include "../../../shared_code/utils.h"
#include "../../../shared_code/config.h"
#include "../../../shared_code/arc4.h"
#include "../../../shared_code/lzma.h"

#include "../../../shared_code/arc4.c"


#define USE_LZMA_COMPRESSOR 1
#include "../../../shared_code/lzma.c"

#include "../../../loader/mod_shared/zerokit.h"

#define MODSJOINER_VERSION "0.5.9"

typedef struct _global_config
{
	char* modsList; // Список mod-ов из которых состоит зерокит.
} global_config_t;

global_config_t gConfigParams;

void* handleConfigParameter(char* keyName, int* pValueType)
{
	void* pVal = NULL;

	if (gConfigParams.modsList == NULL && strcmp(keyName, "mods_list") == 0) {
		pVal = (void*)&gConfigParams.modsList; *pValueType = TYPE_STRING;
	}

	return pVal;
}

typedef struct cmd_options
{
    char* modsPath; // Путь с именем файла для сгенерированного пака.
    char* outPath;  // Путь к файлам буткита.
    int noisy;      // Наличие параметра позволит выводить подробную информацию. Иначе буду выводится только имена файлов.
} cmd_options_t, *pcmd_options_t;

cmd_options_t gCmdOptions;

cmd_line_info_t cmdLineInfo = {3,
{
    {{"m" , "mods"}, "Path to mods dir", "", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.modsPath},
    {{"o" , "out"}, "Path to output directory", "", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.outPath},
    {{"n", "noisy"}, "Noisy mode", "", OPT_FLAG_OPTIONAL, TYPE_CMDLINE | TYPE_BOOLEAN, (void*)&gCmdOptions.noisy},
}
};

#define NOISY_PRINTF(x) if (gCmdOptions.noisy) printf x;
#define QUIET_PRINTF(x) if (!gCmdOptions.noisy) printf x;

// Первым параметром командной строки идёт путь к файлу ldr.conf, вторым ldrsrv.conf
int main(int argc, char** argv)
{
	char *ptr1, *ptr2, *end;
	char originPath[MAX_PATH];
	char packsPath[MAX_PATH];
	char filePath[MAX_PATH];
	char commandLine[1024];
	char outModPath[MAX_PATH];
	uint8_t* modFileContent = NULL;
	size_t modFileSize;
	int i, ret = EXIT_FAILURE;
	char modName[64];
	DWORD exitCode;
	char* suffix[2] = {"_x32.sys", "_x64.sys"};
	uint8_t *pack32Buffer = NULL, *pack64Buffer = NULL;
	mods_pack_header_t packHdrs[2];
	uint8_t* packBuffers[2];
    uint32_t modNum = 0;

    memset(&gCmdOptions, 0, sizeof(cmd_options_t));

    if (config_parse_cmdline(argc, argv, &cmdLineInfo, MODSJOINER_VERSION) != ERR_OK) {
        return EXIT_FAILURE;
    }

	NOISY_PRINTF(("\n--- modsjoiner ---\n\n"));

	memset(&gConfigParams, 0, sizeof(global_config_t));

	NOISY_PRINTF((" Loading config... "));
	if (config_load("modsjoiner.conf", line_parser_conf, line_parser_conf_done, handleConfigParameter) != 0) {
		goto exit;
	}

    NOISY_PRINTF(("OK\n Checking modsurgeon executable... "));
	if (utils_is_file_exists("modsurgeon.exe") != 0) {
		NOISY_PRINTF(("Failed   ! Not exsists\n\n"));
		goto exit;
	}

	strcpy_s(originPath, MAX_PATH, gCmdOptions.modsPath);
	filePath[0] = originPath[strlen(originPath) - 1];
	if (filePath[0] != '/' && filePath[0] != '\\')
		strcat_s(originPath, MAX_PATH, "\\");

	strcpy_s(packsPath, MAX_PATH, gCmdOptions.outPath);
// 	filePath[0] = packsPath[strlen(packsPath) - 1];
// 	if (filePath[0] != '/' && filePath[0] != '\\')
// 		strcat_s(packsPath, MAX_PATH, "\\");

	if (utils_is_file_exists(packsPath) != 0) {
		NOISY_PRINTF(("OK\n Creating '%s' directory... ", packsPath));
		if (utils_create_directory(packsPath) != 0) {
			NOISY_PRINTF(("Failed  ! Can't to create directory\n\n"));
			goto exit;
		}
	}

    NOISY_PRINTF(("OK\n"));

	pack32Buffer = malloc(sizeof(mods_pack_header_t));
	pack64Buffer = malloc(sizeof(mods_pack_header_t));

	packBuffers[0] = pack32Buffer;
	packBuffers[1] = pack64Buffer;

	packHdrs[0].sizeOfPack = packHdrs[1].sizeOfPack = 0;

	// Промебаемся по всему списку и для каждого мода вызываем modsurgeon
	ptr1 = ptr2 = gConfigParams.modsList;
	end = ptr1 + strlen(gConfigParams.modsList);
	for ( ; *ptr1 != '\0' && ptr1 < end; ) {
		for ( ; *ptr2 != ';' && *ptr2 != '\0'; ++ptr2);
		*ptr2 = '\0';

		for (i = 0; i < 2; ++i) {
            
			strcpy_s(modName, 64, "mod_");
			strcat_s(modName, 64, ptr1);
			strcat_s(modName, 64, suffix[i]);
			
			utils_build_path(filePath, originPath, modName, 0);

            sprintf_s(commandLine, 1024, "modsurgeon.exe -m=\"%s%s\" -o=\"%s\"", originPath, modName, packsPath);
            if (gCmdOptions.noisy) {
                strcat_s(commandLine, 1024, " -n");
            }

            if (!utils_launch_and_verify(commandLine, &exitCode)) {
                NOISY_PRINTF(("Failed\n  ! modsurgeon exited with error %d\n\n", exitCode));
                goto exit;
            }

            utils_build_path(filePath, packsPath, modName, 0);
            filePath[strlen(filePath) - 1] = '_';

            if (modNum == 0) {
                if (utils_read_file(filePath, &modFileContent, &modFileSize) != 0) {
                    NOISY_PRINTF(("Failed\n  ! Can't load %s\n\n", modName));
                    goto exit;
                }
            }
            else {
                CLzmaEncProps props;
                uint32_t outSize;
                uint8_t* outData;
                unsigned propsSize = LZMA_PROPS_SIZE;
                pmod_header_t pModHeader;

                if (utils_read_file(filePath, &modFileContent, &modFileSize) != 0) {
                    NOISY_PRINTF(("Failed\n  ! Can't load %s\n\n", filePath));
                    goto exit;
                }

                pModHeader = (pmod_header_t)modFileContent;

                LzmaEncProps_Init(&props);
                props.dictSize = 1048576; // 1 MB

                outSize = pModHeader->sizeOfModReal + pModHeader->sizeOfModReal/3 + 128 - LZMA_PROPS_SIZE;
                outData = malloc(outSize);

                ret = LzmaEncode(&outData[LZMA_PROPS_SIZE], &outSize, modFileContent + sizeof(mod_header_t), pModHeader->sizeOfModReal, &props, outData, &propsSize);

                if (ret != ERR_OK) {
                    NOISY_PRINTF(("Failed\n  ! Can't compress mod\n\n"));
                    goto exit;
                }

                modFileContent = realloc(modFileContent, sizeof(mod_header_t) + outSize);

                pModHeader = (pmod_header_t)modFileContent;
                pModHeader->flags |= MODF_COMPRESSED;
                pModHeader->sizeOfMod = outSize;
                memcpy(modFileContent + sizeof(mod_header_t), outData, outSize);
                modFileSize = outSize + sizeof(mod_header_t);

                free(outData);
            }

			packBuffers[i] = realloc(packBuffers[i], sizeof(mods_pack_header_t) + packHdrs[i].sizeOfPack + modFileSize);
			if (packBuffers[i] == NULL) {
				NOISY_PRINTF(("Failed\n  ! Can't reallocate memory in pack buffer for %s\n\n", modName));
				goto exit;
			}

			memcpy(packBuffers[i] + sizeof(mods_pack_header_t) + packHdrs[i].sizeOfPack, modFileContent, modFileSize);
			free(modFileContent);
			modFileContent = NULL;

			packHdrs[i].sizeOfPack += (uint32_t)modFileSize;
		}		

		ptr1 = ++ptr2;
        ++modNum;
	}

	pack32Buffer = packBuffers[0];
	pack64Buffer = packBuffers[1];

	crc64_buildtable();

	crc64_computate(pack32Buffer + sizeof(mods_pack_header_t), (size_t)packHdrs[0].sizeOfPack, (uint32_t*)&packHdrs[0].crc);
	crc64_computate(pack64Buffer + sizeof(mods_pack_header_t), (size_t)packHdrs[1].sizeOfPack, (uint32_t*)&packHdrs[1].crc);

	memcpy(pack32Buffer, &packHdrs[0], sizeof(mods_pack_header_t));
	memcpy(pack64Buffer, &packHdrs[1], sizeof(mods_pack_header_t));

    utils_build_path(outModPath, packsPath, "pack32.bin", 0);

	NOISY_PRINTF(("Saving 32-bit pack to %s... ", outModPath));
	if (utils_save_file(outModPath, pack32Buffer, sizeof(mods_pack_header_t) + packHdrs[0].sizeOfPack) != 0) {
		NOISY_PRINTF(("Failed\n  ! Can't save pack\n\n", outModPath));
		goto exit;
	}

    utils_build_path(outModPath, packsPath, "pack64.bin", 0);

	NOISY_PRINTF(("OK\nSaving 64-bit pack to %s... ", outModPath));
	if (utils_save_file(outModPath, pack64Buffer, sizeof(mods_pack_header_t) + packHdrs[1].sizeOfPack) != 0) {
		NOISY_PRINTF(("Failed\n  ! Can't save pack\n\n", outModPath));
		goto exit;
	}

	NOISY_PRINTF(("OK\n"));

    ret = EXIT_SUCCESS;
exit:
	if (pack32Buffer != NULL)
		free(pack32Buffer);
	if (pack64Buffer != NULL)
		free(pack64Buffer);
	if (modFileContent != NULL)
		free(modFileContent);

    NOISY_PRINTF(("\n--- modsjoiner ---\n\n"));

	return ret;
}
