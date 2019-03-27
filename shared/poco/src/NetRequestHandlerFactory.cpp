#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Util/Application.h"

#include "NetRequestHandlerFactory.h"

SharedRequestHandlerFactory::SharedRequestHandlerFactory(const std::string& serverAdmin) :
_serverAdmin(serverAdmin)
{
}

std::string SharedRequestHandlerFactory::createURI(const Poco::StringTokenizer& parts)
{
    std::string uri;

    Poco::StringTokenizer::Iterator itr = parts.begin();
    Poco::StringTokenizer::Iterator end = parts.end();

    for ( ; itr < end; ++itr) {
        uri += "/";
        uri += *itr;
    }
    uri += "/";
    return uri;
}

Poco::Net::HTTPRequestHandler* SharedRequestHandlerFactory::createRequestHandler(const Poco::Net::HTTPServerRequest& request)
{
    Poco::StringTokenizer parts(request.getURI(), "/", Poco::StringTokenizer::TOK_IGNORE_EMPTY | Poco::StringTokenizer::TOK_TRIM);

    try {
        return handleForRequest(parts, request);
    }
    catch (Poco::Exception& exc) {
        Poco::Util::Application::instance().logger().error(exc.displayText());
    }

    return 0;
}
