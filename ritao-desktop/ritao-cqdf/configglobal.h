#ifndef CONFIGGLOBAL_H
#define CONFIGGLOBAL_H

#include <QStringList>

class QJsonObject;

class ConfigGlobal
{
public:
    ConfigGlobal();

public:
    QString dbHost() const { return _dbHost; }
    QString dbName() const { return _dbName; }
    QString dbUsername() const { return _dbUsername; }
    QString dbPassword() const { return _dbPassword; }
    QString cqdfUrl() const { return _cqdfUrl; }
    QString cqdfAppkey() const { return _cqdfAppkey; }
    QString cqdfFormat() const { return _cqdfFormat; }
    QString cqdfEncrypt() const { return _cqdfEncrypt; }
    QString hgCode() const { return _hgCode; }
    QString hgNumber() const { return _hgNumber; }
    QString hgName() const { return _hgName; }
    QString hgPassword() const { return _hgPassword; }
    QString hgUrl() const { return _hgUrl; }
    int intervalMinutes() const { return _intervalMinutes; }

private:
    void read(const QJsonObject &json);

private:
    QString _dbHost;
    QString _dbName;
    QString _dbUsername;
    QString _dbPassword;
    QString _cqdfUrl;
    QString _cqdfAppkey;
    QString _cqdfFormat;
    QString _cqdfEncrypt;
    QString _hgCode;
    QString _hgNumber;
    QString _hgName;
    QString _hgPassword;
    QString _hgUrl;
    int _intervalMinutes;
};

extern ConfigGlobal g_config;

#endif // CONFIGGLOBAL_H
