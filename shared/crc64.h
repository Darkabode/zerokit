#ifndef __CRC64_H_
#define __CRC64_H_

void crc64_buildtable();
   
void crc64_computate(uint8_t* data, uint32_t size, uint32_t* pCrc);
  

#endif // __CRC64_H_
