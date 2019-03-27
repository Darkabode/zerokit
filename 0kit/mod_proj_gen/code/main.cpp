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
#include <iostream>


using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;
using Poco::Util::AbstractConfiguration;
using Poco::Util::OptionCallback;
using Poco::AutoPtr;

#define MOD_PROP_PATH "modPath"
#define MOD_PROP_NAME "modName"
#define MOD_PROP_DEPEND "modDepend"
#define MOD_TEMPLATE1 "mod_template"
#define MOD_TEMPLATE1_SIZE 12
#define MOD_TEMPLATE2 "pTemplateBlock"
#define MOD_TEMPLATE2_SIZE 14

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

		options.addOption(Option("mod-path", "p", "specify path where will be created new mod project")
			.required(true)
			.repeatable(false)
			.argument("path")
			.binding(MOD_PROP_PATH));
		
		options.addOption(
			Option("mod-name", "n", "define a name of mod project")
			.required(true)
			.repeatable(false)
			.argument("name")
			.binding(MOD_PROP_NAME));

		options.addOption(
			Option("mod-depend", "d", "define a name of the already existent mod project")
			.required(true)
			.repeatable(false)
			.argument("depend")
			.binding(MOD_PROP_DEPEND));
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
		helpFormatter.setHeader("Loader's MOD-project generator.");
		helpFormatter.format(std::cout);
	}

	int main(const std::vector<std::string>& args)
	{
		if (!_helpRequested) {
			_sModPath = config().getString(MOD_PROP_PATH);
			_sModName1 = config().getString(MOD_PROP_NAME);
			_sModName1.insert(0, "mod_");
			_sModDepend1 = config().getString(MOD_PROP_DEPEND);
			_sModDepend1.insert(0, "mod_");
			logger().information("MOD-project path: " + _sModPath);
			logger().information("MOD-project name: " + _sModName1);

			_sModName2 = config().getString(MOD_PROP_NAME);
			_sModName2[0] = toupper(_sModName2[0]);
			_sModName2.insert(0, "p");
			_sModName2.append("Block");

			_sModName3 = config().getString(MOD_PROP_NAME);
			for (int i = 0; i < _sModName3.size(); ++i)
				_sModName3[i] = toupper(_sModName3[i]);
			_sModName3.insert(0, "MOD_");

			_sModDepend2 = config().getString(MOD_PROP_DEPEND);
			_sModDepend2[0] = toupper(_sModDepend2[0]);
			_sModDepend2.insert(0, "p");
			_sModDepend2.append("Block");

			try {
				_templatePath = config().getString("application.dir") + "mod_template";
				logger().information("MOD-template path: " + _templatePath.path());
				logger().information("");

				Poco::File file(_sModPath);
				if (!file.isDirectory()) {
					logger().error(_sModPath + " is not valid path");
					return Application::EXIT_IOERR;
				}

				// Проверяем наличие зависимого проекта...
				changeFile(_sModDepend1, "");
				file = _destPath;

				if (!file.exists()) {
					logger().error(_sModPath + " is not valid path. Before create this one you must create " + _sModDepend1 + " project!");
					return Application::EXIT_IOERR;
				}
				
				std::string fileBuff;
				std::string ptrModBlock;

				// Добавляем блок в структуру global_block_t в файле mod_shared\global.h
				changeFile("mod_shared", "global.h");
				file = _destPath;
				readFile(file, fileBuff);

				std::size_t pos;
				std::string modDependDeclr;

				modDependDeclr += "p";
				modDependDeclr += _sModDepend1;
				modDependDeclr += "_block_t ";
				modDependDeclr += _sModDepend2;
				modDependDeclr += ";";

				pos = fileBuff.find(modDependDeclr);
				if (pos == std::string::npos)
					throw Poco::DataException("Can't find " + modDependDeclr + " declaration in mod_shared\\global.h");

				ptrModBlock += modDependDeclr;
				ptrModBlock += "\r\n\tp";
				ptrModBlock += _sModName1;
				ptrModBlock += "_block_t";
				ptrModBlock += " ";
				ptrModBlock += _sModName2;
				ptrModBlock += ";";				
				fileBuff.replace(pos, modDependDeclr.size(), ptrModBlock);

				writeFile(file, fileBuff);
				logger().information("Added declaration of " + _sModName1 + " block to global_block_t structure!");

				// Добавляем директиву #include в mod_shared\headers.h
				changeFile("mod_shared", "headers.h");
				file = _destPath;
				readFile(file, fileBuff);

				modDependDeclr.clear();
				modDependDeclr += "#include \"../../mod_shared/";
				modDependDeclr += _sModDepend1;
				modDependDeclr += "Api.h\"";				

				pos = fileBuff.find(modDependDeclr);
				if (pos == std::string::npos)
					throw Poco::DataException("Can't find " + modDependDeclr + " in mod_shared\\headers.h");

				ptrModBlock.clear();
				ptrModBlock += modDependDeclr;
				ptrModBlock += "\r\n#include \"../../mod_shared/";
				ptrModBlock += _sModName1;
				ptrModBlock += "Api.h\"";				
				fileBuff.replace(pos, modDependDeclr.size(), ptrModBlock);

				writeFile(file, fileBuff);
				logger().information("Added include directive " + _sModName1 + " in mod_shared\\headers.h!");


				changeFile(_sModName1, "");
				file = _destPath;

				if (file.exists()) {
					logger().error(file.path() + " already exists");
					return Application::EXIT_IOERR;
				}

				if (!file.createDirectory()) {
					logger().error("Can't create " + file.path());
					return Application::EXIT_IOERR;
				}

				logger().information("");
				logger().information("Created new directory: " + file.path());

				Poco::DirectoryIterator itr(_templatePath);
				std::string relPath;
				cloneDirectory(itr, relPath);

				// Создаём файл mod_templateApi.h в папке mod_shared.
				changeFile("mod_shared", _sModName1 + "Api.h");				
				file = _destPath;
				
				if (!file.createFile()) {
					logger().error("Can't create " + file.path());
					return Application::EXIT_IOERR;
				}

				ptrModBlock.clear();
				ptrModBlock += "p";
				ptrModBlock += _sModName1;
				ptrModBlock += "_block_t";

				fileBuff.clear();
				fileBuff += "#ifndef __";
				fileBuff += _sModName3;
				fileBuff += "API_H_\r\n";
				fileBuff += "#define __";
				fileBuff += _sModName3;
				fileBuff += "API_H_\r\n\r\n\r\n";
				fileBuff += "typedef struct _";
				fileBuff += _sModName1;
				fileBuff += "_block\r\n{\r\n\tint dummy; // must be deleted...\r\n} ";
				fileBuff += _sModName1;
				fileBuff += "_block_t, *";
				fileBuff += ptrModBlock;
				fileBuff += ";\r\n\r\n#endif // __";
				fileBuff += _sModName3;
				fileBuff += "API_H_\r\n";

				writeFile(file, fileBuff);				
				logger().information("Created new file: " + file.path());
			}
			catch (Poco::Exception& exc) {
				logger().error(exc.displayText());
				return Application::EXIT_SOFTWARE;
			}

			logger().information(_sModName1 + " project has created successfully!");
		}
		return Application::EXIT_OK;
	}

