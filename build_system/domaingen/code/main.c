#include <Windows.h>
#include <stdio.h>
#include <math.h>

#include "../../../shared_code/platform.h"
#include "../../../shared_code/types.h"
#include "../../../shared_code/native.h"
#include "../../../shared_code/config.h"

#include "../../../shared_code/bignum.h"
#include "../../../shared_code/bn_mul.h"
#include "../../../shared_code/rsa.h"

#define MPI_USE_FILE_IO 1
#define USE_STRING_IO 1
#define USE_STRING_WRITE_IO 1
#include "../../../shared_code/bignum.c"
#include "../../../shared_code/rsa.c"

#ifndef RANDOM_AUX_VARY
uint32_t randomAuxVarY;
#define RANDOM_AUX_VARY randomAuxVarY
#endif

#ifndef RANDOM_CONSTANT_VECTOR
uint32_t randomConstantVector[128];
#define RANDOM_CONSTANT_VECTOR randomConstantVector
#endif // RANDOM_CONSTANT_VECTOR

void crypto_random_init(rsa_context_t* pRsa)
{
    USE_GLOBAL_BLOCK

        MPI_WRITE_BINARY(&pRsa->N, (uint8_t*)RANDOM_CONSTANT_VECTOR, sizeof(RANDOM_CONSTANT_VECTOR));
    RANDOM_AUX_VARY = RANDOM_CONSTANT_VECTOR[127];
}

uint32_t crypto_random(uint32_t* seed)
{
    uint32_t* v1;
    uint32_t result;
    uint32_t v3;
    USE_GLOBAL_BLOCK

        v1 = RANDOM_CONSTANT_VECTOR + (RANDOM_AUX_VARY & 0x7F);
    RANDOM_AUX_VARY = result = *v1;
    v3 = (0x7FFFFFED * (*seed) + 0x7FFFFFC3) % 0x7FFFFFFF;
    *seed = v3;
    *v1 = v3;
    return result;
}

#define DOMAINGEN_VERSION "0.3.5"

typedef int (__stdcall *FnRtlTimeToSecondsSince1970)(__in PLARGE_INTEGER Time, __out uint32_t* ElapsedSeconds);
FnRtlTimeToSecondsSince1970 fnRtlTimeToSecondsSince1970;

uint32_t get_system_time()
{
    SYSTEMTIME sysTime;
    LARGE_INTEGER localTm;
    uint32_t unixTime = 0;

    GetLocalTime(&sysTime);
    SystemTimeToFileTime(&sysTime, (FILETIME*)&localTm);
    fnRtlTimeToSecondsSince1970(&localTm, (PULONG)&unixTime);

    return unixTime;
}

typedef struct cmd_options
{
    uint32_t count;     // Количество доменов.
    uint32_t lowLimit;  // Минимальное количество символов в домене.
    uint32_t hiLimit;   // Максимальное количество символов в домене.
    uint32_t period;    // Период в часах.
    uint32_t date;      // Дата начала генерации домена.
    int verbose;        // Редим вывода информации.
    char* pubKeyPath;   // Путь до файла открытого ключа.
    uint32_t aff_id;    // Идентификатор аффилиэйта.
} cmd_options_t, *pcmd_options_t;

cmd_options_t gCmdOptions;

cmd_line_info_t cmdLineInfo = {8,
{
    {{"a", "affid"}, "Affiliate ID", "Specify integer value", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_UINT32, (void*)&gCmdOptions.aff_id},
    {{"n", "num"}, "Number of names", "Specify integer value", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_UINT32, (void*)&gCmdOptions.count},
    {{"l", "low"}, "Minimum limit for name length", "Specify integer value", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_UINT32, (void*)&gCmdOptions.lowLimit},
    {{"h", "high"}, "Maximum limit for name length", "Specify integer value", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_UINT32, (void*)&gCmdOptions.hiLimit},
    {{"p", "period"}, "Unique period for name in hours", "Specify integer value", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_UINT32, (void*)&gCmdOptions.period},
    {{"d", "date"}, "Start date in UnixTime format (UTC+0)", "Specify integer value", OPT_FLAG_OPTIONAL, TYPE_CMDLINE | TYPE_UINT32, (void*)&gCmdOptions.date},
    {{"k", "key"}, "Path to public key file", "Specify correct path to file", OPT_FLAG_REQUIRED, TYPE_CMDLINE | TYPE_STRING, (void*)&gCmdOptions.pubKeyPath},
    {{"v", "verbose"}, "Verbose output mode", "Specify integer value", OPT_FLAG_OPTIONAL, TYPE_CMDLINE | TYPE_BOOLEAN, (void*)&gCmdOptions.verbose},
}
};


