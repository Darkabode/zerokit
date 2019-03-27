#ifndef __CONFIGURATION_H_
#define __CONFIGURATION_H_

#if defined(_MSC_VER)
#pragma pack(push, 1)
#endif
typedef struct _configuration
{
	char block_header[8];
	uint16_t clientId;
	uint8_t block_hash[8];
} configuration_t, *pconfiguration_t;
#if defined(_MSC_VER)
#pragma pack(pop)
#endif

#endif // __CONFIGURATION_H_
