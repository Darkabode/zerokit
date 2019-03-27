#ifndef __SHARED_NETHANDLERBASE_H_
#define __SHARED_NETHANDLERBASE_H_

#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"

class SharedRequestHandlerBase : public Poco::Net::HTTPRequestHandler
{
public:
    SharedRequestHandlerBase(const std::string& panelInfo, const std::string& panelSection, const std::string& currentURL, bool autoUpdate = false);
    void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);

    virtual bool handleForm(Poco::Net::HTTPServerRequest& /*request*/, Poco::Net::HTTPServerResponse& /*response*/) { return false; }
    virtual void handleBody(std::ostream& ostr, Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) = 0;
    virtual void processHeader(std::ostream& ostr, Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) {}
    virtual void processFooter(std::ostream& /*ostr*/, Poco::Net::HTTPServerRequest& /*request*/, Poco::Net::HTTPServerResponse& /*response*/) {}

protected:
    void beginHtml(std::ostream& ostr, Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
    void endHtml(std::ostream& ostr, Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
    void printError(std::ostream& ostr, const std::string& errText);
    void handleError(const std::string& errText, Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);

    bool _autoUpdate;
    std::string _panelSection;
    std::string _panelInfo;
    std::string _currentURL;
};

#endif // __SHARED_NETHANDLERBASE_H_
