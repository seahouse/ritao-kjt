#include "inventorychannelq4sbatchget.h"

#include "global.h"
#include "configglobal.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>

InventoryChannelQ4SBatchGet::InventoryChannelQ4SBatchGet(QObject *parent) : QObject(parent),
    _optType(OTNone)
{
    _timer = new QTimer;
    connect(_timer, SIGNAL(timeout()), this, SLOT(sTimeout()));

    _manager = new QNetworkAccessManager;
    connect(_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(sReplyFinished(QNetworkReply*)));
}

void InventoryChannelQ4SBatchGet::download()
{
    _optType = OTDownloading;
    _hData._success = true;
    _hData._msgList.clear();

    _timer->start(1000);
}

void InventoryChannelQ4SBatchGet::setProductIdList(QList<QString> productIdList)
{
    _hData._total = productIdList.size();
    _hData._productList = productIdList;
    _hData._currentIndex = 0;
}

void InventoryChannelQ4SBatchGet::sTimeout()
{
    _timer->stop();

    switch (_optType) {
//    case OTDownloadIdList:
//        downloadProductIdList();
//        break;
    case OTDownloading:
        downloadNextItems();
        break;
    case OTDownloadEnd:
        emit finished(_hData._success, _hData._msgList.join("\r\n"));
        break;
    case OTDownloadError:
        emit finished(false, _msg);
    default:
        break;
    }
}

void InventoryChannelQ4SBatchGet::sReplyFinished(QNetworkReply *reply)
{
    QByteArray replyData = reply->readAll();

    QJsonObject json(QJsonDocument::fromJson(replyData).object());
    QString code = json.value("Code").toString("-99");
    QString desc = json.value("Desc").toString();

    QString opt;
    switch (_optType) {
//    case OTDownloadIdList:
//        if (code == "0")
//        {
//            /// 读取订单id列表
//            QJsonObject data = json.value("Data").toObject();
//            _hData._total = data.value("Total").toInt();
//            QJsonArray productListArray = data.value("ProductList").toArray();
//            QList<ProductSummary> productList;
//            for (int i = 0; i < productListArray.size(); i++)
//            {
//                QJsonObject productObject = productListArray.at(i).toObject();
//                productList.append(ProductSummary(
//                                       productObject.value("ProductID").toString(),
//                                       productObject.value("Status").toInt(),
//                                       QDateTime::fromString(productObject.value("ChangeDate").toString())
//                                       ));
//            }
//            _hData._productList = productList;
//            _hData._currentIndex = 0;
//            qDebug() << "total count:" << _hData._total;

//            _optType = OTDownloading;
//            _timer->start(1000);
//        }
//        else
//        {
//            _optType = OTProductDownloadError;
//            _msg = tr("下载区间订单ID列表错误：") + desc;
//            _timer->start(1000);
//        }
//        break;
    case OTDownloading:
        /// 解析数据，写入ERP数据库，并继续请求数据
        if ("0" == code)
        {
            QJsonArray dataArray = json.value("Data").toArray();
//            QJsonDocument jsonDoc(json);
//            QFile file("data.txt");
//            if (file.open(QIODevice::WriteOnly))
//            {
//                QTextStream out(&file);
//                out << jsonDoc.toJson(QJsonDocument::Compact);
//                file.close();
//            }
//            QJsonArray productListArray = data.value("ItemList").toArray();
            for (int i = 0; i < dataArray.size(); i++)
                insertItem2ERPByJson(dataArray.at(i).toObject());
        }
        _timer->start(1000);
        break;
    default:
        break;
    }

    qInfo() << opt << code << desc;
}

//void InventoryChannelQ4SBatchGet::downloadProductIdList()
//{
//    QMap<QString, QString> paramsMap(g_paramsMap);
//    paramsMap["method"] = "Product.ProductIDGetQuery";    // 由接口提供方指定的接口标识符
//    paramsMap["timestamp"] = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");       // 调用方时间戳，格式为“4 位年+2 位月+2 位日+2 位小时(24 小时制)+2 位分+2 位秒”
//    paramsMap["nonce"] = QString::number(100000 + qrand() % (999999 - 100000)); // QString::number(100000 + qrand() % (999999 - 100000));

//    /// 获取上次运行的结束日期，作为本次的起始日期
//    QDateTime dateStart;
//    QSqlQuery query;
//    query.prepare(tr("select 参数内容Date from 系统参数 where 参数分组='跨境通' and 参数名称='商品下载结束时间'"));
//    if (!query.exec())
//        qInfo() << query.lastError().text();
//    else if (query.first())
//        dateStart = query.value(tr("参数内容Date")).toDateTime();

