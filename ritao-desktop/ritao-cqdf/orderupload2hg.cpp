#include "orderupload2hg.h"

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
#include <QDomDocument>

OrderUpload2HG::OrderUpload2HG(QObject *parent) : QObject(parent),
    _optType(OTNone),
    _success(true)
{
    _timer = new QTimer;
    connect(_timer, SIGNAL(timeout()), this, SLOT(sTimeout()));

    _manager = new QNetworkAccessManager;
    connect(_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(sReplyFinished(QNetworkReply*)));
}

void OrderUpload2HG::upload()
{
    _orderIdQueue.clear();
    _msgList.clear();

    /// 获取需要新增到跨境通的商品KID列表
    QSqlQuery query(tr("select 同步主键KID from 数据同步 "
                       "where p3='1' and p4='0' and 同步指令='新增' and 同步表名='订单' "
                       "order by 数据同步KID "));
    while (query.next())
        _orderIdQueue.enqueue(query.value(tr("同步主键KID")).toInt());

    qDebug() << _orderIdQueue;

    _optType = OTOrderUploading;
    _timer->start(1000);
}

void OrderUpload2HG::sTimeout()
{
    _timer->stop();

    switch (_optType) {
    case OTOrderUploading:
        uploadNextOrder();
        break;
    case OTOrderUploadEnd:
        emit finished(_success, _msgList.join(", ") + ".");
        break;
    case OTOrderUploadError:
        emit finished(_success, _msg);
        break;
    default:
        break;
    }
}

