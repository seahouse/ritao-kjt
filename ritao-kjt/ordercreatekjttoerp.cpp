#include "ordercreatekjttoerp.h"

#include "global.h"
#include "rnetworkaccessmanager.h"

#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QCryptographicHash>
#include <QUrl>
#include <QNetworkRequest>

OrderCreateKJTToERP::OrderCreateKJTToERP(QObject *parent) :
    QObject(parent)
{
}

void OrderCreateKJTToERP::run(const QDateTime &dateTimeStart, const QDateTime &dateTimeEnd)
{
    QMap<QString, QString> paramsMap(g_paramsMap);
    paramsMap["method"] = "Order.OrderIDQuery";    // 由接口提供方指定的接口标识符
    paramsMap["timestamp"] = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");       // 调用方时间戳，格式为“4 位年+2 位月+2 位日+2 位小时(24 小时制)+2 位分+2 位秒”
    paramsMap["nonce"] = QString::number(100000 + qrand() % (999999 - 100000)); // QString::number(100000 + qrand() % (999999 - 100000));

    QJsonObject json;
    json["OrderDateBegin"] = dateTimeStart.toString("yyyy-MM-dd hh:mm:ss");
    json["OrderDateEnd"] = dateTimeEnd.toString("yyyy-MM-dd hh:mm:ss");
    QJsonDocument jsonDoc(json);
    qDebug() << jsonDoc.toJson(QJsonDocument::Compact);

    paramsMap["data"] = jsonDoc.toJson(QJsonDocument::Compact);
    qDebug() << paramsMap;

    QString params;
    QMapIterator<QString, QString> i(paramsMap);
    while (i.hasNext())
    {
        i.next();
        params.append(i.key()).append("=").append(i.value().toUtf8().toPercentEncoding()).append("&");
    }

    params.append(kjt_secretkey);
    urlencodePercentConvert(params);
    qDebug() << params;
    QString sign = QCryptographicHash::hash(params.toLatin1(), QCryptographicHash::Md5).toHex();

    QString url;
    url.append(kjt_url).append("?").append(params).append("&sign=").append(sign);

    qDebug() << url;
//    g_networkManager.get(QNetworkRequest(QUrl(url)));
}
