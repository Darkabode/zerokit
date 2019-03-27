#ifndef __SHARED_NETREQUESTHANDLERFACTORY_H_
#define __SHARED_NETREQUESTHANDLERFACTORY_H_

#include "Poco/StringTokenizer.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"


class SharedRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory
{
public:
    SharedRequestHandlerFactory(const std::string& serverAdmin);

protected:
    std::string createURI(const Poco::StringTokenizer& parts);

    virtual Poco::Net::HTTPRequestHandler* handleForRequest(const Poco::StringTokenizer& parts, const Poco::Net::HTTPServerRequest& request) = 0;

    std::string _serverAdmin;

private:
    Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request);
};

#endif // __SHARED_NETREQUESTHANDLERFACTORY_H_