void OrderUpload2HG::uploadNextOrder()
{
    if (_orderIdQueue.isEmpty())
    {
        _optType = OTOrderUploadEnd;
        _msgList.append("上传订单结束");
        _timer->start(1000);
        return;
    }

    _ohData._currentOrderId = _orderIdQueue.dequeue();

    QSqlQuery query;
    query.prepare(tr("select * from 订单 "
                     "where 订单KID=:id "));
    query.bindValue(":id", _ohData._currentOrderId);
    if (!query.exec())
    {
        _success = false;
        qInfo() << query.lastError().text();
        _timer->start(1000);
        return;
    }

    if (query.first())
    {
        _doc.clear();

        QMap<QString, QString> paramsMap(g_paramsMap);
        paramsMap["service"] = "subAddSaleOrder";             // 服务名称

//        QDomDocument _doc;
        QDomElement root = _doc.createElement("DTC_Message");
        _doc.appendChild(root);

        QDomElement tagMessageHead = _doc.createElement("MessageHead");
        root.appendChild(tagMessageHead);

        QDomElement tag = _doc.createElement("MessageType");
        tagMessageHead.appendChild(tag);
        QDomText t = _doc.createTextNode("ORDER_INFO");
        tag.appendChild(t);

        tag = _doc.createElement("MessageId");
        tagMessageHead.appendChild(tag);
        QString messageId = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
        t = _doc.createTextNode(messageId);
        tag.appendChild(t);

        appendElement(tagMessageHead, "MessageTime", QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss"));
//        tag = _doc.createElement("MessageTime");
//        tagMessageHead.appendChild(tag);
//        t = _doc.createTextNode(QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss"));
//        tag.appendChild(t);

        appendElement(tagMessageHead, "SenderId", g_config.hgNumber());        //
        appendElement(tagMessageHead, "ReceiverId", hg_receiver);
        appendElement(tagMessageHead, "UserNo", g_config.hgNumber());
        QString password = QCryptographicHash::hash(QString(messageId + g_config.hgPassword()).toLatin1(), QCryptographicHash::Md5).toHex();
        qDebug() << "password: " << password;
        appendElement(tagMessageHead, "Password", password);

        QDomElement tagMessageBody = _doc.createElement("MessageBody");
        root.appendChild(tagMessageBody);

        QDomElement tagDTCFlow = _doc.createElement("DTCFlow");
        tagMessageBody.appendChild(tagDTCFlow);

        QDomElement tagORDER_HEAD = _doc.createElement("ORDER_HEAD");
        tagDTCFlow.appendChild(tagORDER_HEAD);

        appendElement(tagORDER_HEAD, "CUSTOMS_CODE", g_config.hgCode());                // 申报海关代码
        int tradeType = query.value("贸易类型").toInt();
        QString bizTypeCode = (tradeType == 0) ? "I10" : "I20";
        appendElement(tagORDER_HEAD, "BIZ_TYPE_CODE", bizTypeCode);               // 业务类型. 直邮：I10, 保税：I20
        appendElement(tagORDER_HEAD, "ORIGINAL_ORDER_NO", query.value("订单号").toString());       // 原始订单编号
        appendElement(tagORDER_HEAD, "ESHOP_ENT_CODE", g_config.hgNumber());
        appendElement(tagORDER_HEAD, "ESHOP_ENT_NAME", g_config.hgName());
        appendElement(tagORDER_HEAD, "DESP_ARRI_COUNTRY_CODE", "116");              // 起运国。 默认116（日本）
        appendElement(tagORDER_HEAD, "SHIP_TOOL_CODE", "Y");                        // 运输方式
        appendElement(tagORDER_HEAD, "RECEIVER_ID_NO", query.value("纳税人识别号").toString());       // 收货人身份证号码
        appendElement(tagORDER_HEAD, "RECEIVER_NAME", query.value("收货人").toString());             // 收货人姓名
        QString province;
        QSqlQuery provinceQuery;
        provinceQuery.prepare("select 省份 from 省份 where 省份KID=:provinceId");
        provinceQuery.bindValue(":provinceId", query.value("收货省份ID").toInt());
        if (provinceQuery.exec() && provinceQuery.first())
            province = provinceQuery.value("省份").toString();
        QString city;
        QSqlQuery cityQuery;
        cityQuery.prepare("select 城市名称 from 城市 where 城市KID=:cityId");
        cityQuery.bindValue(":cityId", query.value("收货城市ID").toInt());
        if (cityQuery.exec() && cityQuery.first())
            city = cityQuery.value("城市名称").toString();
        QString district;
        QSqlQuery districtQuery;
        districtQuery.prepare("select 区域 from 区域 where 区域KID=:districtId");
        districtQuery.bindValue(":districtId", query.value("收货区域ID").toInt());
        if (districtQuery.exec() && districtQuery.first())
            district = districtQuery.value("区域").toString();
        QString address = province + city + district + query.value("收货地址").toString();
        appendElement(tagORDER_HEAD, "RECEIVER_ADDRESS", address);                  // 收货人地址
        appendElement(tagORDER_HEAD, "RECEIVER_TEL", query.value("手机号码").toString());       // 收货人电话
        appendElement(tagORDER_HEAD, "GOODS_FEE", query.value("订单应付总额").toString());       // 货款总额
        appendElement(tagORDER_HEAD, "TAX_FEE", query.value("税金").toString());               // 税金总额
        QString sortLineId = (tradeType == 0) ? "SORTLINE04" : "SORTLINE03";
        appendElement(tagORDER_HEAD, "SORTLINE_ID", sortLineId);                // 分拣线ID. 保税：SORTLINE03, 直邮：SORTLINE04
        appendElement(tagORDER_HEAD, "TRANSPORT_FEE", query.value("配送费用").toString());      // 运费
        appendElement(tagORDER_HEAD, "CHECK_TYPE", "P");                // 验证类型. R:收货人, P:支付人
        appendElement(tagORDER_HEAD, "BUYER_REG_NO", "");               // 订购人注册号
        appendElement(tagORDER_HEAD, "BUYER_NAME", query.value(tr("个人姓名")).toString());     // 订购人姓名
        appendElement(tagORDER_HEAD, "BUYER_ID_TYPE", "1");     // 订购人证件类型  1=身份证，2=其他
        appendElement(tagORDER_HEAD, "BUYER_ID", query.value(tr("纳税人识别号")).toString());     // 订购人证件号码
        appendElement(tagORDER_HEAD, "DISCOUNT", "0.00");     // 优惠减免金额
        double actualPaid = query.value(tr("商品总金额")).toDouble() + query.value(tr("配送费用")).toDouble();
        appendElement(tagORDER_HEAD, "ACTUAL_PAID", QString::number(actualPaid));     // 实际支付金额
        appendElement(tagORDER_HEAD, "INSURED_FEE", "0.00");            // 保费

        QSqlQuery queryDetail;
        queryDetail.prepare(tr("select * from 订单商品 where 订单ID=:orderId"));
        queryDetail.bindValue(":orderId", _ohData._currentOrderId);
        if (queryDetail.exec())
        {
            while (queryDetail.next())
            {
                QDomElement tagORDER_DETAIL = _doc.createElement("ORDER_DETAIL");
                tagORDER_HEAD.appendChild(tagORDER_DETAIL);

                appendElement(tagORDER_DETAIL, "SKU", queryDetail.value("商品编号").toString());      // 商品货号
                appendElement(tagORDER_DETAIL, "GOODS_SPEC", queryDetail.value("商品规格属性").toString());       // 规格型号
                appendElement(tagORDER_DETAIL, "CURRENCY_CODE", "142");      // 币制代码. 默认：142（人民币）
                appendElement(tagORDER_DETAIL, "PRICE", queryDetail.value("销售单价").toString());    // 商品单价
                appendElement(tagORDER_DETAIL, "QTY", queryDetail.value("购买数量").toString());      // 商品数量
                appendElement(tagORDER_DETAIL, "GOODS_FEE", queryDetail.value("付款金额").toString());// 商品总价
                appendElement(tagORDER_DETAIL, "TAX_FEE", queryDetail.value("税金").toString());     // 税款金额
                appendElement(tagORDER_DETAIL, "COUNTRY", "");     // 原产国
            }
        }

        QString data = _doc.toString(-1);
        qDebug() << data;
        QFile file("11.xml");
        if (file.open(QIODevice::WriteOnly))
        {
            QTextStream out(&file);
            out << data;
            file.close();
        }
        QString data2 = _doc.toString();
        QFile file2("12.xml");
        if (file2.open(QIODevice::WriteOnly))
        {
            QTextStream out(&file2);
            out << data2;
            file2.close();
        }





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
        _success = false;
        qInfo() << "上传的订单（id: " + QString::number(_ohData._currentOrderId) + "）不存在";
        uploadNextOrder();
    }
}

void OrderUpload2HG::uploadToHG()
{
    QString url = "http://113.204.136.28/KJClientReceiver/Data.aspx";

    QDomDocument _doc;
    QDomElement root = _doc.createElement("DTC_Message");
    _doc.appendChild(root);

    QDomElement tagMessageHead = _doc.createElement("MessageHead");
    root.appendChild(tagMessageHead);

    QDomElement tag = _doc.createElement("MessageType");
    tagMessageHead.appendChild(tag);
    QDomText t = _doc.createTextNode("ORDER_INFO");
    tag.appendChild(t);

    tag = _doc.createElement("MessageId");
    tagMessageHead.appendChild(tag);
    t = _doc.createTextNode("aaaa");
    tag.appendChild(t);

    QString data = _doc.toString();
    qDebug() << data;

    QFile file("11.xml");
    if (file.open(QIODevice::WriteOnly))
    {
        QTextStream out(&file);
        out << data;
        file.close();
    }

    QMap<QString, QString> paramsMap(g_paramsMap);
    paramsMap["method"] = "Order.SOOutputCustoms";                // 由接口提供方指定的接口标识符
//    paramsMap["method"] = "Inventory.ChannelQ4SAdjustRequest";                // 由接口提供方指定的接口标识符
    paramsMap["timestamp"] = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");       // 调用方时间戳，格式为“4 位年+2 位月+2 位日+2 位小时(24 小时制)+2 位分+2 位秒”
    paramsMap["nonce"] = QString::number(100000 + qrand() % (999999 - 100000)); // QString::number(100000 + qrand() % (999999 - 100000));

    QJsonObject json;
    json["MerchantOrderID"] = "wfx201604050003";      // 商家订单编号
    /// 订单物流运输公司编号
    /// 1=顺丰 2=圆通 84=如风达
    json["ShipTypeID"] = 1;
    json["TrackingNumber"] = "123456";
    json["CommitTime"] = "20151117020101";
    json["Status"] = "1";
    json["Message"] = "aaaa";

    QJsonDocument jsonDoc(json);
    paramsMap["data"] = jsonDoc.toJson(QJsonDocument::Compact);

    QString params;
    QMapIterator<QString, QString> i(paramsMap);
    while (i.hasNext())
    {
        i.next();
        params.append(i.key()).append("=").append(i.value().toLatin1().toPercentEncoding()).append("&");
    }

    qDebug() << params;
    QString sign = QCryptographicHash::hash(params.toLatin1(), QCryptographicHash::Md5).toHex();
    params.append("sign=").append(sign);

//    QFile file("1.txt");
//    QByteArray bb;
//    if (file.open(QIODevice::ReadOnly))
//    {
//        bb = file.readAll();
//        file.close();
//    }

    QNetworkRequest req;
    req.setUrl(QUrl(url));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
//    _manager->post(req, params.toLatin1());
}

void OrderUpload2HG::sReplyFinished(QNetworkReply *reply)
{
    QByteArray replyData = reply->readAll();
    qInfo() << replyData;

    QJsonObject json(QJsonDocument::fromJson(replyData).object());
    bool code = json.value("isSuccess").toBool(false);
    QString body = json.value("body").toString();

    QString opt;
    switch (_optType) {
    case OTOrderUploading:
        opt = tr("新增订单");
        if (code == true)
        {
            QSqlQuery query;

//            QJsonObject data = json.value("Data").toObject();
//            /// 将跨境通的“订单号”存入订单表
//            int orderIdKJT = data.value("SOSysNo").toInt();
//            query.prepare(tr("update 订单 set 第三方订单号=:orderIdKJT "
//                             "where 订单KID=:id "));
//            query.bindValue(":orderIdKJT", QString::number(orderIdKJT));
//            query.bindValue(":id", _ohData._currentOrderId);
//            if (!query.exec())
//                qInfo() << tr("更新订单的第三方订单号: ") << query.lastError().text();
//            /// 将跨境通的“Kjt计算的运费金额”存入订单表
//            /// 将跨境通的“商品跨贸税总金额”存入订单表
//            double shippingAmount = data.value("ShippingAmount").toDouble();
//            double taxAmount = data.value("TaxAmount").toDouble();
//            query.prepare(tr("update 订单 set 订单保价=:shippingAmount, 税金=:taxAmount "
//                             "where 订单KID=:id "));
//            query.bindValue(":shippingAmount", shippingAmount);
//            query.bindValue(":taxAmount", taxAmount);
//            query.bindValue(":id", _ohData._currentOrderId);
//            if (!query.exec())
//                qInfo() << tr("更新订单的Kjt计算的运费金额到订单保价: ") << query.lastError().text();

            /// 记录同步数据，并进行下一个跨境通同步
            query.prepare(tr("update 数据同步 set p2='1' "
                             "where p1='1' and 同步指令='新增' and 同步表名='订单' and 同步主键KID=:id "));
            query.bindValue(":id", _ohData._currentOrderId);
            if (!query.exec())
                qInfo() << tr("更新数据同步的跨境通处理: ") << query.lastError().text();

            _timer->start(1000);
        }
        else
        {
            _success = false;
            /// 不记录此错误，继续下一个订单
//            _optType = OTOrderUploadError;
//            _msg = tr("上传订单错误：") + body;
            _msgList.append("上传订单错误: (" + _ohData._currentOrderNumber + ")" + body);
            _timer->start(1000);
        }
        break;
    default:
        break;
    }

    qInfo() << opt << code << body;
}

void OrderUpload2HG::appendElement(QDomElement &tagParent, const QString &name, const QString &value)
{
    QDomElement tag = _doc.createElement(name);
    tagParent.appendChild(tag);
    QDomText t = _doc.createTextNode(value);
    tag.appendChild(t);
}
