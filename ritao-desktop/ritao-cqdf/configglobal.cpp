#include "configglobal.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QDebug>

ConfigGlobal g_config;

ConfigGlobal::ConfigGlobal()
{
    QFile file("config.json");
    if (!file.open(QIODevice::ReadOnly))
    {
        qInfo() << "不存在配置文件config.json。";
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    read(doc.object());
}

void ConfigGlobal::read(const QJsonObject &json)
{
    _dbHost = json.value("dbHost").toString();
    _dbName = json.value("dbName").toString();
    _dbUsername = json.value("dbUsername").toString();
    _dbPassword = json.value("dbPassword").toString();
    _kjtUrl = json.value("kjtUrl").toString();
    _kjtSecretkey = json.value("kjtSecretkey").toString();
    _kjtAppid = json.value("kjtAppid").toString();
    _kjtSaleschannelsysno = json.value("kjtSaleschannelsysno").toString();
    _kjtVersion = json.value("kjtVersion").toString();
    _kjtFormat = json.value("kjtFormat").toString();
    _intervalMinutes = json.value("intervalMinutes").toInt(30);
}