void checkLimit(short* pLower, short* pHigher, short limit)
{
    if (*pLower >= limit) {
        pHigher += (short)(*pLower / limit);
        *pLower   = (short)(*pLower % limit);
    }
}

int isLeapYear(int year)
{
    return (year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0);
}

int daysOfMonth(int year, int month)
{
    static int daysOfMonthTable[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month == 2 && isLeapYear(year)) {
        return 29;
    }
    else {
        return daysOfMonthTable[month];
    }
}

void printf_date(uint32_t unixTime)
{
    uint64_t timestamp = (uint64_t)unixTime * 1000000;
    int64_t utcTime = timestamp * 10 + (((int64_t)0x01b21dd2) << 32) + 0x13814000;
    int64_t span = utcTime / 10;
    double utcDays = ((double)utcTime) / 864000000000.0;
    double julianDay = utcDays + 2299160.5; // first day of Gregorian reform (Oct 15 1582)
    short  _year;
    short  _month;
    short  _day;
    short  _hour;
    short  _minute;
    short  _second;
    short  _millisecond;
    double z    = floor(julianDay - 1721118.5);
    double r    = julianDay - 1721118.5 - z;
    double g    = z - 0.25;
    double a    = floor(g / 36524.25);
    double b    = a - floor(a/4);
    double c;
    double dday;

    _year       = (short)floor((b + g)/365.25);
    c    = b + z - floor(365.25*_year);
    _month      = (short)floor((5*c + 456)/153);
    dday = c - floor((153.0*_month - 457)/5) + r;
    _day        = (short)dday;
    if (_month > 12) {
        ++_year;
        _month -= 12;
    }
    r      *= 24;
    _hour   = (short)floor(r);
    r      -= floor(r);
    r      *= 60;
    _minute = (short)floor(r);
    r      -= floor(r);
    r      *= 60;
    _second = (short)floor(r);
    r      -= floor(r);
    r      *= 1000;
    _millisecond = (short)floor(r);

    checkLimit(&_millisecond, &_second, 1000);
    checkLimit(&_second, &_minute, 60);
    checkLimit(&_minute, &_hour, 60);
    checkLimit(&_hour, &_day, 24);

    if (_day > daysOfMonth(_year, _month)) {
        _day -= daysOfMonth(_year, _month);
        if (++_month > 12) {
            ++_year;
            _month -= 12;
        }
    }
    _hour        = ((span/3600000000I64) % 24);
    _minute      = (span/(60000000I64)) % 60;
    _second      = (span/(1000000I64)) % 60;
    _millisecond = (span/1000I64) % 1000;

    printf("%02u.%02u.%02u %02u:%02u:%02u GMT", _day, _month, _year, _hour, _minute, _second);
}

