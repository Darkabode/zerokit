#include <math.h>

#include "types.h"
#include "crc64.h"

#define cnCRC_64_H  0x42F0E1EB   
#define cnCRC_64_L  0xA9EA3693   
//CRC-64-ECMA-182 x64 + x62 + x57 + x55 + x54 + x53 + x52 + x47 + x46 + x45 +    
//x40 + x39 + x38 + x37 + x35 + x33 + x32 + x31 + x29 + x27 + x24 + x23 + x22    
//+ x21 + x19 + x17 + x13 + x12 + x10 + x9 + x7 + x4 + x + 1    
//(as described in ECMA-182 p.63)  or 0xC96C5795D7870F42 (0x92D8AF2BAF0E1E85)    

uint32_t TableCRCHigh[256]; // CRC ±í for 64   
uint32_t TableCRCLow[256]; // CRC ±í for 64   


void crc64_buildtable()    
{    
	uint32_t i, j;
	uint32_t nData[2];
	uint32_t nAccum[2];

	nData[0] = 0;
	nData[1] = 0;

	for (i = 0; i < 256; ++i) {
		nAccum[0] = 0;
		nAccum[1] = 0;
		nData[1] = i;

		nData[0] = nData[1]<<24;
		nData[1]  = 0;
		for (j = 0; j < 8; ++j) {
			if ((nData[0] ^ nAccum[0]) & 0x80000000) {
				nAccum[0] = ((nAccum[0] << 1) | ((nAccum[1] & 0x80000000) >> 31)) ^ cnCRC_64_H;
				nAccum[1] = (nAccum[1] << 1) ^ cnCRC_64_L;
			}
			else {
				nAccum[0] = (nAccum[0] << 1 ) | ((nAccum[1]&0x80000000)>>31);
				nAccum[1] = nAccum[1] << 1;
			}
			nData[0] = (nData[0] << 1) | ((nData[1] & 0x80000000) >> 31);
			nData[1] = nData[1] << 1;
		}

		TableCRCHigh[i] = nAccum[0];
		TableCRCLow[i]  = nAccum[1];
	}
}    

void crc64_computate(uint8_t* data, uint32_t size, uint32_t* pCrc)
{    
	uint32_t i;
	uint32_t temp;
	uint32_t index;

	pCrc[0] = 0;
	pCrc[1] = 0;

	for (i = 0; i < size; ++i) {
		temp = pCrc[0];
		index = (temp >> 24) ^ *data++;

		pCrc[0] = ((pCrc[0] << 8) | ((pCrc[1] & 0xff000000) >> 24)) ^ TableCRCHigh[index];
		pCrc[1] = (pCrc[1] << 8) ^ TableCRCLow[index];
	}
}
