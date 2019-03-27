#include <QProcess>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>

#include "zhwindow.h"

#include "../../shared_code/platform.h"
#include "../../shared_code/types.h"
#include "../../../loader/mod_shared/pack_protect.h"

#pragma pack(push, 1)
typedef struct _request_file_header
{
	uint32_t subsCount;
	uint32_t clientId;
    char domens[1024];
} request_file_header_t, *prequest_file_header_t;
#pragma pack(pop)

ZHelperWindow::ZHelperWindow(QWidget *parent, Qt::WFlags flags) :
QMainWindow(parent, flags),
_settings("zhelper.ini", QSettings::IniFormat)
{
	ui.setupUi(this);
}

ZHelperWindow::~ZHelperWindow()
{

}

/*
int create_request_file(const char* outPath, uint16_t id, uint32_t user_id, const char* subs)
{
int ret = 0;
time_t now;
struct tm* ts;
char ch;
uint16_t subsCount, subid, tmpVal;
uint8_t buff[16];
uint8_t md5Hash[16];
const char *itr;
char fileName[37];
char* fullPath;
char *fileData = NULL;
uint16_t* ptr;
int outPathLen = strlen(outPath);

now = time(NULL);
ts = localtime(&now);
sprintf((char*)buff, "%02d%02d%04d%02d%02d%02d", ts->tm_mday, ts->tm_mon, ts->tm_year, ts->tm_hour, ts->tm_min, ts->tm_sec);

md5(buff, 14, md5Hash);
utils_md5_to_str(md5Hash, fileName);
strcat(fileName, ".bin");

// Временный Формат файла: <id>,<subsCount>
fileData = malloc(8 + 2 * strlen(subs));
ptr = (uint16_t*)fileData;
*(ptr++) = id;
*(uint32_t*)ptr = user_id;
ptr++; ptr++; ptr++;

itr = subs;
subsCount = 0;

for ( ; *itr != '\0'; ) {
	subid = tmpVal = 0;

	do {
		ch = *itr++;
		if (ch == ':')
			break;
		else if (ch == '\0')
			break;
		else if (ch < '0')
			break;
		ch = ch - '0';
		if (ch > 9)
			break;
		tmpVal = 10 * subid;
		subid = (uint16_t)ch + tmpVal;
	} while (1);

	if (ch == ':' || ch == '\0') {
		*(ptr++) = subid;
		++subsCount;
	}	
}

ptr = (uint16_t*)fileData;
ptr++; ptr++;
*(++ptr) = subsCount;

fullPath = calloc(1, outPathLen + 40);
strcpy(fullPath, outPath);
if (fullPath[outPathLen - 1] != '/')
fullPath[outPathLen] = '/';
strcat(fullPath, fileName);

ret = utils_save_file(fullPath, fileData, sizeof(uint32_t) + (2 + subsCount) * sizeof(uint16_t));
free(fileData);
if (ret) {
	if (gGlobalConfig.reqStdoutFullPath == 1) {
		fprintf(stdout, "err:Can't create file %s\n", fullPath);
	}
	else {
		fprintf(stdout, "err:Can't create file %s\n", fileName);
	}
}
else {
	if (gGlobalConfig.reqStdoutFullPath == 1) {
		fprintf(stdout, "ok:%s\n", fullPath);
	}
	else {
		fprintf(stdout, "ok:%s\n", fileName);
	}	
}

free(fullPath);
return ret;
}

*/

