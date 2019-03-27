#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "platform.h"
#include "types.h"
#include "utils_cli.h"
#include "config.h"

int internalHandleKey(char** pKey, int type, void* outVal)
{
    char *itr, *itrEnd;
    size_t valLen;
    char *val = NULL, *valItr;
    int realType = type & 0x7f;

    if (pKey != NULL && *pKey != NULL && outVal != NULL) {
        itr = *pKey;

        for ( ; *itr == ' ' && *itr != '\n' && *itr != '\r' && *itr != 0; ++itr);
        if (*itr == '\n' || *itr == '\r' || *itr == 0) {
            return ERR_BAD;
        }

        if (realType != TYPE_STRING && realType != TYPE_STRING_PATH && realType != TYPE_STRING_ARRAY) {
            for (itrEnd = itr; *itrEnd != 0 && *itrEnd != '\r' && *itrEnd != '\n' && *itrEnd != ' ' && *itrEnd != '\t'; ++itrEnd);
        }
        else {
            if ((type & TYPE_CMDLINE) != TYPE_CMDLINE) {
                if (*itr != '"') {
                    return ERR_BAD;
                }
                ++itr;
                for (itrEnd = itr; *itrEnd != '\0' && *itrEnd != '\r' && *itrEnd != '\n'; ++itrEnd);
                for ( ; *(--itrEnd) != '"'; );
            }
            else {
                for (itrEnd = itr; *itrEnd != '\0'; ++itrEnd);
            }
        }

        valLen = itrEnd - itr;
        if (valLen <= 0) {
            return ERR_BAD;
        }
            
        if (realType == TYPE_STRING_PATH) {
            valLen += 2;
        }
        else {
            ++valLen;
        }

        val = (char*)calloc(1, valLen);
        if (val == NULL) {
            return ERR_BAD;
        }

        valItr = val;
        if (realType != TYPE_STRING && realType != TYPE_STRING_PATH && realType != TYPE_STRING_ARRAY) {
            for ( ; *itr != 0 && *itr != '\n' && *itr != '\r' && *itr != ' ' && *itr != '\t'; ++itr) {
                *valItr++ = *itr;
            }
        }
        else {
            if ((type & TYPE_CMDLINE) != TYPE_CMDLINE) {
                for ( ; *itr != '\0' && *itr != '\r' && *itr != '\n'; ++itr) {
                    *valItr++ = *itr;
                }
                for ( ; *(--valItr) != '"'; );
                *valItr = '\0';
            }
            else {
                for ( ; *itr != '\0'; ++itr) {
                    *valItr++ = *itr;
                }
            }

            // Особый случай для TYPE_STRING_PATH, когда путь должен оканчиваться на /.
            if (realType == TYPE_STRING_PATH && *(valItr - 1) != '/') {
                *valItr = '/';
            }
        }

        *pKey = itr;

        if (realType == TYPE_STRING || realType == TYPE_STRING_PATH || realType == TYPE_STRING_ARRAY) {
            *((char**)outVal) = val;
        }
        else if (realType == TYPE_SIZET) {
#if _OS == _OS_WINDOWS_NT
            *((size_t*)outVal) = (size_t)_atoi64(val);
#else
            *((size_t*)outVal) = (size_t)atoll(val);
#endif
            free(val);
        }
        else if (realType == TYPE_UINT32) {
#if _OS == _OS_WINDOWS_NT
            *((uint32_t*)outVal) = (uint32_t)_atoi64(val);
#else
            *((uint32_t*)outVal) = (uint32_t)atoll(val);
#endif
            free(val);
        }
        else if (realType == TYPE_UINT16) {
            int preValue;
            preValue = atoi(val);
            free(val);

            if (preValue <= 0 || (preValue > 65535))
                return ERR_BAD;
            
            *((uint16_t*)outVal) = (uint16_t)preValue;
        }
    }

    return ERR_OK;
}

char* line_parser_conf(char** pItr, char* end)
{
    char *pKey = NULL;
    char replaced;
    char *keyBeg, *keyEnd;
    char* itr = *pItr;

    for ( ; !isalpha(*itr) && *itr != '#' && itr < end; ++itr);

    do {
        if (*itr == '#') {
            for ( ; *itr != '\n' && itr < end; ++itr);
            break;
        }
        keyBeg = itr;
        for ( ; *itr != '=' && *itr != '\n' && *itr != '\t' && *itr != ' ' && itr < end; ++itr);
        keyEnd = itr;
        if (*itr != '=') {
            for ( ; *itr != '=' && *itr != '\n' && itr < end; ++itr);
            if (*itr != '=' || *itr == '\n')
                break;
        }
        itr++;
        replaced = *keyEnd;
        *keyEnd = 0;

        pKey = (char*)malloc(keyEnd - keyBeg + 1);
        strcpy(pKey, keyBeg);

        *keyEnd = replaced;
    } while (0);

    *pItr = itr;

    return pKey;
}

