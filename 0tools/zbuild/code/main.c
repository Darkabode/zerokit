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

#define ZBUILD_VERSION "0.5.4"

typedef struct _global_config
{
    char* urls_params;
    char* urls_link;
    char* urls_file;
    char* urls_proto;
    char* urls_domain;
    char* urls_uri;
    char* preCommands[64];
    int preCount;
    char* buildParams[64];
    int buildCount;
    char* postCommands[64];
    int postCount;
} global_config_t;

global_config_t gConfigParams;

void* configHandler(char* keyName, int* pValueType)
{
    void* pVal = NULL;

    if (gConfigParams.urls_params == NULL && strcmp(keyName, "mainurls_param") == 0) {
        pVal = (void*)&gConfigParams.urls_params; *pValueType = TYPE_STRING;
    }
    else if (gConfigParams.urls_link == NULL && strcmp(keyName, "mainurls_link") == 0) {
        pVal = (void*)&gConfigParams.urls_link; *pValueType = TYPE_STRING;
    }
    else if (gConfigParams.urls_file == NULL && strcmp(keyName, "mainurls_file") == 0) {
        pVal = (void*)&gConfigParams.urls_file; *pValueType = TYPE_STRING;
    }
    else if (gConfigParams.urls_proto == NULL && strcmp(keyName, "mainurls_proto") == 0) {
        pVal = (void*)&gConfigParams.urls_proto; *pValueType = TYPE_STRING;
    }
    else if (gConfigParams.urls_domain == NULL && strcmp(keyName, "mainurls_domain") == 0) {
        pVal = (void*)&gConfigParams.urls_domain; *pValueType = TYPE_STRING;
    }
    else if (gConfigParams.urls_uri == NULL && strcmp(keyName, "mainurls_uri") == 0) {
        pVal = (void*)&gConfigParams.urls_uri; *pValueType = TYPE_STRING;
    }
    else if (strcmp(keyName, "precommand") == 0) {
        pVal = (void*)&gConfigParams.preCommands[gConfigParams.preCount++]; *pValueType = TYPE_STRING;
    }
    else if (strcmp(keyName, "postcommand") == 0) {
        pVal = (void*)&gConfigParams.postCommands[gConfigParams.postCount++]; *pValueType = TYPE_STRING;
    }
    else if (strcmp(keyName, "buildparams") == 0) {
        pVal = (void*)&gConfigParams.buildParams[gConfigParams.buildCount++]; *pValueType = TYPE_STRING;
    }

    return pVal;
}


typedef struct cmd_options
{
    char* params;
} cmd_options_t, *pcmd_options_t;

cmd_options_t gCmdOptions;

cmd_line_info_t cmdLineInfo = {1,
{
    {{"p", "params"}, "Build parameters", "", OPT_FLAG_OPTIONAL, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.params},
}
};

const char CONFIG_FILE[] = "zbuild.conf";