int main(int argc, char** argv)
{
    HMODULE hNtdll;
    uint32_t i, j, currPeriod, seed, nameLen;
    char** names;
    FILE* keyFile;
    int err;
    rsa_context_t rsa;

    memset(&gCmdOptions, 0, sizeof(cmd_options_t));
    if (config_parse_cmdline(argc, argv, &cmdLineInfo, DOMAINGEN_VERSION) != ERR_OK) {
        return EXIT_FAILURE;
    }

    if (gCmdOptions.date == 0) {
        gCmdOptions.date = time(NULL);
    }

    hNtdll = LoadLibraryA("ntdll.dll");
    if (hNtdll == NULL) {
        printf("Failed\n  ! Can't load ntdll.dll\n\n");
        return EXIT_FAILURE;
    }

    fnRtlTimeToSecondsSince1970 = (FnRtlTimeToSecondsSince1970)GetProcAddress(hNtdll, "RtlTimeToSecondsSince1970");
    if (fnRtlTimeToSecondsSince1970 == NULL) {
        printf("Failed\n  ! Can't get address of RtlTimeToSecondsSince1970 function\n\n");
        return EXIT_FAILURE;
    }

    if ((keyFile = fopen(gCmdOptions.pubKeyPath, "rb")) == NULL) {
        printf("Failed\n  ! Could not open this file\n\n");
        return EXIT_FAILURE;
    }

    rsa_init(&rsa, RSA_PKCS_V15, 0);

    if ((err = mpi_read_file(&rsa.N , 16, keyFile)) != 0 ||
        (err = mpi_read_file(&rsa.E , 16, keyFile)) != 0)
    {
        printf( "Failed\n  ! Error %d while reading MPI number\n\n", err);
        return EXIT_FAILURE;
    }

    rsa.len = (mpi_msb(&rsa.N) + 7) >> 3;
    fclose(keyFile);

    names = (char**)malloc(sizeof(char*) * gCmdOptions.count);

    currPeriod = (gCmdOptions.date/3600) / gCmdOptions.period;
    for (j = 0; j < gCmdOptions.count; ++j, ++currPeriod) {
        char* name;
        uint32_t minVal, maxVal;
        seed = currPeriod ^ (gCmdOptions.aff_id | (gCmdOptions.aff_id << 16));

        crypto_random_init(&rsa);

        for (i = 0; i < 776; ++ i) {
            crypto_random(&seed);
        }

        minVal = gCmdOptions.lowLimit + (crypto_random(&seed) % (gCmdOptions.hiLimit - gCmdOptions.lowLimit + 1));
        nameLen = gCmdOptions.lowLimit + (crypto_random(&seed) % (gCmdOptions.hiLimit - gCmdOptions.lowLimit + 1));
        maxVal = max(nameLen, minVal);
        if (minVal == maxVal) {
            minVal = nameLen;
        }

        nameLen = minVal + (crypto_random(&seed) % (maxVal - minVal + 1));

        name = (char*)malloc(nameLen + 1);
        __stosb(name, 0, nameLen + 1);

        for (i = 0; i < nameLen; ++i) {
            uint32_t val = 48 + (crypto_random(&seed) % (83 - 48));

            if (val > 57) {
                val += 39;
            }
            name[i] = (char)val;
        }

        names[j] = name;
    }

    if (gCmdOptions.verbose) {
        uint32_t startTime = (gCmdOptions.date / 3600) * 3600;
        printf("Domains count:\t\t\t%u\n", gCmdOptions.count);
        printf("Unique period:\t\t\t%u hour(s)\n", gCmdOptions.period);

        printf("Minimum length:\t\t\t%u\n", gCmdOptions.lowLimit);
        printf("Miximum length:\t\t\t%u\n", gCmdOptions.hiLimit);

        printf("Date:\t\t\t%u (", startTime); printf_date(startTime); printf(")\n");

        printf("Names:\n\n");
        currPeriod = (((gCmdOptions.date/3600) / gCmdOptions.period) * gCmdOptions.period) * 3600;
        for (j = 0; j < gCmdOptions.count; ++j) {
            printf_date(currPeriod); printf(" - "); currPeriod += (3600 * gCmdOptions.period) - 1; printf_date(currPeriod); ++currPeriod;
            printf(":\n %s\n\n", names[j]);
        }
        printf("\nURL for time conversion: http://www.onlineconversion.com/unix_time.htm\n\n");
    }
    else {
        for (j = 0; j < gCmdOptions.count; ++j) {
            printf("%s;", names[j]);
        }
        printf("\n");
    }

    return EXIT_SUCCESS;
}