//    if (!dateStart.isValid())
//        dateStart = QDateTime(QDate(1900, 1, 1));
//    qDebug() << dateStart;
//    _hData._dateStart = dateStart;
//    _hData._dateEnd = QDateTime::currentDateTime();

//    QJsonObject json;
//    json["SaleChannelSysNo"] = kjt_saleschannelsysno;
//    json["ChangedDateBegin"] = dateStart.toString("yyyy-MM-dd hh:mm:ss");
//    json["ChangedDateEnd"] = _hData._dateEnd.toString("yyyy-MM-dd hh:mm:ss");
//    QJsonDocument jsonDoc(json);
//    QFile file("11.txt");
//    if (file.open(QIODevice::WriteOnly))
//    {
//        QTextStream out(&file);
//        out << jsonDoc.toJson(QJsonDocument::Compact);
//        file.close();
//    }
//    qDebug() << jsonDoc.toJson(QJsonDocument::Compact);
//    paramsMap["data"] = jsonDoc.toJson(QJsonDocument::Compact);
//    qDebug() << paramsMap;

//    QString params;
//    QMapIterator<QString, QString> i(paramsMap);
//    while (i.hasNext())
//    {
//        i.next();
//        params.append(i.key()).append("=").append(i.value().toUtf8().toPercentEncoding()).append("&");
//    }

//    params.append(kjt_secretkey);
//    urlencodePercentConvert(params);
//    qDebug() << params;
//    QString sign = QCryptographicHash::hash(params.toLatin1(), QCryptographicHash::Md5).toHex();

//    QString url;
//    url.append(kjt_url).append("?").append(params).append("&sign=").append(sign);

//    qDebug() << url;
//    _manager->get(QNetworkRequest(QUrl(url)));
//}

void InventoryChannelQ4SBatchGet::downloadNextItems()
{
    if (_hData._currentIndex >= _hData._total)
    {
//        setProductGetFromKJTTime();   // 执行结束，修改系统参数的时间

        _optType = OTDownloadEnd;
        if (_hData._success)
            _hData._msgList.append(tr("下载品分销渠道库存结束."));
        _timer->start(1000);
        return;
    }

    QMap<QString, QString> paramsMap(g_paramsMap);
    paramsMap["method"] = "Inventory.ChannelQ4SBatchGet";    // 由接口提供方指定的接口标识符
    paramsMap["timestamp"] = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");       // 调用方时间戳，格式为“4 位年+2 位月+2 位日+2 位小时(24 小时制)+2 位分+2 位秒”
    paramsMap["nonce"] = QString::number(100000 + qrand() % (999999 - 100000)); // QString::number(100000 + qrand() % (999999 - 100000));

    QJsonObject json;
    QStringList productIdList;
    foreach (QString productId, _hData._productList.mid(_hData._currentIndex, 20)) {
        productIdList.append(productId);
    }
    _hData._currentIndex += 20;
    json["ProductIDs"] = productIdList.join(',');
    json["SaleChannelSysNo"] = g_config.kjtSaleschannelsysno();
    QJsonDocument jsonDoc(json);
//    QFile file("11.txt");
//    if (file.open(QIODevice::WriteOnly))
//    {
//        QTextStream out(&file);
//        out << jsonDoc.toJson(QJsonDocument::Compact);
//        file.close();
//    }
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

    params.append(g_config.kjtSecretkey());
    urlencodePercentConvert(params);
    qDebug() << params;
    QString sign = QCryptographicHash::hash(params.toLatin1(), QCryptographicHash::Md5).toHex();

    QString url;
    url.append(g_config.kjtUrl()).append("?").append(params).append("&sign=").append(sign);

    qDebug() << url;
    _manager->get(QNetworkRequest(QUrl(url)));
}

//void InventoryChannelQ4SBatchGet::setProductGetFromKJTTime()
//{
//    if (!_hData._success) return;
//    if (!_hData._dateStart.isValid() || !_hData._dateEnd.isValid()) return;

//    QSqlQuery query;
//    query.prepare(tr("update 系统参数 set 参数内容Date=:dateStart where 参数分组='跨境通' and 参数名称='商品下载起始时间'"));
//    query.bindValue(":dateStart", _hData._dateStart);
//    if (!query.exec())
//    {
//        qInfo() << query.lastError().text();
//    }

