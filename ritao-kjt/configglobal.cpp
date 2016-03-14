#include "configglobal.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

ConfigGlobal g_config;

ConfigGlobal::ConfigGlobal() :
    _tempMaxHandlerCount(-1),
    _havePrinter(0),
    _autoReply(0),
    _autoTellSale(0)
{
    QFile file("config.json");
    if (!file.open(QIODevice::ReadOnly))
        return;

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    read(doc.object());
}

void ConfigGlobal::read(const QJsonObject &json)
{
    _tempExcelForPrint = json["TempExcelForPrint"].toString();
    if (json.contains("TempMaxHandlerCount"))
        _tempMaxHandlerCount = json["TempMaxHandlerCount"].toInt();
    if (json.contains("zipExeFile"))
        _zipExeFile = json["zipExeFile"].toString();
    if (json.contains("HavePrinter"))
        _havePrinter = json["HavePrinter"].toInt();
    if (json.contains("AutoReply"))
        _autoReply = json["AutoReply"].toInt();
    if (json.contains("AutoTellSale"))
        _autoTellSale = json["AutoTellSale"].toInt();
    if (json.contains("FtpFolder"))
    {
        QJsonArray ja = json["FtpFolder"].toArray();
        for (int i = 0; i < ja.size(); i++)
            _ftpFolderList.append(ja[i].toString());
    }
    if (json.contains("FtpDestFolder"))
        _ftpDestFolder = json["FtpDestFolder"].toString();
    if (json.contains("MailDestFolder"))
        _mailDestFolder = json["MailDestFolder"].toString();
    if (json.contains("DocsCopyDestFolder"))
        _docsCopyDestFolder = json["DocsCopyDestFolder"].toString();
    if (json.contains("MailSkipList"))
    {
        QJsonArray ja = json["MailSkipList"].toArray();
        for (int i = 0; i < ja.size(); i++)
            _mailSkipList.append(ja[i].toString());
    }
}
