#include <Windows.h>
#include <shlwapi.h>
#include <Wininet.h>
#include <stdio.h>
#include <math.h>

#include "../../../shared/platform.h"
#include "../../../shared/types.h"
#include "../../../shared/native.h"
#include "../../../shared/config.h"
#include "../../../shared/utils_cli.h"

#include "../../../shared/zmodules/zmodule.h"

#define ZTRANSFORM_VERSION "0.2.2"

typedef struct cmd_options
{
    char* in;
    char* out;
} cmd_options_t, *pcmd_options_t;

cmd_options_t gCmdOptions;

cmd_line_info_t cmdLineInfo = {2,
{
    {{"i", "in"}, "Input PE file", "Specify nonempty path", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.in},
    {{"o", "out"}, "Output ZModule file", "Specify nonempty path", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.out},
}
};

int main(int argc, char** argv)
{
    int ret = EXIT_FAILURE;
    uint8_t* peBuffer;
    uint16_t nSection;
    size_t peSize;
    PIMAGE_DOS_HEADER pDosHdr;
    PIMAGE_NT_HEADERS pNtHdrs;
    pzmodule_section_header_t pZModuleSectHdr;
    PIMAGE_SECTION_HEADER pSectionHdr;
    PIMAGE_DATA_DIRECTORY pDirectory;
    PIMAGE_DATA_DIRECTORY pExportDirectory = NULL;
    uint32_t zmoduleImageSize;
    uint32_t zmoduleAlignedImageSize;
    uint8_t* zmoduleBuffer;

    memset(&gCmdOptions, 0, sizeof(cmd_options_t));
    if (config_parse_cmdline(argc, argv, &cmdLineInfo, ZTRANSFORM_VERSION) != ERR_OK) {
        return EXIT_FAILURE;
    }

    printf ("\n>>> ztransform\n\n");

    do {
        // Проверяем 
        printf(" Reading %s...", gCmdOptions.in);
        if (utils_file_exists(gCmdOptions.in) != ERR_OK) {
            printf("Error: file not exists\n\n");
            break;
        }

        if (utils_read_file(gCmdOptions.in, &peBuffer, &peSize) != ERR_OK) {
            printf("Error: cannot read file\n\n");
            break;
        }

        pDosHdr = (PIMAGE_DOS_HEADER)peBuffer;
        pNtHdrs = (PIMAGE_NT_HEADERS)(peBuffer + pDosHdr->e_lfanew);

        if (pDosHdr->e_magic != IMAGE_DOS_SIGNATURE && pNtHdrs->Signature != IMAGE_NT_SIGNATURE) {
            printf("Error: not PE file\n\n");
            break;
        }

        if (pNtHdrs->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 && pNtHdrs->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) {
            printf("Error: unsupported machine type\n\n");
            break;
        }

        printf("OK\n Transformating...");

        zmoduleBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1024 * 1024);
        if (pNtHdrs->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) {
            pzmodule_header64_t pZModuleHdr;
            PIMAGE_NT_HEADERS64 pNtHdrs64 = (PIMAGE_NT_HEADERS64)pNtHdrs;
            zmoduleImageSize = sizeof(zmodule_header64_t) + sizeof(zmodule_section_header_t) * pNtHdrs->FileHeader.NumberOfSections;
            pZModuleHdr = (pzmodule_header64_t)zmoduleBuffer;
            pZModuleHdr->numberOfSections = (uint8_t)pNtHdrs64->FileHeader.NumberOfSections;
            pZModuleHdr->sizeOfBaseHeader = sizeof(zmodule_header64_t);
            pZModuleHdr->sizeOfHeaders = zmoduleImageSize;
            pZModuleHdr->sizeOfImage = pNtHdrs64->OptionalHeader.SizeOfImage;
            pZModuleHdr->imageBase = ((PIMAGE_NT_HEADERS64)pNtHdrs64)->OptionalHeader.ImageBase;
            switch (pNtHdrs64->FileHeader.Machine) {
                case IMAGE_FILE_MACHINE_AMD64: pZModuleHdr->machine = ZMODULE_MACHINE_AMD64; break;
                case IMAGE_FILE_MACHINE_I386: pZModuleHdr->machine = ZMODULE_MACHINE_IA32; break;
                case IMAGE_FILE_MACHINE_IA64: pZModuleHdr->machine = ZMODULE_MACHINE_IA64; break;
            }
            zmoduleAlignedImageSize = ALIGN_UP_BY(zmoduleImageSize, 0x1000);
            pSectionHdr = IMAGE_FIRST_SECTION(pNtHdrs64);
            pZModuleSectHdr = (pzmodule_section_header_t)(zmoduleBuffer + sizeof(zmodule_header64_t));

            if (pNtHdrs64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress != 0) {
                pDirectory = &pNtHdrs64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
                pZModuleHdr->dataDirectory[ZMODULE_DIRECTORY_ENTRY_BASERELOC].size = pDirectory->Size;
                pZModuleHdr->dataDirectory[ZMODULE_DIRECTORY_ENTRY_BASERELOC].virtualAddress = pDirectory->VirtualAddress;
            }
            if (pNtHdrs64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress != 0) {
                pExportDirectory = &pNtHdrs64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
                pZModuleHdr->dataDirectory[ZMODULE_DIRECTORY_ENTRY_EXPORT].size = pExportDirectory->Size;
                pZModuleHdr->dataDirectory[ZMODULE_DIRECTORY_ENTRY_EXPORT].virtualAddress = pExportDirectory->VirtualAddress;
            }
        }
        else if (pNtHdrs->FileHeader.Machine == IMAGE_FILE_MACHINE_I386) {
            pzmodule_header32_t pZModuleHdr;
            zmoduleImageSize = sizeof(zmodule_header32_t) + sizeof(zmodule_section_header_t) * pNtHdrs->FileHeader.NumberOfSections;
            pZModuleHdr = (pzmodule_header32_t)zmoduleBuffer;
            pZModuleHdr->numberOfSections = (uint8_t)pNtHdrs->FileHeader.NumberOfSections;
            pZModuleHdr->sizeOfBaseHeader = sizeof(zmodule_header32_t);
            pZModuleHdr->sizeOfHeaders = zmoduleImageSize;
            pZModuleHdr->sizeOfImage = pNtHdrs->OptionalHeader.SizeOfImage;
            pZModuleHdr->imageBase = ((PIMAGE_NT_HEADERS32)pNtHdrs)->OptionalHeader.ImageBase;
            switch (pNtHdrs->FileHeader.Machine) {
                case IMAGE_FILE_MACHINE_AMD64: pZModuleHdr->machine = ZMODULE_MACHINE_AMD64; break;
                case IMAGE_FILE_MACHINE_I386: pZModuleHdr->machine = ZMODULE_MACHINE_IA32; break;
                case IMAGE_FILE_MACHINE_IA64: pZModuleHdr->machine = ZMODULE_MACHINE_IA64; break;
            }
            zmoduleAlignedImageSize = ALIGN_UP_BY(zmoduleImageSize, 0x1000);
            pSectionHdr = IMAGE_FIRST_SECTION(pNtHdrs);
            pZModuleSectHdr = (pzmodule_section_header_t)(zmoduleBuffer + sizeof(zmodule_header32_t));

            if (pNtHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress != 0) {
                pDirectory = &((PIMAGE_NT_HEADERS32)pNtHdrs)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
                pZModuleHdr->dataDirectory[ZMODULE_DIRECTORY_ENTRY_BASERELOC].size = pDirectory->Size;
                pZModuleHdr->dataDirectory[ZMODULE_DIRECTORY_ENTRY_BASERELOC].virtualAddress = pDirectory->VirtualAddress;
            }
            if (pNtHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress != 0) {
                pExportDirectory = &((PIMAGE_NT_HEADERS32)pNtHdrs)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
                pZModuleHdr->dataDirectory[ZMODULE_DIRECTORY_ENTRY_EXPORT].size = pExportDirectory->Size;
                pZModuleHdr->dataDirectory[ZMODULE_DIRECTORY_ENTRY_EXPORT].virtualAddress = pExportDirectory->VirtualAddress;
            }
        }

        for (nSection = 0; nSection < pNtHdrs->FileHeader.NumberOfSections; ++nSection, ++pSectionHdr, ++pZModuleSectHdr) {
            uint8_t* pNull;
            pZModuleSectHdr->sizeOfRawData = pSectionHdr->SizeOfRawData;
            pZModuleSectHdr->pointerToRawData = (pSectionHdr->PointerToRawData != 0 ? zmoduleImageSize : 0);
            pZModuleSectHdr->virtualAddress = zmoduleAlignedImageSize;
            __movsb(zmoduleBuffer + pZModuleSectHdr->pointerToRawData, peBuffer + pSectionHdr->PointerToRawData, pSectionHdr->SizeOfRawData);
            for (pNull = zmoduleBuffer + pZModuleSectHdr->pointerToRawData + pSectionHdr->SizeOfRawData - 1; *pNull == 0; --pNull) {
                --pZModuleSectHdr->sizeOfRawData;
            }

            // Обрабатываем таблицу экспорта.
            if (pExportDirectory != NULL && pExportDirectory->VirtualAddress >= pSectionHdr->VirtualAddress &&
                pExportDirectory->VirtualAddress < (pSectionHdr->VirtualAddress + pSectionHdr->SizeOfRawData)) {
                PIMAGE_EXPORT_DIRECTORY pExport = (PIMAGE_EXPORT_DIRECTORY)(zmoduleBuffer + pZModuleSectHdr->pointerToRawData + (pExportDirectory->VirtualAddress - pSectionHdr->VirtualAddress));
                char* name = (char*)(zmoduleBuffer + pZModuleSectHdr->pointerToRawData + (pExport->Name - pSectionHdr->VirtualAddress));
                // Затираем имя модуля.
                for ( ; *name != '\0'; ++name) {
                    *name = '\0';
                }
                pExport->Name = 0;
                pExport->TimeDateStamp = 0;
                pExport->Characteristics = 0;
                pExport->MinorVersion = pExport->MajorVersion = 0;
            }

            zmoduleImageSize += pZModuleSectHdr->sizeOfRawData;
            zmoduleAlignedImageSize += ALIGN_UP_BY(pSectionHdr->Misc.VirtualSize, 0x1000);
        }

        printf("OK\n Saving ZModule to %s...", gCmdOptions.out);
        if (utils_save_file(gCmdOptions.out, zmoduleBuffer, zmoduleImageSize) != ERR_OK) {
            printf("Error: cannot save\n\n");
            break;
        }
        printf("OK\n");

        ret = EXIT_SUCCESS;
    } while (0);

    printf ("\n<<< ztransform\n");

    return ret;
}