//    query.prepare(tr("update 系统参数 set 参数内容Date=:dateStart where 参数分组='跨境通' and 参数名称='商品下载结束时间'"));
//    query.bindValue(":dateStart", _hData._dateEnd);
//    if (!query.exec())
//    {
//        qInfo() << query.lastError().text();
//    }
//}

void InventoryChannelQ4SBatchGet::insertItem2ERPByJson(const QJsonObject &json)
{
    QString productId = json.value("ProductID").toString();             // 商品ID
    int onlineQty = json.value("OnlineQty").toInt();                    // 独占库存
    int wareHouseID = json.value("WareHouseID").toInt();                // 出库仓 仓库编号



    qDebug() << productId << onlineQty << wareHouseID;
//    if (productId == "032JPH027440001")
//        int iii = 0;

    /// 获取erp的商品id与贸易类型
    int productIdLocal = 0;     // 本地的商品id
    int productTradeType = 1;   // 贸易类型。 0 = 直邮，1 = 自贸（保税）
    QSqlQuery query;
    query.prepare(tr("select 商品KID, 贸易类型 from 商品 where p31=:productId"));
    query.bindValue(":productId", productId);
    if (!query.exec())
    {
        qInfo() << query.lastError().text();
        _hData._success = false;
        return;
    }
    if (query.first())
    {
        productIdLocal = query.value("商品KID").toInt();
        productTradeType = query.value("贸易类型").toInt();
    }

    /// 获取erp的仓库ID
    int wareHouseIDLocal = 0;     // 本地的仓库ID
    /// 直邮，仓库类型为3；保税，仓库类型为1
    int wareHouseType = (productTradeType == 0 ? 3 : 1);
//    QSqlQuery query;
    query.prepare(tr("select 仓库KID from 仓库 where 仓库编号=:wareHouseID and 仓库类别=:wareHouseType"));
    query.bindValue(":wareHouseID", QString::number(wareHouseID));
    query.bindValue(":wareHouseType", QString::number(wareHouseType));
    if (!query.exec())
    {
        qInfo() << query.lastError().text();
        _hData._success = false;
        return;
    }
    if (query.first())
        wareHouseIDLocal = query.value(0).toInt();

    if (productIdLocal > 0 && wareHouseIDLocal > 0)
    {
        QSqlQuery queryInventory;
        queryInventory.prepare("select * from 仓库库存 where 商品ID=:productIdLocal");
        queryInventory.bindValue(":productIdLocal", productIdLocal);
        if (!queryInventory.exec())
        {
            qInfo() << queryInventory.lastError().text();
            _hData._success = false;
            return;
        }

        /// 若存在，则更新；若不存在，则新增
        if (queryInventory.first())
        {
            /// 修改：不进行更新。 20160426
//            QSqlQuery queryUpdate;
//            queryUpdate.prepare("update 仓库库存 set "
//                                "库存数量=:onlineQty, 仓库ID=:wareHouseIDLocal "
//                                "where 商品ID=:productIdLocal");
//            queryUpdate.bindValue(":onlineQty", onlineQty);
//            queryUpdate.bindValue(":wareHouseIDLocal", wareHouseIDLocal);

//            queryUpdate.bindValue(":productIdLocal", productIdLocal);

//            if (!queryUpdate.exec())
//            {
//                qInfo() << queryUpdate.lastError().text();
//                _hData._success = false;
//            }
        }
        else
        {
            QSqlQuery queryInsert;
            queryInsert.prepare("insert into 仓库库存( "
                                "商品ID, 库存数量, 仓库ID "
                                ") values ("
                                ":productIdLocal, :onlineQty, :wareHouseIDLocal "
                                ")");
            queryInsert.bindValue(":productIdLocal", productIdLocal);
            queryInsert.bindValue(":onlineQty", onlineQty);
            queryInsert.bindValue(":wareHouseIDLocal", wareHouseIDLocal);

            if (!queryInsert.exec())
            {
                qInfo() << queryInsert.lastError().text();
                _hData._success = false;
            }
        }
    }
    else
    {
        qInfo() << "商品ID(" + productId + ")或仓库ID(" + QString::number(wareHouseID) + ")未找到，无法同步商品分销渠道库存";
        _hData._success = false;
        return;
    }
}

QDateTime InventoryChannelQ4SBatchGet::convertKjtTime(const QString &kjtTime)
{
    /// 跨镜通将会把日期修改为 yyyy-MM-dd hh:mm:ss 形式。到时再修改此处
    qint64 utcTime = kjtTime.mid(kjtTime.indexOf("(") + 1, 13).toLongLong();
    return QDateTime::fromMSecsSinceEpoch(utcTime);      // 订单时间
}
