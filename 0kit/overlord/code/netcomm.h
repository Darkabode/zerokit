#ifndef __NETCOMM_H_
#define __NETCOMM_H_

extern int wsaInited;

int netcomm_check();
int netcomm_make_transaction(const char* server, uint16_t port, const char* httpRequest, uint32_t httpRequestLen, uint8_t** pData, uint32_t* pDataSize);
int netcomm_get_ntp_time(uint32_t* pNtpTime);
int netcomm_gethostbyname(const char* server, uint32_t* pAddr);

#endif // __NETCOMM_H_
