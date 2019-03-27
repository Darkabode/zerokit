#include "types.h"

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

uint8_t* base64_encode(uint8_t* inData, int inSize, uint8_t* outData)
{
	int i = 0;
	int j = 0;
	int t = 0;
	uint8_t char_array_3[3] = {0};
	uint8_t char_array_4[4] = {0};
 
	while (t < inSize) {
		char_array_3[i++] = inData[t++];
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for(i = 0; i < 4 ; i++)
				*outData++ = cb64[char_array_4[i]];
			i = 0;
		}
	}

	if (i) {
		for(j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			*outData++ = cb64[char_array_4[j]];

		while((i++ < 3))
			*outData++ = '=';
	}
	return outData;
}

int base64_decode(const uint8_t* inData, int inSize, uint8_t* outData)
{
	int i = 0;
	int j = 0;
	int t = 0;
	int q = 0;
	uint8_t char_array_4[4] = {0};
	uint8_t char_array_3[3] = {0};

    while (t < inSize && (inData[t] != '=')) {
        char_array_4[i++] = inData[t++];
        if (i == 4) {
            for (i = 0; i < 4; i++) {
                for (j = 0; j < sizeof(cb64); j++)
                    if (cb64[j] == char_array_4[i])
                        break;
                char_array_4[i] = (uint8_t)j;
            }

            char_array_3[0] = (char_array_4[0] << 2) | (char_array_4[1] >> 4);
            char_array_3[1] = (char_array_4[1] << 4) | (char_array_4[2] >> 2);
            char_array_3[2] = ((char_array_4[2] << 6) & 0xc0) | char_array_4[3];

            for (i = 0; i < 3; i++)
                outData[q++] = char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < 4; j++) {
            int t;
            for (t = 0; t < sizeof(cb64); t++)
                if (cb64[t] == char_array_4[j])
                    break;
            char_array_4[j] = (uint8_t)t;
        }

        char_array_3[0] = (char_array_4[0] << 2) | (char_array_4[1] >> 4);
        char_array_3[1] = (char_array_4[1] << 4) | (char_array_4[2] >> 2);
        char_array_3[2] = ((char_array_4[2] << 6) & 0xc0) | char_array_4[3];

        for (j = 0; j < i - 1; j++)
            outData[q++] = char_array_3[j];
    }

	return q;
}
