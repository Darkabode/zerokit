#ifndef __BASE64_H_
#define __BASE64_H_

uint8_t* base64_encode(uint8_t* inData, int inSize, uint8_t* outData);
int base64_decode(const uint8_t* inData, int inSize, uint8_t* outData);

#endif // __BASE64_H_
