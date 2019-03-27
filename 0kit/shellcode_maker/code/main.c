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

#define MODSURGEON_VERSION "0.6.7"

typedef struct cmd_options
{
    char* modPath;  // Путь с именем файла для сгенерированного пака.
    char* outPath;  // Путь к файлам буткита.
    char* name;     // Имя шеллкода, которое будет фигурировать в имени функции и прочих ссылках.
    int verbose;    // Шумный режим вывода.
    int binMode;    // Бинарный режим вывода.
} cmd_options_t, *pcmd_options_t;

cmd_options_t gCmdOptions;

cmd_line_info_t cmdLineInfo = {5,
{
    {{"i" , "in"}, "Path to original module", "", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.modPath},
    {{"o" , "out"}, "Path to output directory", "", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.outPath},
    {{"n", "name"}, "Shellcode name", "", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.name},
    {{"v", "verbose"}, "Verbose output mode", "", OPT_FLAG_OPTIONAL, TYPE_CMDLINE | TYPE_BOOLEAN, (void*)&gCmdOptions.verbose},
    {{"b", "bin"}, "Binary output", "", OPT_FLAG_OPTIONAL, TYPE_CMDLINE | TYPE_BOOLEAN, (void*)&gCmdOptions.binMode},
}
};

#define NOISY_PRINTF(x) if (gCmdOptions.verbose) printf x;


// Первым параметром командной строки идёт путь к файлу ldr.conf, вторым ldrsrv.conf
int main(int argc, char** argv)
{
    char* origName;
    char filePath[512];
    uint8_t* modFileContent = NULL;
    uint8_t*dest = NULL, *ptr;
    char* cFileContent = NULL;
    uint32_t destSize = 0;
    uint32_t fnOffset;
    size_t modFileSize;
    PIMAGE_DOS_HEADER dosHdr;
    PIMAGE_NT_HEADERS ntHdr;
    PIMAGE_SECTION_HEADER sectHdr;
    uint32_t i;
    uint32_t sectionNum;
    int ret = EXIT_FAILURE;

    memset(&gCmdOptions, 0, sizeof(cmd_options_t));

    if (config_parse_cmdline(argc, argv, &cmdLineInfo, MODSURGEON_VERSION) != ERR_OK) {
        return EXIT_FAILURE;
    }

    NOISY_PRINTF(("Shellcode Maker -->\n"));

    do {
        NOISY_PRINTF(("Reading file %s... ", gCmdOptions.modPath));
        if (utils_read_file(gCmdOptions.modPath, &modFileContent, &modFileSize) != 0) {
            printf("Failed\n  ! Can't load file %s\n\n", gCmdOptions.modPath);
            break;
        }

        NOISY_PRINTF(("OK\nChecking file is executable... "));
        dosHdr = (PIMAGE_DOS_HEADER)modFileContent;
        if (dosHdr->e_magic != IMAGE_DOS_SIGNATURE) {
            printf("Failed\n  ! File is not valid %s\n\n", gCmdOptions.modPath);
            break;
        }

        ntHdr = (PIMAGE_NT_HEADERS)(modFileContent + dosHdr->e_lfanew);
        if (ntHdr->Signature != IMAGE_NT_SIGNATURE) {
            printf("Failed\n  ! File is not valid %s\n\n", gCmdOptions.modPath);
            break;
        }

        origName = gCmdOptions.modPath + strlen(gCmdOptions.modPath);
        for (; origName >= gCmdOptions.modPath && *origName != '/' && *origName != '\\'; --origName);
        ++origName;

        NOISY_PRINTF(("OK\nSearching for  .text section... "));

        sectHdr = IMAGE_FIRST_SECTION(ntHdr);
        sectionNum = (uint32_t)ntHdr->FileHeader.NumberOfSections;

        for (i = 0; i < sectionNum; ++i) {
            if (strcmp(sectHdr->Name, ".text") == 0 && (sectHdr->Characteristics & (IMAGE_SCN_CNT_CODE|IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_MEM_READ)) == 
                (IMAGE_SCN_CNT_CODE|IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_MEM_READ)) {
                NOISY_PRINTF(("OK\nShellcode '%s' extraction... ", origName));

                fnOffset = (uint32_t)(ntHdr->OptionalHeader.AddressOfEntryPoint - sectHdr->VirtualAddress);
                destSize = sectHdr->SizeOfRawData - fnOffset;
                dest = malloc(destSize);

                if (dest == NULL) {
                    break;
                }

                memcpy(dest, modFileContent + sectHdr->PointerToRawData + fnOffset, destSize);
                break;
            }

            sectHdr = (PIMAGE_SECTION_HEADER)((PUCHAR)sectHdr + sizeof(IMAGE_SECTION_HEADER));
        }

        if (i >= sectionNum) {
            printf("Failed\n  ! Not found .text section\n\n");
            break;
        }
        else if (dest == NULL) {
            printf("Failed\n  ! Failed to allocate memory for shellcode\n\n");
            break;
        }

        // Обрезаем конец шеллкода (компилеровский align).
        for (ptr = dest + destSize - 1; *ptr == 0x90 || *ptr == 0x00 || *ptr == 0xCC; --ptr, --destSize);

        if (*(ptr - 1) == 0xC2) {
            ++destSize;
        }

        if (gCmdOptions.binMode) {
            // Формируем путь, где будем сохранять пропатченную версию handler.
            origName[strlen(origName) - 3] = 'b';
            origName[strlen(origName) - 2] = '\0';
            utils_build_path(filePath, gCmdOptions.outPath, origName, 0);

            NOISY_PRINTF(("OK\nSaving bin to %s... ", filePath));
            if (utils_save_file(filePath, dest, destSize) != 0) {
                printf("Failed\n  ! Can't save bin to file %s\n\n", filePath);
                break;
            }
        }
        else {
            // Генерируем содержимое *.c файла.
            cFileContent = malloc(12 * destSize + strlen(gCmdOptions.name) + 64);
            if (cFileContent == NULL) {
                printf("Failed\n  ! Failed to allocate memory for C file content\n\n");
                break;
            }

            sprintf(cFileContent, 
                "const uint8_t %s[%u] = {",
                gCmdOptions.name, destSize);
            for (i = 0; i < destSize; ++i) {
                char tmp[7];
                if (i % 16 == 0) {
                    strcat(cFileContent, "\n    ");
                }
                sprintf(tmp, "0x%02X,", dest[i]);
                strcat(cFileContent, tmp);
            }
            cFileContent[strlen(cFileContent) - 1] = '\0';
            strcat(cFileContent, "};\n");
            //strcpy(cFileContent, )

            // Формируем путь, где будем сохранять пропатченную версию handler.
            origName[strlen(origName) - 3] = 'c';
            origName[strlen(origName) - 2] = '\0';
            utils_build_path(filePath, gCmdOptions.outPath, origName, 0);

            NOISY_PRINTF(("OK\nSaving shellcode to %s... ", filePath));
            if (utils_save_file(filePath, cFileContent, strlen(cFileContent)) != 0) {
                printf("Failed\n  ! Can't save shellcode to file %s\n\n", filePath);
                break;
            }
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

    NOISY_PRINTF(("<-- Shellcode Maker\n"));

    return ret;
}
