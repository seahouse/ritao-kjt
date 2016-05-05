#ifndef CONFIGGLOBAL_H
#define CONFIGGLOBAL_H

#include <QStringList>

class QJsonObject;

class ConfigGlobal
{
public:
    ConfigGlobal();

public:
    QString kjtUrl() const { return _kjtUrl; }
    QString kjtSecretkey() const { return _kjtSecretkey; }
    QString kjtAppid() const { return _kjtAppid; }
    QString kjtSaleschannelsysno() const { return _kjtSaleschannelsysno; }
    QString kjtVersion() const { return _kjtVersion; }
    QString kjtFormat() const { return _kjtFormat; }
    int intervalMinutes() const { return _intervalMinutes; }

private:
    void read(const QJsonObject &json);

private:
    QString _kjtUrl;
    QString _kjtSecretkey;
    QString _kjtAppid;
    QString _kjtSaleschannelsysno;
    QString _kjtVersion;
    QString _kjtFormat;
    int _intervalMinutes;
};

extern ConfigGlobal g_config;

#endif // CONFIGGLOBAL_H
