#include "globals.h"

#ifdef _WIN32
#include <intrin.h>

HANDLE gHeap = NULL;
#endif // _WIN32


#include "Poco/Buffer.h"
#include "Poco/Util/Application.h"
#include "Poco/FileStream.h"
#include "Poco/Data/MySQL/Connector.h"

#include "rsawrap.cpp"

Globals::Globals(Poco::Util::LayeredConfiguration& config) :
_sid((Poco::UInt32)config.getInt("server.id")),
_poolMinSize(config.getInt("mempool.min_size", 100)),
_poolMaxSize(config.getInt("mempool.max_size", 300)),
gPostBodyPool(config.getInt("mempool.http_post_body_size", 4096), _poolMinSize, _poolMaxSize),
gShaHashPool(20, _poolMinSize, _poolMaxSize),
gQueryPool(1024, _poolMinSize, _poolMaxSize),
gBotInfoPool(198, _poolMinSize, _poolMaxSize),
_mysqlPool(Poco::Data::MySQL::Connector::KEY, config.getString("db.conn")),
_bundlesPathPrefix(config.getString("bundles.pathPrefix", "files/")),
_redisHost(config.getString("redis.host", "localhost")),
_redisPort(config.getInt("redis.port", 6379))
{
    Poco::File rsaKeyFile("skey.private");
    if (!rsaKeyFile.exists()) {
        Poco::FileNotFoundException(rsaKeyFile.path());
    }
    Poco::Buffer<Poco::UInt8> rsaKey((std::size_t)rsaKeyFile.getSize());
    readFile(rsaKeyFile, (char*)rsaKey.begin());

    rsa_init(&gRsaContext, RSA_PKCS_V15, 0);

    std::size_t sz;
    Poco::UInt8* ptr = rsaKey.begin();

    sz = *(Poco::UInt16*)ptr; ptr += 2;
    if (mpi_read_binary(&gRsaContext.N, ptr, sz)) {
        Poco::DataException("Can't read RSA N value");
    }
    ptr += sz;

    sz = *(Poco::UInt16*)ptr; ptr += 2;
    if (mpi_read_binary(&gRsaContext.E, ptr, sz)) {
        Poco::DataException("Can't read RSA E value");
    }
    ptr += sz;

    sz = *(Poco::UInt16*)ptr; ptr += 2;
    if (mpi_read_binary(&gRsaContext.D, ptr, sz)) {
        Poco::DataException("Can't read RSA D value");
    }
    ptr += sz;

    sz = *(Poco::UInt16*)ptr; ptr += 2;
    if (mpi_read_binary(&gRsaContext.P, ptr, sz)) {
        Poco::DataException("Can't read RSA P value");
    }
    ptr += sz;

    sz = *(Poco::UInt16*)ptr; ptr += 2;
    if (mpi_read_binary(&gRsaContext.Q, ptr, sz)) {
        Poco::DataException("Can't read RSA Q value");
    }
    ptr += sz;

    sz = *(Poco::UInt16*)ptr; ptr += 2;
    if (mpi_read_binary(&gRsaContext.DP, ptr, sz)) {
        Poco::DataException("Can't read RSA DP value");
    }
    ptr += sz;

    sz = *(Poco::UInt16*)ptr; ptr += 2;
    if (mpi_read_binary(&gRsaContext.DQ, ptr, sz)) {
        Poco::DataException("Can't read RSA DQ value");
    }
    ptr += sz;

    sz = *(Poco::UInt16*)ptr; ptr += 2;
    if (mpi_read_binary(&gRsaContext.QP, ptr, sz)) {
        Poco::DataException("Can't read RSA QP value");
    }

    gRsaContext.len = (mpi_msb(&gRsaContext.N) + 7) >> 3;

    crc64_buildtable();

    pGip = GeoIP_open("GeoIP.dat", GEOIP_MEMORY_CACHE);

    if (pGip == NULL) {
        Poco::RuntimeException("Can't initialize GeoIP");
    }

    Poco::Data::MySQL::Connector::registerConnector();

    int err;
    _pRedisSysInfo = new RedisWrapper(_redisHost, _redisPort, Poco::Util::Application::instance().logger());
    if ((err = _pRedisSysInfo->connect()) != ERR_OK) {
        throw Poco::RuntimeException("_pRedisSysInfo: Cannot connect to redis with error " + Poco::NumberFormatter::format(err));
    }

    _pRedisTasksRequest = new RedisWrapper(_redisHost, _redisPort, Poco::Util::Application::instance().logger());
    if ((err = _pRedisTasksRequest->connect()) != ERR_OK) {
        throw Poco::RuntimeException("_pRedisTasksRequest: Cannot connect to redis with error " + Poco::NumberFormatter::format(err));
    }

    _pRedisTasksReport = new RedisWrapper(_redisHost, _redisPort, Poco::Util::Application::instance().logger());
    if ((err = _pRedisTasksReport->connect()) != ERR_OK) {
        throw Poco::RuntimeException("_pRedisTasksReport: Cannot connect to redis with error " + Poco::NumberFormatter::format(err));
    }
}

Globals::~Globals()
{
    GeoIP_delete(pGip);
    Poco::Data::MySQL::Connector::unregisterConnector();
}

const char* Globals::geoip_get_country(const char* ip)
{
    return GeoIP_country_code_by_addr(pGip, ip);
}

void Globals::readFile(Poco::File& file, char* data)
{
    Poco::FileInputStream iStream(file.path(), std::ios::in | std::ios::binary);
    if (!iStream.good()) {
        throw Poco::FileException("Can't read " + file.path());
    }

    if (file.getSize() > 0) {
        iStream.read(data, (std::streamsize)file.getSize());
        iStream.close();
    }
}
