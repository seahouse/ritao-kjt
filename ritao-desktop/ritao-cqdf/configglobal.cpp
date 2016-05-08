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
    _cqdfUrl = json.value("cqdfUrl").toString();
    _cqdfAppkey = json.value("cqdAppkey").toString();
    _cqdfFormat = json.value("cqdFormat").toString();
    _cqdfEncrypt = json.value("cqdEncrypt").toString();
    _intervalMinutes = json.value("intervalMinutes").toInt(30);
}