void ZHelperWindow::on_createButton_clicked()
{
	int i;
	QList<uint32_t> clientIds;
	QList<uint32_t> affIds;
	QList<uint32_t> subIds;
	QString sTokens = ui.reqTokens->toPlainText().trimmed();

	if (sTokens.isEmpty()) {
		QMessageBox msgBox(QMessageBox::Information, tr("Info"), tr("You need paste one or more request token(s)."));
		msgBox.exec();
		return;
	}

    if (ui.domens->toPlainText().size() > 1023) {
        QMessageBox msgBox(QMessageBox::Information, tr("Info"), tr("Maximum allowed size of domens list is 1023 chars."));
        msgBox.exec();
        return;
    }

	QByteArray tokens = ui.reqTokens->toPlainText().trimmed().toAscii();
	int firstBracket, secondBracket;

	for (i = 0; i < tokens.count(); ++i) {
		for ( ; tokens[i] != '[' && i < tokens.count(); ++i);
		firstBracket = i;
		for ( ; tokens[i] != ']' && i < tokens.count(); ++i);
		secondBracket = i;

		if (firstBracket == secondBracket) {
			QMessageBox msgBox(QMessageBox::Warning, tr("Error"), tr("Invalid request token(s)"));
			msgBox.exec();
			return;
		}

		QByteArray reqData = tokens.mid(firstBracket + 1, secondBracket - firstBracket - 1);

		if (reqData.count() > 0) {
			QList<QByteArray> reqList = reqData.split(',');

			if (reqList.size() != 3) {
				QMessageBox msgBox(QMessageBox::Warning, tr("Error"), tr("Invalid request token(s)"));
				msgBox.exec();
				return;
			}

			uint32_t clientId = reqList.at(0).trimmed().toUInt();
			uint32_t affId = reqList.at(1).trimmed().toUInt();
			uint32_t subId = reqList.at(2).trimmed().toUInt();

			clientIds.push_back(clientId);
			affIds.push_back(affId);
			subIds.push_back(subId);
		}
	}

	if (clientIds.count() == 0) {
		QMessageBox msgBox(QMessageBox::Warning, tr("Error"), tr("There are no valid tokens for request file"));
		msgBox.exec();
		return;
	}

    uint32_t clientId = _settings.value("client_id").toUInt();
 	for (i = 1; i < clientIds.count(); ++i) {
 		if (clientIds.at(i) != clientId) {
            QMessageBox::critical( NULL, tr("Error"), tr("Several requests has different client id"), QMessageBox::Ok);
			return;
 		}
 	}

	uint32_t sizeReqData = sizeof(request_file_header_t) + 2 * sizeof(uint32_t) * clientIds.count();
	char* reqData = new char[sizeReqData];
    memset(reqData, 0, sizeReqData);
	prequest_file_header_t pHeader = (prequest_file_header_t)reqData;
	pHeader->clientId = clientId;
	pHeader->subsCount = clientIds.count();

    QByteArray arr = ui.domens->toPlainText().trimmed().toAscii();
    strcpy_s(pHeader->domens, 1024, arr.constData());

	do {
		uint32_t* pValues = (uint32_t*)(reqData + sizeof(request_file_header_t));
		for (i = 0; i < clientIds.count(); ++i) {
			*pValues++ = affIds.at(i);
			*pValues++ = subIds.at(i);
		}

		QString fName = QFileDialog::getSaveFileName(this, tr("Save request file"), "", tr("Request file (*.zrq)"));
		{
			QFile file(fName);

			if (!file.open(QIODevice::WriteOnly)) {
				QMessageBox msgBox(QMessageBox::Critical, tr("Error"), tr("Could not create request file"));
				msgBox.exec();
				break;
			}

			QDataStream out(&file);
			if (out.writeRawData(reqData, sizeReqData) != sizeReqData) {
				QMessageBox::critical( NULL, tr("Error"), tr("Failed to write data in file"), QMessageBox::Ok);
				break;
			}

			file.close();
		}

		QProcess process;
		process.start(QApplication::applicationDirPath() + "/zsigner.exe " + "-i=\"" + fName + "\" " + "-priv=\"" + QApplication::applicationDirPath() + "/" + _settings.value("key_file", "key.private").toString() + "\" -e");
		process.waitForFinished();
		if (process.exitCode() != 0) {
			QMessageBox::critical(NULL, tr("Error"), QString("Failed to sign file %1").arg(fName), QMessageBox::Ok);
			break;
		}

        // В заголовок записываем идентификатор клиента.
        int fSize;
        char* buffer;
        psign_pack_header_t pSignPackHdr;
        {
            QFile file(fName);

            if (!file.open(QIODevice::ReadOnly)) {
                QMessageBox::critical(NULL, tr("Error"), tr("Could not open request file for postprocessing"));
                break;
            }

            fSize = (int)file.size();
            buffer = new char[fSize];
            QDataStream out(&file);

            if (out.readRawData(buffer, fSize) != fSize) {
                QMessageBox::critical( NULL, tr("Error"), QString("Failed to write toOne of the token has incorrect client id (Your: %1, In token: %2)").arg(clientId).arg(clientIds.at(i)), QMessageBox::Ok);
                break;
            }

            file.close();
        }
        pSignPackHdr = (psign_pack_header_t)buffer;
        pSignPackHdr->reserved1 = clientId;
        {
            QFile file(fName);

            if (!file.open(QIODevice::WriteOnly)) {
                QMessageBox msgBox(QMessageBox::Critical, tr("Error"), tr("Could not create request file"));
                msgBox.exec();
                break;
            }

            QDataStream out(&file);
            if (out.writeRawData(buffer, fSize) != fSize) {
                QMessageBox::critical( NULL, tr("Error"), tr("Failed to write data in file"), QMessageBox::Ok);
                break;
            }

            file.close();
        }
        delete[] buffer;

		QMessageBox::information(NULL, tr("OK"), tr("Request file successully created!"), QMessageBox::Ok);
	} while (0);

	delete[] reqData;
}
