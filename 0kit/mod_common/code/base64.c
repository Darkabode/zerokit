UINT8* base64_encode(UINT8* outData, const UINT8* inData, UINT inSize)
{
    UINT i;
    UINT j;
    UINT8* pHead = outData;
    const UINT8* inEnd = inData + inSize;
    UINT8 char_array_3[3];
    UINT8 char_array_4[4];
    USE_GLOBAL_BLOCK

    __stosb(char_array_3, 0, 3);
    __stosb(char_array_4, 0, 4);
    i = 0;
    while (inData < inEnd) {
        pGlobalBlock->pCommonBlock->fnmemset(&char_array_3[i++], *(inData++), sizeof(UINT8));
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; i < 4 ; i++) {
                *outData++ = pGlobalBlock->pCommonBlock->base64[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        __stosb(&char_array_3[i], '\0', 3 - i);

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++) {
            *outData++ = pGlobalBlock->pCommonBlock->base64[char_array_4[j]];
        }

        __stosb(outData, '=', 3 - i);
    }
    return pHead;
}

int base64_decode(UINT8* inData, UINT inSize, UINT8* outData)
{
    UINT i;
    UINT j;
    UINT t;
    UINT q;
    UINT8 char_array_4[4];
    UINT8 char_array_3[3];
    USE_GLOBAL_BLOCK

    __stosb(char_array_3, 0, 3);
    __stosb(char_array_4, 0, 4);
    i = t = q = 0;
    while (t < inSize && (inData[t] != '=')) {
        char_array_4[i++] = inData[t++];
        if (i == 4) {
            for (i = 0; i < 4; i++) {
                for (j = 0; j < 65; j++) {
                    if (pGlobalBlock->pCommonBlock->base64[j] == char_array_4[i]) {
                        break;
                    }
                }
                char_array_4[i] = (UINT8)j;
            }

            char_array_3[0] = (char_array_4[0] << 2) | (char_array_4[1] >> 4);
            char_array_3[1] = (char_array_4[1] << 4) | (char_array_4[2] >> 2);
            char_array_3[2] = ((char_array_4[2] << 6) & 0xc0) | char_array_4[3];

            for (i = 0; i < 3; i++) {
                outData[q++] = char_array_3[i];
            }
            i = 0;
        }
    }

    if (i) {
        __stosb(&char_array_4[i], 0, 4 - i);

        for (j = 0; j < 4; j++) {
            for (t = 0; t < 65; t++)
                if (pGlobalBlock->pCommonBlock->base64[t] == char_array_4[j]) {
                    break;
                }
            char_array_4[j] = (UINT8)t;
        }

        char_array_3[0] = (char_array_4[0] << 2) | (char_array_4[1] >> 4);
        char_array_3[1] = (char_array_4[1] << 4) | (char_array_4[2] >> 2);
        char_array_3[2] = ((char_array_4[2] << 6) & 0xc0) | char_array_4[3];
        __movsb(&outData[q], char_array_3, i - 1);
        q += i - 1;
    }

    return q;
}