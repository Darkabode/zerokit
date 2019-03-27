#include <GeoIPCity.h>

#include "../../shared_code/types.h"

#include "globalconfig.h"
#include "geoip.h"

extern global_config_t gGlobalConfig;

static GeoIP* pGip;

int geoip_init()
{
	if (gGlobalConfig.geoipDataFile == 0 || gGlobalConfig.geoipDataFile[0] == 0)
		return 1;

	pGip = GeoIP_open(gGlobalConfig.geoipDataFile, GEOIP_MEMORY_CACHE);

	if (pGip == NULL)
		return 1;

	return 0;
}

void geoip_done()
{
	GeoIP_delete(pGip);
}

const char* geoip_get_country(const char* ip)
{
	return GeoIP_country_code_by_addr(pGip, ip);
}

/*
int  geoip_get_info(const char* ip, char** pCity, char** pCountryCode)
{
	GeoIPRecord* pRec;
	pRec = GeoIP_record_by_addr(pGip, ip);
	if (pRec == NULL)
		return 1;

	*pCity = pRec->city;
	*pCountryCode = pRec->country_code;
	return 0;
}
*/
