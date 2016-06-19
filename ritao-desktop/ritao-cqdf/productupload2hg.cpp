#include "productupload2hg.h"

#include "global.h"
#include "configglobal.h"

#include <QSqlQuery>
#include <QVariant>
#include <QDebug>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QSqlError>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QFile>
#include <QNetworkRequest>
#include <QNetworkReply>


ProductUpload2HG::ProductUpload2HG(QObject *parent) : QObject(parent)
{
    _timer = new QTimer;
    connect(_timer, SIGNAL(timeout()), this, SLOT(sTimeout()));

    _manager = new QNetworkAccessManager;
    connect(_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(sReplyFinished(QNetworkReply*)));
}

void ProductUpload2HG::upload()
{
    _productIdQueue.clear();
    _msgList.clear();

    /// 获取需要新增到跨境通的商品KID列表
    /// test erp product id: 6608
    QSqlQuery query(tr("select 同步主键KID from 数据同步 "
                       "where p3='1' and p4='0' and 同步指令='新增' and 同步表名='商品' "
                       "order by 数据同步KID "));
    while (query.next())
        _productIdQueue.enqueue(query.value(tr("同步主键KID")).toInt());

    qDebug() << _productIdQueue;

    _optType = OTProductUploading;
    _timer->start(1000);
}

void ProductUpload2HG::sTimeout()
{
    _timer->stop();

    switch (_optType) {
    case OTProductUploading:
        uploadNextProduct();
        break;
    case OTProductUploadEnd:
        emit finished(true, tr("上传商品结束。"));
        break;
    case OTProductUploadError:
        emit finished(false, _msg);
        break;
    default:
        break;
    }
}

