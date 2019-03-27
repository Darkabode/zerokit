#include "overlord.h"

#include "shell.h"

char shellExeName[17];

int shell_check_presence()
{
    int ret = 0;
    HKEY hKey = NULL;
    static const char* key = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";
    char value[1024];
    DWORD type = REG_NONE;
    char* baseName;
    DWORD bufferSize = sizeof(value);
    int isHklm = 0;

    __stosb((uint8_t*)value, 0, sizeof(value));
    __stosb((uint8_t*)shellExeName, 0, sizeof(shellExeName));
    
    do {
        if (RegOpenKeyExA(HKEY_CURRENT_USER, key, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
                break;
            }
            isHklm = 1;
        }

        if (RegQueryValueExA(hKey, "Shell", 0, &type, (LPBYTE)value, &bufferSize) != ERROR_SUCCESS) {
            if (isHklm) {
                break;
            }

            RegCloseKey(hKey);
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
                break;
            }

            if (RegQueryValueExA(hKey, "Shell", 0, &type, (LPBYTE)value, &bufferSize) != ERROR_SUCCESS) {
                break;
            }
        }

        if (lstrlenA(value) == 0) {
            break;
        }

        baseName = (char*)utils_get_base_name(value, NULL);

        ret = lstrlenA(baseName);
        if (ret == 0) {
            ret = 0;
            break;
        }
 
        __movsb((uint8_t*)shellExeName, (const uint8_t*)baseName, ret > 16 ? 16 : ret);
        utils_to_lower(shellExeName);

        ret = 1;
    } while (0);

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return ret;
}