void line_parser_conf_done()
{
}

char* pSection = NULL;

char* line_parser_ini(char** pItr, char* end)
{
    char* pKey = NULL;
    char replaced;
    char *keyBeg, *keyEnd;
    char* itr = *pItr;

    for ( ; isspace(*itr) && itr < end; ++itr);

    do {
        if (itr < end) {
            if (*itr == ';') {
                for ( ; *itr != '\n' && *itr != '\r' && itr < end; ++itr);
            }
            else if (*itr == '[') {
                keyBeg = ++itr;
                for ( ; *itr != ']' && *itr != '\n' && *itr != '\t' && *itr != ' ' && itr < end; ++itr);
                keyEnd = itr;
                if (*itr != ']') {
                    for ( ; *itr != ']' && *itr != '\n' && itr < end; ++itr);
                    if (*itr != ']' || *itr == '\n')
                        break;
                }
                ++itr;
                replaced = *keyEnd;
                *keyEnd = 0;

                if (pSection != NULL) {
                    free(pSection);
                }

                pSection = (char*)malloc(keyEnd - keyBeg + 1);
                strcpy(pSection, keyBeg);

                *keyEnd = replaced;

                for ( ; (*itr == '\n' || *itr == '\r' || *itr == ' ' || *itr == '\t') && itr < end; ++itr);
            }
            else {
                keyBeg = itr;
                for ( ; *itr != '=' && *itr != '\r' && *itr != '\n' && *itr != '\t' && *itr != ' ' && itr < end; ++itr);
                keyEnd = itr;
                if (*itr != '=') {
                    for ( ; *itr != '=' && *itr != '\r' && *itr != '\n' && itr < end; ++itr);
                    if (*itr != '=' || *itr == '\n') {
                        if (pKey != NULL) {
                            free(pKey);
                            pKey = NULL;
                        }
                        break;
                    }
                }
                ++itr;
                replaced = *keyEnd;
                *keyEnd = 0;


                if (pSection != NULL) {
                    pKey = (char*)malloc(strlen(pSection) + keyEnd - keyBeg + 1);
                    strcpy(pKey, pSection);
                    strcat(pKey, keyBeg);
                }
                else {
                    pKey = (char*)malloc(keyEnd - keyBeg + 1);
                    strcpy(pKey, keyBeg);
                }

                *keyEnd = replaced;
            }
        }
    } while (0);

    *pItr = itr;

    return pKey;
}

void line_parser_ini_done()
{
    if (pSection != NULL) {
        free(pSection);
        pSection = NULL;
    }
}

int config_load_from_buffer(char* data, size_t len, Fnline_parser_conf fnLineParser, Fnline_parser_conf_done fnLineParserDone, FnConfigHandler fnConfigHandler)
{
    char *itr, *end, *pKey;
    int valueType;
    void* pVal;

    itr = data;
    end = data + len;
    for ( ; itr < end; ) {
        pKey = fnLineParser(&itr, end);
        if (pKey == NULL) {
            continue;
        }

        if ((pVal = fnConfigHandler(pKey, &valueType)) == NULL) {
            for ( ; *itr != '\r' && *itr != '\n'; ++itr);
            continue;
        }

        free(pKey);

        internalHandleKey(&itr, valueType, pVal);

        ++itr;
    }

    fnLineParserDone();

    return ERR_OK;
}

int config_load(const char* confPath, Fnline_parser_conf fnLineParser, Fnline_parser_conf_done fnLineParserDone, FnConfigHandler fnConfigHandler)
{
    char* data;
    size_t len;
    int ret;

    if (utils_read_file(confPath, (uint8_t**)&data, &len) != 0) {
        return ERR_BAD;
    }

    ret = config_load_from_buffer(data, len, fnLineParser, fnLineParserDone, fnConfigHandler);

    free(data);
    return ret;
}

void config_print_cmd_usage(char** argv, pcmd_line_info_t pCmdLineInfo, const char* appVersion)
{
    int i;
    char* appName = argv[0] + strlen(argv[0]);

    for ( ; appName > argv[0] && *appName != '\\' && *appName != '/'; --appName);

    if (appName > argv[0]) {
        ++appName;
    }

    printf("Version %s\n\n", appVersion);
    printf("Example: %s [options]\n\n", appName);
    printf("Options:\n");

    for (i = 0; i < pCmdLineInfo->optionsCount; ++i) {
        printf("\t-%s\t%s\n", pCmdLineInfo->cmdOptionHelpers[i].shortLongName[0], pCmdLineInfo->cmdOptionHelpers[i].description);
    }
}

