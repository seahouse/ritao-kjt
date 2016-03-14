#ifndef CONFIGGLOBAL_H
#define CONFIGGLOBAL_H

#include <QStringList>

class QJsonObject;

class ConfigGlobal
{
public:
    ConfigGlobal();

public:
    QString tempExcelForPrint() const { return _tempExcelForPrint; }
    int tempMaxHandlerCount() const { return _tempMaxHandlerCount; }
    QString zipExeFile() const { return _zipExeFile; }
    bool havePrinter() const { return _havePrinter; }
    bool autoReply() const { return _autoReply; }
    bool autoTellSale() const { return _autoTellSale; }
    QStringList ftpFolderList() const { return _ftpFolderList; }
    QString ftpDestFolder() const { return _ftpDestFolder; }
    QString mailDestFolder() const { return _mailDestFolder; }
    QString docsCopyDestFolder() const { return _docsCopyDestFolder; }
    QStringList mailSkipList() const { return _mailSkipList; }

private:
    void read(const QJsonObject &json);

private:
    QString _tempExcelForPrint;
    int _tempMaxHandlerCount;
    QString _zipExeFile;
    int _havePrinter;
    int _autoReply;
    int _autoTellSale;
    QStringList _ftpFolderList;
    QString _ftpDestFolder;
    QString _mailDestFolder;
    QString _docsCopyDestFolder;
    QStringList _mailSkipList;
};

extern ConfigGlobal g_config;

#endif // CONFIGGLOBAL_H
