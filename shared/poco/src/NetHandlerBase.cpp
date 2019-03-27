#include "Poco/Util/Application.h"
#include "Poco/DateTimeFormatter.h"

#include "NetHandlerBase.h"


SharedRequestHandlerBase::SharedRequestHandlerBase(const std::string& panelInfo, const std::string& panelSection, const std::string& currentURL, bool autoUpdate) :
_autoUpdate(autoUpdate),
_panelInfo(panelInfo),
_panelSection(panelSection),
_currentURL(currentURL)
{
}

void SharedRequestHandlerBase::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{
  Poco::Util::Application& app = Poco::Util::Application::instance();
  //         app.logger().information("Request from " + request.clientAddress().toString());

  try {
      response.setChunkedTransferEncoding(true);
      response.setContentType("text/html");

      if (!handleForm(request, response)) {
          std::ostream& ostr = response.send();

          beginHtml(ostr, request, response);
          handleBody(ostr, request, response);
          endHtml(ostr, request, response);
      }
  }
  catch (Poco::Exception& exc) {
      app.logger().error(exc.displayText());
      response.redirect(request.getURI());
  }
}

void SharedRequestHandlerBase::beginHtml(std::ostream& ostr, Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{
    Poco::Util::LayeredConfiguration& cfg = Poco::Util::Application::instance().config();
    ostr << "<html><head>";
    if (_autoUpdate) {
        ostr << "<meta http-equiv=\"refresh\" content=\"7\">";
    }
    ostr << "<title>" << _panelInfo << "</title>"
        "<style type=\"text/css\">"
        "table { border-collapse:collapse; }"
        "table tr td {"
        "padding: px;"
        "border-collapse: collapse; }"
        "A { color: #ffffff; }"
        "A:visited { color: #ffffff; }"
        "A:active { color: #ffffff; }"
        "</style>"
        "</head><body>"
        "<table border=\"0\" cellpadding=\"1\" cellspacing=\"1\" style=\"width: 100%; background-color: #2C4056;\"><tr>"
        "<td style=\"width: 50%; text-align: left\"><span style=\"color:#ffffff;\">" << cfg.getString("system.osName") << " " << cfg.getString("system.osVersion") << "; " << cfg.getString("system.osArchitecture") << "; " << cfg.getString("system.nodeName") << "</span></td>"
        "<td style=\"width: 3px; text-align: center\"><strong><span style=\"color:#ffffff;\"><a href=\"" << _currentURL << "\">" << _panelSection << "</a></span></strong></td>"
        "<td style=\"width: 50%; text-align: right\"><span style=\"color:#ffffff;\">Server time: " << Poco::DateTimeFormatter::format(Poco::Timestamp(), "%d.%m.%Y %H:%M:%S") << "</span></td>"
        "</tr></table>"
        "<table border=\"0\" cellpadding=\"1\" cellspacing=\"1\" style=\"width: 100%; background-color: #507AAA;\">";

    processHeader(ostr, request, response);

    ostr << "</table><br>";

}

void SharedRequestHandlerBase::endHtml(std::ostream& ostr, Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{
    processFooter(ostr, request, response);

    ostr << "</body></html>\n";
}

void SharedRequestHandlerBase::printError(std::ostream& ostr, const std::string& errText)
{
    ostr << "<p style=\"text-align: center;\"><strong><span style=\"color:#ff0000;\">" << errText << "</span></strong></p>";
}

void SharedRequestHandlerBase::handleError(const std::string& errText, Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{
    std::ostream& ostr = response.send();

    beginHtml(ostr, request, response);

    printError(ostr, errText);

    handleBody(ostr, request, response);

    endHtml(ostr, request, response);
}