int main(int argc, char** argv)
{
    int i;
    int ret = EXIT_FAILURE;
    char command[1024];

    memset(&gConfigParams, 0, sizeof(gConfigParams));
    memset(&gCmdOptions, 0, sizeof(cmd_options_t));
    if (config_parse_cmdline(argc, argv, &cmdLineInfo, ZBUILD_VERSION) != ERR_OK) {
        return EXIT_FAILURE;
    }

    do {
        // Проверяем 
        if (utils_file_exists(CONFIG_FILE) != ERR_OK) {
            break;
        }

        utils_create_directory("autobuild");
        utils_create_directory("autobuild\\x32");
        utils_create_directory("autobuild\\x64");

        printf("Loading %s...", CONFIG_FILE);
        if (config_load(CONFIG_FILE, line_parser_ini, line_parser_ini_done, configHandler) != ERR_OK) {
            printf("Failed: cannot read\n\n");
            break;
        }

        printf("OK\n");

        for (i = 0; i < gConfigParams.preCount; ++i) {
            uint32_t exitCode = 0;
            printf("Run: %s:\n", gConfigParams.preCommands[i]);
            if (!utils_launch_and_verify(gConfigParams.preCommands[i], &exitCode) && exitCode != 0) {
                printf(("Failed\n"));
                break;
            }
            printf("OK\n");
        }

        if (i < gConfigParams.preCount) {
            break;
        }

        if (gConfigParams.urls_link != NULL) {
            char* urlsNamesBuffer;
            char* urlsBuffer;
            char* ptr;
            size_t sz, newSz;
            printf("Downloading URLs:\n");
            sprintf_s(command, sizeof(command) - 1, "curl.exe %s -o urls.names \"%s\"", gConfigParams.urls_params, gConfigParams.urls_link);
            if (!utils_launch_and_verify(command, NULL)) {
                printf(("Failed: cannot download\n"));
                break;
            }

            if (utils_read_file("urls.names", &urlsNamesBuffer, &sz) != ERR_OK) {
                printf(("Failed: cannot read names from file\n"));
                break;
            }

            newSz = strlen(gConfigParams.urls_domain);
            if (gConfigParams.urls_proto != NULL) {
                newSz += strlen(gConfigParams.urls_proto);
            }
            if (gConfigParams.urls_uri != NULL) {
                newSz += strlen(gConfigParams.urls_uri);
            }
            newSz = 2 * newSz + sz + 1;
            urlsBuffer = malloc(newSz);
            memset(urlsBuffer, 0, newSz);
            for (ptr = urlsNamesBuffer; *ptr != '\0'; ++ptr) {
                char* name = ptr;
                if (gConfigParams.urls_proto != NULL) {
                    strcat(urlsBuffer, gConfigParams.urls_proto);
                }
                for ( ; *ptr != ';'; ++ptr); *ptr = '\0';
                strcat(urlsBuffer, name);
                strcat(urlsBuffer, gConfigParams.urls_domain);
                if (gConfigParams.urls_uri != NULL) {
                    strcat(urlsBuffer, gConfigParams.urls_uri);
                }
                strcat(urlsBuffer, ";");
            }

            if (utils_save_file("urls.in", urlsBuffer, strlen(urlsBuffer)) != ERR_OK) {
                printf(("Failed: cannot save complete URLs\n"));
                break;
            }

            free(urlsNamesBuffer);
            free(urlsBuffer);

            printf("OK\n");
        }

        printf("Generating module config...");
        if (!utils_launch_and_verify("confgen.exe", NULL)) {
            printf(("Failed\n"));
            break;
        }

        for (i = 0; i < gConfigParams.buildCount; ++i) {
            uint32_t exitCode = 0;
            char temp[256];
            sprintf_s(command, sizeof(command) - 1, "cmd.exe /C \"build.cmd %s", gConfigParams.buildParams[i]);
            if (gCmdOptions.params != NULL) {
                sprintf_s(temp, sizeof(temp) - 1, " %s", gCmdOptions.params);
                strcat(command, temp);
            }
            strcat(command, "\"");
            printf("Run: %s...", command);
            if (!utils_launch_and_verify(command, &exitCode) && exitCode != 0) {
                printf(("Failed\n"));
                break;
            }
            printf("OK\n");
        }

        if (i < gConfigParams.buildCount) {
            break;
        }

        for (i = 0; i < gConfigParams.postCount; ++i) {
            uint32_t exitCode = 0;
            printf("Run: %s:\n", gConfigParams.postCommands[i]);
            if (!utils_launch_and_verify(gConfigParams.postCommands[i], &exitCode) && exitCode != 0) {
                printf(("Failed\n"));
                break;
            }
            printf("OK\n");
        }

        if (i < gConfigParams.postCount) {
            break;
        }

        ret = EXIT_SUCCESS;
    } while (0);

    return ret;
}