void ProductUpload2HG::uploadNextProduct()
{
    if (_productIdQueue.isEmpty())
    {
        _optType = OTProductUploadEnd;
        _timer->start(1000);
        return;
    }

    _ohData._currentProductId = _productIdQueue.dequeue();

    QSqlQuery query;
    query.prepare(tr("select * from 商品 "
                     "where 商品KID=:id "));
    query.bindValue(":id", _ohData._currentProductId);
    if (!query.exec())
    {
//        _optType = OTOrderUploadError;
//        _msg = tr("订单");
//        qFatal(query.lastError().text().toStdString().c_str());
        qInfo() << query.lastError().text();
        _timer->start(1000);
        return;
    }

    if (query.first())
    {
        _doc.clear();
        QMap<QString, QString> paramsMap(g_paramsMap);
        paramsMap["service"] = "subItemAddOrUpdate";             // 由接口提供方指定的接口标识符

        QDomElement root = _doc.createElement("DTC_Message");
        _doc.appendChild(root);

        QDomElement tagMessageHead = _doc.createElement("MessageHead");
        root.appendChild(tagMessageHead);

        appendElement(tagMessageHead, "MessageType", "SKU_INFO");
        QString messageId = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
        appendElement(tagMessageHead, "MessageId", messageId);
        appendElement(tagMessageHead, "ActionType", "1");
        appendElement(tagMessageHead, "MessageTime", QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss"));
        appendElement(tagMessageHead, "SenderId", g_config.hgNumber());
        appendElement(tagMessageHead, "ReceiverId", hg_receiver);
        appendElement(tagMessageHead, "UserNo", g_config.hgNumber());
        QString password = QCryptographicHash::hash(QString(messageId + g_config.hgPassword()).toLatin1(), QCryptographicHash::Md5).toHex();
        qDebug() << "password: " << password;
        appendElement(tagMessageHead, "Password", password);

        QJsonObject json;

        QJsonArray itemList;
        QJsonObject item;
        item["goodsId"] = query.value(tr("商品编码")).toString();               // 商品货号
        item["title"] = query.value(tr("商品名称")).toString();                 // 商品名称
        item["num"] = "0";                                                      // 数量
        item["desc"] = query.value(tr("商品简述")).toString();                  // 商品描述
        item["price"] = QString::number(query.value(tr("销售价")).toDouble()); // 商品价格
        item["postFee"] = "0";                                                  // 平邮费用
        item["expessFee"] = "0";                                                // 快递费用
        item["emsFee"] = "0";                                                   // ems费用
        item["outerId"] = "";                                                   // 外部编码
        item["listTime"] = query.value(tr("创建日期")).toDate().toString();     // 上架时间
        item["type"] = "fixed";                     // 发布类型  fixed(一口价),auction(拍卖)
        item["approveStatus"] = "onsale";           // 商品上传后的状态  onsale(出售中),instock(仓库中)

        QJsonArray skuList;
        QJsonObject sku;
        sku["skuId"] = query.value(tr("商品编码")).toString();              // sku的id
        sku["skuHgId"] = "20115207022601";
        int productTradeType = query.value(tr("贸易类型")).toInt();             // 贸易类型  0 = 直邮 1 = 自贸
        sku["isbs"] = productTradeType == 1 ? "true" : "false";             // 是否保税商品true或者false
        sku["hgzc"] = "test";
        sku["hgxh"] = "test";
        sku["ownerCode"] = "rks";
        sku["ownerName"] = "zgm";
        sku["skuSpecId"] = query.value(tr("商品规格")).toString();              // 产品规格信息
        sku["outerId"] = "";                        // 新增可以空,商家设置的外部id
        sku["barcode"] = query.value(tr("商品条形码")).toString();               // 商品级别的条形码(SKUID)
        sku["numlid"] = "0";
        sku["quantity"] = "0";
        sku["price"] = QString::number(query.value(tr("销售价")).toDouble());  // 商品SKU的价格
        sku["fjm"] = "test";
        sku["status"] = "normal";
        sku["type"] = "全量更新";

        skuList.append(sku);
        item["skuList"] = skuList;
        itemList.append(item);
        json["itemList"] = itemList;

        QJsonDocument jsonDoc(json);
        paramsMap["content"] = jsonDoc.toJson(QJsonDocument::Compact);

        QString data = _doc.toString(-1);
        qDebug() << data;
        QFile file("01.xml");
        if (file.open(QIODevice::WriteOnly))
        {
            QTextStream out(&file);
            out << data;
            file.close();
        }
        QString data2 = _doc.toString();
        QFile file2("02.xml");
        if (file2.open(QIODevice::WriteOnly))
        {
            QTextStream out(&file2);
            out << data2;
            file2.close();
        }

//        QString params;
//        QMapIterator<QString, QString> i(paramsMap);
//        while (i.hasNext())
//        {
//            i.next();
//            params.append(i.key()).append("=").append(i.value().toUtf8().toPercentEncoding()).append("&");
//        }

//        urlencodePercentConvert(params);
//        QString secret = QCryptographicHash::hash(params.toLatin1(), QCryptographicHash::Md5).toHex();
//        params.append("secret=");       // 暂时留空
//        qDebug() << params;

        qDebug() << paramsMap;

        QString params;
        params.append("data=").append(data.toUtf8());
        urlencodePercentConvert(params);
        qDebug() << params.toUtf8();
        qDebug() << data.toUtf8().toBase64().toPercentEncoding();

        QNetworkRequest req;
        req.setUrl(QUrl(g_config.hgUrl()));
//        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        _manager->post(req, QString("data=").append(data.toUtf8().toBase64().toPercentEncoding()).toLatin1());
//        _manager->post(req, params.toUtf8().toBase64());
    }
    else
    {
        _msgList.append("商品(id:" + QString::number(_ohData._currentProductId) + ")不存在。");
        _timer->start(1000);
        return;
    }
}

void ProductUpload2HG::sReplyFinished(QNetworkReply *reply)
{
    QByteArray replyData = reply->readAll();
    qDebug() << replyData;

    QJsonObject json(QJsonDocument::fromJson(replyData).object());
    bool code = json.value("isSuccess").toBool(false);
    QString body = json.value("body").toString();

    QString opt;
    switch (_optType) {
    case OTProductUploading:
        opt = tr("新增商品");
        if (code == true)
        {
            QSqlQuery query;
            /// 将重庆地服的outerId 存入商品表（p43）
            QJsonArray array = QJsonDocument::fromJson(body.toLatin1()).array();
            if (array.size() > 0)
            {
                QJsonObject data = array.at(0).toObject();
                QString outerId = data.value("outerId").toString();
                query.prepare(tr("update 商品 set p43=:outerId "
                                 "where 商品KID=:id "));
                query.bindValue(":outerId", outerId);
                query.bindValue(":id", _ohData._currentProductId);
                if (!query.exec())
                    qInfo() << tr("更新重庆地服的outerid失败: ") << query.lastError().text();
            }

            /// 记录同步数据，并进行下一个重庆地服同步
            query.prepare(tr("update 数据同步 set p2='1' "
                             "where p1='1' and 同步指令='新增' and 同步表名='商品' and 同步主键KID=:id "));
            query.bindValue(":id", _ohData._currentProductId);
            if (!query.exec())
                qInfo() << tr("更新数据同步的跨境通商品新增处理: ") << query.lastError().text();

            _timer->start(1000);
        }
        else
        {
            _optType = OTProductUploadError;
            _msg = tr("上传商品到海关错误：") + body;
            _timer->start(1000);
        }
        break;
    default:
        break;
    }

    qInfo() << opt << code << body;
}

void ProductUpload2HG::appendElement(QDomElement &tagParent, const QString &name, const QString &value)
{
    QDomElement tag = _doc.createElement(name);
    tagParent.appendChild(tag);
    QDomText t = _doc.createTextNode(value);
    tag.appendChild(t);
}