private:
	void cloneDirectory(Poco::DirectoryIterator& itr, std::string& relPath)
	{
		Poco::DirectoryIterator end;
		Poco::File projFile;
		Poco::Path projPath;
		std::string fName;
		std::size_t pos;

		for ( ; itr != end; ++itr) {
			fName = itr.name();
			pos = fName.find(MOD_TEMPLATE1);
			if (pos != std::string::npos) {
				fName.replace(pos, MOD_TEMPLATE1_SIZE, _sModName1);
			}

			projFile = *itr;
			projPath.assign(projFile.path());

			Poco::File newPath(_destPath.toString() + relPath + fName);

			if (projFile.isDirectory()) {
				if (!newPath.createDirectory())
					throw Poco::FileException("Can't create " + newPath.path());

				logger().information("Created new directory: " + newPath.path());
				Poco::DirectoryIterator newItr(itr.path().toString());
				cloneDirectory(newItr, relPath + fName + Poco::Path::separator());
			}
			else { // file...
				processTemplateFile(Poco::File(itr.path().toString()), newPath);
			}
		}
	}

	void processTemplateFile(Poco::File& templateFile, Poco::File& destFile)
	{
		std::string fileBuff;
		readFile(templateFile, fileBuff);
		
		if (!destFile.createFile())
			throw Poco::FileException("Can't create " + destFile.path());

		if (!fileBuff.empty()) {
			std::size_t pos;

			for (pos = fileBuff.find(MOD_TEMPLATE1, 0); pos != std::string::npos; pos = fileBuff.find(MOD_TEMPLATE1, pos + 1)) {
				fileBuff.replace(pos, MOD_TEMPLATE1_SIZE, _sModName1);
			}

			for (pos = fileBuff.find(MOD_TEMPLATE2, 0); pos != std::string::npos; pos = fileBuff.find(MOD_TEMPLATE2, pos + 1)) {
				fileBuff.replace(pos, MOD_TEMPLATE2_SIZE, _sModName2);
			}

			writeFile(destFile, fileBuff);
			logger().information("Created new file: " + destFile.path());
		}
	}

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
		_destPath.assign(_sModPath);
		_destPath.pushDirectory(pathName);
		if (!fileName.empty())
			_destPath.setFileName(fileName);
		_destPath.makeAbsolute();
	}

	bool _helpRequested;
	std::string _sModPath;
	std::string _sModName1;
	std::string _sModName2;
	std::string _sModName3;
	std::string _sModDepend1;
	std::string _sModDepend2;
	Poco::File _templatePath;
	Poco::Path _destPath;
};

POCO_APP_MAIN(ModpgApp)