int config_parse_cmdline(int argc, char** argv, pcmd_line_info_t pCmdLineInfo, const char* appVersion)
{
    char endCh, *ptr, *end;
    int i, j, l;
    uint8_t optFlags;

//     if (argc == 1) {
//         config_print_cmd_usage(argv, pCmdLineInfo, appVersion);
//         return ERR_BAD;
//     }

    // Инициализируем булевы параметры командной строки.
    for (i = 0; i < pCmdLineInfo->optionsCount; ++i) {
        if (pCmdLineInfo->cmdOptionHelpers[i].valueType == TYPE_BOOLEAN) {
            *(int*)pCmdLineInfo->cmdOptionHelpers[i].pValue = FALSE;
        }
    }

    for (i = 0; i < pCmdLineInfo->optionsCount; ++i) {
        optFlags = pCmdLineInfo->cmdOptionHelpers[i].flags;
        for (j = 1; j < argc; ++j) {
            if (argv[j][0] == '-') {
                if (argv[j][1] == '-') {
                    l = 1;
                }
                else {
                    l = 0;
                }

                ptr = end = &argv[j][1 + l];
                for ( ; *end != '\0' && *end != '='; ++end);
                endCh = *end;
                if (endCh != '\0') {
                    *end = '\0';
                }

                if (strcmp(ptr, "h") == 0) {
                    config_print_cmd_usage(argv, pCmdLineInfo, appVersion);
                    return ERR_BAD;
                }
                else if (strcmp(ptr, pCmdLineInfo->cmdOptionHelpers[i].shortLongName[l]) == 0) {
                    if (endCh != '\0') {
                        ptr = end + 1;
                        *end = endCh;
                    }

                    if ((pCmdLineInfo->cmdOptionHelpers[i].valueType & TYPE_STRING_ARRAY) == TYPE_STRING_ARRAY) {
                        char** arr;
                        int currIndex;

                        arr = *(char***)pCmdLineInfo->cmdOptionHelpers[i].pValue;
                        if (arr == NULL) {
                            pCmdLineInfo->cmdOptionHelpers[i].arraySize = 1;
                            arr = (char**)calloc(1, sizeof(char*));
                            currIndex = 0;
                        }
                        else {
                            currIndex = pCmdLineInfo->cmdOptionHelpers[i].arraySize++;
                            arr = (char**)realloc(arr, sizeof(char*) * pCmdLineInfo->cmdOptionHelpers[i].arraySize);
                            arr[currIndex] = NULL;
                        }

                        if (internalHandleKey(&ptr, pCmdLineInfo->cmdOptionHelpers[i].valueType, &arr[currIndex]) == ERR_BAD) {
                            fprintf(stderr, "%s: %s. Use -h for help.\n", pCmdLineInfo->cmdOptionHelpers[i].shortLongName[1], pCmdLineInfo->cmdOptionHelpers[i].badDescription);
                            return ERR_BAD;
                        }

                        *(char***)pCmdLineInfo->cmdOptionHelpers[i].pValue = arr;
                    }
                    else if ((pCmdLineInfo->cmdOptionHelpers[i].valueType & TYPE_BOOLEAN) != TYPE_BOOLEAN) {
                        if (internalHandleKey(&ptr, pCmdLineInfo->cmdOptionHelpers[i].valueType, pCmdLineInfo->cmdOptionHelpers[i].pValue) == ERR_BAD) {
                            fprintf(stderr, "%s: %s. Use -h for help.\n", pCmdLineInfo->cmdOptionHelpers[i].shortLongName[1], pCmdLineInfo->cmdOptionHelpers[i].badDescription);
                            return ERR_BAD;
                        }
                        break;
                    }
                    else {
                        *(int*)pCmdLineInfo->cmdOptionHelpers[i].pValue = TRUE;
                        break;
                    }
                    
                }
                else {
                    *end = endCh;
                }
            }
        }
        if (j >= argc) {
            if ((optFlags == OPT_FLAG_REQUIRED && (pCmdLineInfo->cmdOptionHelpers[i].valueType & TYPE_STRING_ARRAY) == TYPE_STRING_ARRAY && pCmdLineInfo->cmdOptionHelpers[i].arraySize == 0) ||
                (optFlags == OPT_FLAG_REQUIRED && (pCmdLineInfo->cmdOptionHelpers[i].valueType & TYPE_STRING_ARRAY) != TYPE_STRING_ARRAY)) {
                fprintf(stderr, "Option '%s' is required. Use -h for help.\n", pCmdLineInfo->cmdOptionHelpers[i].shortLongName[1]);
                return ERR_BAD;
            }
        }
    }

    return ERR_OK;
}
