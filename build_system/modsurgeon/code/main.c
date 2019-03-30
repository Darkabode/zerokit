#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../shared_code/platform.h"
#include "../../../shared_code/types.h"
#include "../../../shared_code/crc64.h"
#include "../../../shared_code/utils.h"
#include "../../../shared_code/config.h"

#include "../../../loader/mod_shared/zerokit.h"

#define MODSURGEON_VERSION "0.6.6"

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
                    if (*blockItr != *itr) {
						break;
                    }
				}

                if (*blockItr == 0) {
					return (itr - blockSize);
                }
			}
		}
	}

	return 0;
}

typedef struct cmd_options
{
    char* modPath; // Путь с именем файла для сгенерированного пака.
    char* outPath;  // Путь к файлам буткита.
    int noisy;      // Наличие параметра позволит выводить подробную информацию. Иначе буду выводится только имена файлов.
} cmd_options_t, *pcmd_options_t;

cmd_options_t gCmdOptions;

cmd_line_info_t cmdLineInfo = {3,
{
    {{"m" , "mod"}, "Path to mod", "", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.modPath},
    {{"o" , "out"}, "Path to output directory", "", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.outPath},
    {{"n", "noisy"}, "Noisy mode", "", OPT_FLAG_OPTIONAL, TYPE_CMDLINE | TYPE_BOOLEAN, (void*)&gCmdOptions.noisy},
}
};

#define NOISY_PRINTF(x) if (gCmdOptions.noisy) printf x;
#define QUIET_PRINTF(x) if (!gCmdOptions.noisy) printf x;

// Первым параметром командной строки идёт путь к файлу ldr.conf, вторым ldrsrv.conf
int main(int argc, char** argv)
{
	char* origName;
	char filePath[512];
	uint8_t* modFileContent = NULL;
	char* dest = NULL;
	uint32_t destSize = 0;
	size_t modFileSize;
	PIMAGE_DOS_HEADER dosHdr;
	PIMAGE_NT_HEADERS ntHdr;
	PIMAGE_SECTION_HEADER sectHdr;
	UINT16 i, sectionNum;
	mod_header_t modHdr;
	int ret = EXIT_FAILURE;

    memset(&gCmdOptions, 0, sizeof(cmd_options_t));

    if (config_parse_cmdline(argc, argv, &cmdLineInfo, MODSURGEON_VERSION) != ERR_OK) {
        return EXIT_FAILURE;
    }

	NOISY_PRINTF(("\n--- modsurgeon ---\n\n"));

	do {
		NOISY_PRINTF(("Reading file %s... ", gCmdOptions.modPath));
		if (utils_read_file(gCmdOptions.modPath, &modFileContent, &modFileSize) != 0) {
			NOISY_PRINTF(("Failed\n  ! Can't load file %s\n\n", gCmdOptions.modPath));
			break;
		}
		
		NOISY_PRINTF(("OK\nChecking file is executable... "));
		dosHdr = (PIMAGE_DOS_HEADER)modFileContent;
		if (dosHdr->e_magic != IMAGE_DOS_SIGNATURE) {
			NOISY_PRINTF(("Failed\n  ! File is not Can't load file %s\n\n", gCmdOptions.modPath));
			break;
		}

		ntHdr = (PIMAGE_NT_HEADERS)(modFileContent + dosHdr->e_lfanew);
		if (ntHdr->Signature != IMAGE_NT_SIGNATURE) {
			NOISY_PRINTF(("Failed\n  ! File is not Can't load file %s\n\n", gCmdOptions.modPath));
			break;
		}

		crc64_buildtable();
		
		origName = gCmdOptions.modPath + strlen(gCmdOptions.modPath);
		for (; *origName != '/' && *origName != '\\'; --origName);
		++origName;

		NOISY_PRINTF(("OK\nSearching for .text section... "));

		sectHdr = IMAGE_FIRST_SECTION(ntHdr);
		sectionNum = ntHdr->FileHeader.NumberOfSections;
		
		for (i = 0; i < sectionNum; ++i) {
			if (strcmp(sectHdr->Name, ".text") == 0 && (sectHdr->Characteristics & (IMAGE_SCN_CNT_CODE|IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_MEM_READ)) == 
				(IMAGE_SCN_CNT_CODE|IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_MEM_READ)) {
					NOISY_PRINTF(("OK\nTransforming '%s'... ", origName));
					destSize = sizeof(mod_header_t) + sectHdr->SizeOfRawData;
					dest = malloc(destSize);

					if (dest == NULL) {
						break;
					}
					
					memcpy(dest + sizeof(mod_header_t), modFileContent + sectHdr->PointerToRawData, sectHdr->SizeOfRawData);

                    __stosb((uint8_t*)&modHdr, 0, sizeof(modHdr));
					modHdr.sizeOfModReal = modHdr.sizeOfMod = sectHdr->SizeOfRawData;
					modHdr.fakeBase = sectHdr->VirtualAddress - sizeof(mod_header_t);
					modHdr.entryPointRVA = sizeof(mod_header_t) + ntHdr->OptionalHeader.AddressOfEntryPoint - sectHdr->VirtualAddress;
					crc64_computate(dest + sizeof(mod_header_t), modHdr.sizeOfMod, &modHdr.crc);

					memcpy(dest, &modHdr, sizeof(mod_header_t));

					break;
			}

			sectHdr = (PIMAGE_SECTION_HEADER)((PUCHAR)sectHdr + sizeof(IMAGE_SECTION_HEADER));
		}

		if (i >= sectionNum) {
			NOISY_PRINTF(("Failed\n  ! Not found .text section\n\n"));
			break;
		}
		else if (dest == NULL) {
			NOISY_PRINTF(("Failed\n  ! Failed to allocate memory for transformated mod\n\n"));
			break;
		}

		// Формируем путь, где будем сохранять пропатченную версию handler.
        origName[strlen(origName) - 1] = '_';
		utils_build_path(filePath, gCmdOptions.outPath, origName, 0);

		NOISY_PRINTF(("OK\nSaving transformed mod to %s... ", filePath));
		if (utils_save_file(filePath, dest, destSize) != 0) {
			NOISY_PRINTF(("Failed\n  ! Can't save transformed mod to file %s\n\n", filePath));
			break;
		}

		NOISY_PRINTF(("OK\n"));
		ret = EXIT_SUCCESS;
	} while (0);

    if (modFileContent != NULL) {
		free(modFileContent);
    }
    if (dest != NULL) {
		free(dest);
    }

    NOISY_PRINTF(("\n--- modsurgeon ---\n\n"));

	return ret;
}
