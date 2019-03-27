#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>

#include "../../shared_code/platform.h"
#include "../../shared_code/types.h"
#include "../../shared_code/native.h"
#include "../../shared_code/utils.h"
#include "../../shared_code/crc64.h"
#include "../../shared_code/config.h"
#include "../../shared_code/lzma.h"

#define USE_LZMA_DECOMPRESSOR 1
#include "../../shared_code/lzma.c"

#define AHELPER_VERSION "0.5.2"

char* gPackPath;
char* gOutPath;

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

int LZMADecompress(pvoid_t inStream, size_t inSize, pvoid_t* outStream, size_t* poutSize)
{
    size_t outSize, origOutSize;
    int ret = 1;
    int st;

    do {
        outSize = inSize;
        do {
            if (*outStream != NULL) {
                free(*outStream);
            }
            outSize *= 2;
            origOutSize = outSize;
            *outStream = malloc(outSize);
            ret = lzma_decode(*outStream, &outSize, inStream, inSize, &st);
        } while (ret == ERR_OK && outSize == origOutSize);

        *poutSize = outSize;
    } while (0);

    return ret;
}

typedef struct _file_header
{
    uint64_t fileCRC;  // CRC64 вычисленное по файлу.
    uint32_t fileSize; // Размер файла.
    char name[64];     // Имя файла в формате .
} file_header_t, *pfile_header_t;

int process_response_file()
{
	uint32_t i, j;
	char *packedData, *zrPack = NULL;
	size_t packedSize, zrPackSize;
	char* fullFilePath = NULL;
	pzr_header_t pZrHeader;
	pgroup_header_t pGroupHeader = NULL;
	pfile_header_t pFileHeader = NULL;
	uint32_t realCRC[2];
	int ret = ERR_BAD, dErr;
	
	if (utils_read_file(gPackPath, &packedData, &packedSize) != ERR_OK) {
		printf("fatal_err:Can't read pack\n");
		return ret;
	}

	dErr = LZMADecompress(packedData, packedSize, &zrPack, &zrPackSize);

       free(packedData);

       if (dErr == 0) {
		pZrHeader = (pzr_header_t)zrPack;
		if (pZrHeader->signature != 0x3937525A) {
			printf("fatal_err:Pack is not valid\n");
			goto exit;
		}

		pGroupHeader = (pgroup_header_t)(zrPack + sizeof(zr_header_t));	

		fullFilePath = malloc(MAX_PATH);
		crc64_buildtable();
	
		for (i = 0; i < pZrHeader->groupsCount; ++i) {
			if (pGroupHeader->filesCount == 0) {
				printf("fatal_err:Pack is not valid\n");
				goto exit;
			}

			pFileHeader = (pfile_header_t)((uint8_t*)pGroupHeader + sizeof(group_header_t));
			for (j = 0; j < pGroupHeader->filesCount; ++j) {
				char* ptr;
				uint32_t counter = 0;
				uint8_t* fileData = (uint8_t*)pFileHeader + sizeof(file_header_t);

				utils_build_path(fullFilePath, gOutPath, pFileHeader->name, 0);

				ptr = pFileHeader->name;
				for ( ; *ptr != '\0'; ++ptr) {
					if (*ptr == '_' && counter == 2) {
						*ptr = '\0';
						break;
					}
					else if (*ptr == '_') {
						*ptr = ':';
						++counter;
					}
				}
			
				crc64_computate(fileData, pFileHeader->fileSize, realCRC);
				if (memcmp(&pFileHeader->fileCRC, realCRC, sizeof(realCRC)) == 0) {
					if (utils_save_file(fullFilePath, fileData, pFileHeader->fileSize) != ERR_OK) {
						printf("err:%s\n", pFileHeader->name);
					}
					else {
						printf("ok:%s\n", pFileHeader->name);
					}	
				}
				else {
					printf("err:%s\n", pFileHeader->name);
				}
			
				pFileHeader = (pfile_header_t)((uint8_t*)pFileHeader + sizeof(file_header_t) + pFileHeader->fileSize);
			}
			pGroupHeader = (pgroup_header_t)((uint8_t*)pFileHeader);
		}

		ret = ERR_OK;
	}
	else {
		printf("fatal_err:Can't decompress pack\n");
	}
exit:
	if (zrPack != NULL) {
		free (zrPack);
	}

	if (fullFilePath != NULL) {
		free(fullFilePath);
	}

	return ret;
}

#define USAGE printf("ahelper v%s\nUsage:\n\n\tahelper <cmd_id> <pack_path> <out_path>\n\n", AHELPER_VERSION);

int main(int argc, char** argv)
{
	int ret = 1;
	int cmdId;

	if (argc < 4) {
		USAGE;
		return 1;
	}
	
	cmdId = atoi(argv[1]);
	gPackPath = argv[2];
	gOutPath = argv[3];

	if (cmdId == 1) {
		if (utils_is_file_exists(gPackPath) != ERR_OK) {
			printf("fatal_err:Path '%s' not exists\n", gPackPath);
			return 1;
		}

		if (utils_is_file_exists(gOutPath) != ERR_OK) {
			printf("fatal_err:Path '%s' not exists\n", gOutPath);
			return 1;
		}

		ret = process_response_file();
	}
	else {
		printf("fatal_err:Unknown command id\n");
	}	

	return ret;
}
