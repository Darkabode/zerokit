#ifndef __GEOIP_H_
#define __GEOIP_H_

int geoip_init();
void geoip_done();
const char* geoip_get_country(const char* ip);

#endif // __GEOIP_H_
