#ifndef __IP_RANGES_H_
#define __IP_RANGES_H_

typedef struct _ip_ranges
{
	char* name;

	uint32_t* ranges;
	uint8_t* ids;
	time_t mtime;
	uint32_t tried;
	uint32_t count;

	int nOfNames;
	char* buffer;
	char** countryCodes;
	char** names;
} ip_ranges_t;

#if defined(_MSC_VER)
#pragma pack(push, 1)
#endif

struct _geo_header
{
    uint32_t totalSize;
    uint32_t rangesSize;
    uint32_t countryMaxId;
};

struct _ip_block
{
    uint32_t addr;
    uint8_t countryId;
}
#ifndef _WIN32
__attribute__((packed))
#endif // !_WIN32
;
#if defined(_MSC_VER)
#pragma pack(pop)
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct _ip_block* pip_block_t;
typedef struct _geo_header* pgeo_header_t;


ip_ranges_t* newIPRanges(const char* name);
int refreshRanges(ip_ranges_t* pRanges);
uint8_t ip2id(ip_ranges_t* ranges, uint32_t ip);
char* id2code(ip_ranges_t* ranges, uint8_t id);
char* id2name(ip_ranges_t* ranges, short id);
uint8_t code2id(ip_ranges_t* ranges, const char* countryCode);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __IP_RANGES_H_
