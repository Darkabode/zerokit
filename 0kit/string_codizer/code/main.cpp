#include "Poco/File.h"
#include "Poco/Path.h"
#include "Poco/DirectoryIterator.h"
#include "Poco/FileStream.h"
#include "Poco/Util/Application.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Util/AbstractConfiguration.h"
#include "Poco/AutoPtr.h"
#include "Poco/Format.h"
#include <iostream>


using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;
using Poco::Util::AbstractConfiguration;
using Poco::Util::OptionCallback;
using Poco::AutoPtr;

#define FILE_PATH "filePath"
#define VALUE_NAME "valueName"
//#define MOD_PROP_DEPEND "modDepend"
//#define MOD_TEMPLATE1 "mod_template"
//#define MOD_TEMPLATE1_SIZE 12
//#define MOD_TEMPLATE2 "pTemplateBlock"
//#define MOD_TEMPLATE2_SIZE 14

class ModpgApp: public Application
{
public:
	ModpgApp() :
	_helpRequested(false)
	{
	}

protected:	
	void initialize(Application& self)
	{
		loadConfiguration(); // load default configuration files, if present
		Application::initialize(self);
		// add your own initialization code here
	}

	void uninitialize()
	{
		// add your own uninitialization code here
		Application::uninitialize();
	}

	void reinitialize(Application& self)
	{
		Application::reinitialize(self);
		// add your own reinitialization code here
	}

	void defineOptions(OptionSet& options)
	{
		Application::defineOptions(options);

		options.addOption(
			Option("help", "h", "display this help")
			.required(false)
			.repeatable(false)
			.callback(OptionCallback<ModpgApp>(this, &ModpgApp::handleHelp)));

		options.addOption(Option("path", "p", "path to file with string")
			.required(true)
			.repeatable(false)
			.argument("path")
			.binding(FILE_PATH));

		//options.addOption(
		//	Option("mod-depend", "d", "define a name of the already existent mod project")
		//	.required(true)
		//	.repeatable(false)
		//	.argument("depend")
		//	.binding(MOD_PROP_DEPEND));
	}

	void handleHelp(const std::string& name, const std::string& value)
	{
		_helpRequested = true;
		displayHelp();
		stopOptionsProcessing();
	}

	void displayHelp()
	{
		HelpFormatter helpFormatter(options());
		helpFormatter.setCommand(commandName());
		helpFormatter.setUsage("OPTIONS");
		helpFormatter.setHeader("String Codizer.");
		helpFormatter.format(std::cout);
	}

	int main(const std::vector<std::string>& args)
	{
		if (!_helpRequested) {
			try {
				int i  = 0;
				
				std::string fileBuff;
				std::string singlePart;
				std::string outBuff;

				readFile(Poco::File(config().getString(FILE_PATH)), fileBuff);

				outBuff = "ptr = (UINT8*)someVariable;\n";

				std::string::const_iterator itr = fileBuff.begin();
				std::string::const_iterator end = fileBuff.end();

				while ((end - itr) >= 4) {
					singlePart = Poco::format("*(((PUINT32)ptr)++) = 0x%02x%02x%02x%02x;\n", (unsigned)*(itr++), (unsigned)*(itr++), (unsigned)*(itr++), (unsigned)*(itr++));
					outBuff += singlePart;
				}

				if (itr <= end) {
					int remain = end - itr + 1;

					if (remain == 1) {
						outBuff += "*ptr = 0x";
						outBuff += Poco::format("%02x", (unsigned)0);
					}
					else if (remain == 2) {
						outBuff += "*(PUINT16)ptr = 0x";
						outBuff += Poco::format("%02x", (unsigned)0);
						outBuff += Poco::format("%02x", (unsigned)*itr);
					}
					else {
						outBuff += "*(PUINT16)ptr = 0x";
						outBuff += Poco::format("%02x", (unsigned)0);

						for (std::string::const_iterator itr1 = fileBuff.end() - 1; itr1 >= itr; --itr1) {
							singlePart = Poco::format("%02x", (unsigned)*itr1);
							outBuff += singlePart;
						}
					}

					outBuff += ";";
				}

				writeFile(Poco::File(config().getString(FILE_PATH) + ".c"), outBuff);
			}
			catch (Poco::Exception& exc) {
				logger().error(exc.displayText());
				return Application::EXIT_SOFTWARE;
			}

			logger().information(_sModName1 + "OK!");
		}
		return Application::EXIT_OK;
	}

private:
	void readFile(Poco::File& file, std::string& buffer)
	{
		Poco::FileInputStream iStream(file.path(), std::ios::in | std::ios::binary);
		if (!iStream.good())
			throw Poco::FileException("Can't read " + file.path());

		if (file.getSize() > 0) {
			buffer.resize((std::size_t)file.getSize());
			iStream.read(&buffer.at(0), (std::streamsize)file.getSize());
			iStream.close();
		}
	}

	void writeFile(Poco::File& file, const std::string& buffer)
	{
		Poco::FileOutputStream oStream(file.path(), std::ios::out | std::ios::binary);
		if (!oStream.good())
			throw Poco::FileException("Can't write to " + file.path());

		oStream.write(buffer.data(), buffer.size());
		oStream.close();
	}

	void changeFile(const std::string& pathName, const std::string& fileName)
	{
		_destPath.assign(_sFilePath);
		_destPath.pushDirectory(pathName);
		if (!fileName.empty())
			_destPath.setFileName(fileName);
		_destPath.makeAbsolute();
	}

	bool _helpRequested;
	std::string _sFilePath;
	std::string _sModName1;
	std::string _sModName2;
	std::string _sModName3;
	std::string _sModDepend1;
	std::string _sModDepend2;
	Poco::File _templatePath;
	Poco::Path _destPath;
};

POCO_APP_MAIN(ModpgApp)
