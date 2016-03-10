#include "productdownload.h"

#include "global.h"

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

ProductDownload::ProductDownload(QObject *parent) : QObject(parent),
    _optType(OTNone)
{
    _timer = new QTimer;
    connect(_timer, SIGNAL(timeout()), this, SLOT(sTimeout()));

    _manager = new QNetworkAccessManager;
    connect(_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(sReplyFinished(QNetworkReply*)));
}

void ProductDownload::download()
{
    _optType = OTDownloadIdList;
    _phData._success = true;
    _phData._msgList.clear();

    _timer->start(1000);
}

void ProductDownload::sTimeout()
{
    _timer->stop();

    switch (_optType) {
    case OTDownloadIdList:
        downloadProductIdList();
        break;
    case OTProductDownloading:
        downloadNextProducts();
        break;
    case OTProductDownloadEnd:
        emit finished(_phData._success, _phData._msgList.join("\r\n"));
        break;
    case OTProductDownloadError:
        emit finished(false, _msg);
    default:
        break;
    }
}

void ProductDownload::sReplyFinished(QNetworkReply *reply)
{
    QByteArray replyData = reply->readAll();

    QJsonObject json(QJsonDocument::fromJson(replyData).object());
    QString code = json.value("Code").toString("-99");
    QString desc = json.value("Desc").toString();

    QString opt;
    switch (_optType) {
    case OTDownloadIdList:
        if (code == "0")
        {
            /// 读取订单id列表
            QJsonObject data = json.value("Data").toObject();
            _phData._total = data.value("Total").toInt();
            QJsonArray productListArray = data.value("ProductList").toArray();
            QList<ProductSummary> productList;
            for (int i = 0; i < productListArray.size(); i++)
            {
                QJsonObject productObject = productListArray.at(i).toObject();
                productList.append(ProductSummary(
                                       productObject.value("ProductID").toString(),
                                       productObject.value("Status").toInt(),
                                       QDateTime::fromString(productObject.value("ChangeDate").toString())
                                       ));
            }
            _phData._productList = productList;
            _phData._currentIndex = 0;
//            qDebug() << "orderIdList:" << _phData._productList;

            _optType = OTProductDownloading;
            _timer->start(1000);
        }
        else
        {
            _optType = OTProductDownloadError;
            _msg = tr("下载区间订单ID列表错误：") + desc;
            _timer->start(1000);
        }
        break;
    case OTProductDownloading:
        /// 解析数据，写入ERP数据库，并继续请求数据
        if ("0" == code)
        {
            QJsonObject data = json.value("Data").toObject();
            QJsonArray productListArray = data.value("ProductList").toArray();
            for (int i = 0; i < productListArray.size(); i++)
                insertProduct2ERPByJson(productListArray.at(i).toObject());
        }
        _timer->start(1000);
        break;
    default:
        break;
    }

    qInfo() << opt << code << desc;
}

void ProductDownload::downloadProductIdList()
{
    QMap<QString, QString> paramsMap(g_paramsMap);
    paramsMap["method"] = "Product.ProductIDGetQuery";    // 由接口提供方指定的接口标识符
    paramsMap["timestamp"] = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");       // 调用方时间戳，格式为“4 位年+2 位月+2 位日+2 位小时(24 小时制)+2 位分+2 位秒”
    paramsMap["nonce"] = QString::number(100000 + qrand() % (999999 - 100000)); // QString::number(100000 + qrand() % (999999 - 100000));

    /// 获取上次运行的结束日期，作为本次的起始日期
    QDateTime dateStart;
    QSqlQuery query;
    query.prepare(tr("select 参数内容Date from 系统参数 where 参数分组='跨境通' and 参数名称='商品下载结束时间'"));
    if (!query.exec())
        qInfo() << query.lastError().text();
    else if (query.first())
        dateStart = query.value(tr("参数内容Date")).toDateTime();

    if (!dateStart.isValid())
        dateStart = QDateTime(QDate(1900, 1, 1));
    qDebug() << dateStart;
    _phData._dateStart = dateStart;
    _phData._dateEnd = QDateTime::currentDateTime();

    QJsonObject json;
    json["SaleChannelSysNo"] = "1106";
    json["ChangedDateBegin"] = dateStart.toString("yyyy-MM-dd hh:mm:ss");
    json["ChangedDateEnd"] = _phData._dateEnd.toString("yyyy-MM-dd hh:mm:ss");
    QJsonDocument jsonDoc(json);
    QFile file("11.txt");
    if (file.open(QIODevice::WriteOnly))
    {
        QTextStream out(&file);
        out << jsonDoc.toJson(QJsonDocument::Compact);
        file.close();
    }
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
    _manager->get(QNetworkRequest(QUrl(url)));
}

void ProductDownload::downloadNextProducts()
{
    if (_phData._currentIndex >= _phData._total)
    {
        setProductGetFromKJTTime();   // 执行结束，修改系统参数的时间

        _optType = OTProductDownloadEnd;
        if (_phData._success)
            _phData._msgList.append(tr("下载商品结束."));
        _timer->start(1000);
        return;
    }

    QMap<QString, QString> paramsMap(g_paramsMap);
    paramsMap["method"] = "Product.ProudctInfoBatchGet";    // 由接口提供方指定的接口标识符
    paramsMap["timestamp"] = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");       // 调用方时间戳，格式为“4 位年+2 位月+2 位日+2 位小时(24 小时制)+2 位分+2 位秒”
    paramsMap["nonce"] = QString::number(100000 + qrand() % (999999 - 100000)); // QString::number(100000 + qrand() % (999999 - 100000));

    QJsonObject json;
    QJsonArray productIdListArray;
    foreach (ProductSummary product, _phData._productList.mid(_phData._currentIndex, 5)) {
        productIdListArray.append(QJsonValue(product._productId));
    }
    _phData._currentIndex += 5;
    json["ProductIDs"] = productIdListArray;
    json["SaleChannelSysNo"] = "1106";
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

    params.append(kjt_secretkey);
    urlencodePercentConvert(params);
    qDebug() << params;
    QString sign = QCryptographicHash::hash(params.toLatin1(), QCryptographicHash::Md5).toHex();

    QString url;
    url.append(kjt_url).append("?").append(params).append("&sign=").append(sign);

    qDebug() << url;
    _manager->get(QNetworkRequest(QUrl(url)));
}

void ProductDownload::setProductGetFromKJTTime()
{
    if (!_phData._success) return;
    if (!_phData._dateStart.isValid() || !_phData._dateEnd.isValid()) return;

    QSqlQuery query;
    query.prepare(tr("update 系统参数 set 参数内容Date=:dateStart where 参数分组='跨境通' and 参数名称='商品下载起始时间'"));
    query.bindValue(":dateStart", _phData._dateStart);
    if (!query.exec())
    {
        qInfo() << query.lastError().text();
    }

    query.prepare(tr("update 系统参数 set 参数内容Date=:dateStart where 参数分组='跨境通' and 参数名称='订单下载结束时间'"));
    query.bindValue(":dateStart", _phData._dateEnd);
    if (!query.exec())
    {
        qInfo() << query.lastError().text();
    }
}

void ProductDownload::insertProduct2ERPByJson(const QJsonObject &json)
{
    QString productId = json.value("ProductID").toString();             // ProductID
    int categoryID = json.value("CategoryID").toInt();                  // 商品类别ID
    QString categoryName = json.value("CategoryName").toString();       // 商品类别名称
    QString productName = json.value("ProductName").toString();         // 商品名称
    QString briefName = json.value("BriefName").toString();             // 商品简称
    QString productDescLong = json.value("ProductDescLong").toString(); // 详细描述
    QString productPhotoDesc = json.value("ProductPhotoDesc").toString();   // 以图片方式展示的详细描述
    QString performance = json.value("Performance").toString();         // 详细规格
    QString warranty = json.value("Warranty").toString();               // 售后服务
    QString attention = json.value("Attention").toString();             // 购买须知
    QString defaultImage = json.value("DefaultImage").toString();       // 默认图片
    QString promotionTitle = json.value("PromotionTitle").toString();   // 促销标题
    QString keyWords = json.value("KeyWords").toString();               // 关键字
    int vendorID = json.value("VendorID").toInt();                      // 供应商名称
    int brandID = json.value("BrandID").toInt();                        // 品牌编号
    int productTradeType = json.value("ProductTradeType").toInt();      // 贸易类型：0 = 直邮, 1 = 自贸
    int onlineQty = json.value("OnlineQty").toInt();                    // 渠道独占库存
    int platformQty = json.value("PlatformQty").toInt();                // 平台可售库存
    double price = json.value("Price").toDouble();                      // 渠道分销价格
    int storeSysNo = json.value("StoreSysNo").toInt();                  // 店铺编号
//    QDateTime orderDate = convertKjtTime(json.value("OrderDate").toString());           // 订单时间

    QJsonObject productEntryInfoObject = json.value("ProductEntryInfo").toObject();         // 商品库存列表
    QString productName_EN = productEntryInfoObject.value("ProductName_EN").toString();     // 商品英文名称
    QString Specifications = productEntryInfoObject.value("Specifications").toString();     // 规格
    QString functions = productEntryInfoObject.value("Functions").toString();               // 功能
    QString component = productEntryInfoObject.value("Component").toString();               // 成分
    QString origin = productEntryInfoObject.value("Origin").toString();                     // 产地
    QString purpose = productEntryInfoObject.value("Purpose").toString();                   // 用途
    QString taxUnit = productEntryInfoObject.value("TaxUnit").toString();                   // 计税单位
    QString applyUnit = productEntryInfoObject.value("ApplyUnit").toString();               // 申报单位
    double taxQty = productEntryInfoObject.value("TaxQty").toDouble();                      // 计税单位数量
    double grossWeight = productEntryInfoObject.value("GrossWeight").toDouble();            // 毛重
    int bizType = productEntryInfoObject.value("BizType").toInt();                          // 业务类型: 0 =一般进口, 1=保税进口
    double suttleWeight = productEntryInfoObject.value("SuttleWeight").toDouble();          // 净重
    QString note = productEntryInfoObject.value("Note").toString();                         // 其他备注
    double tariffRate = productEntryInfoObject.value("TariffRate").toDouble();              // 税率
    QString entryCode = productEntryInfoObject.value("EntryCode").toString();               // 备案信息
    int productStoreType = productEntryInfoObject.value("ProductStoreType").toInt();        // 存储方式: 0 =常温, 1 =冷藏, 2=冷冻
    QDateTime manufactureDate = convertKjtTime(json.value("ManufactureDate").toString());   // 出厂日期
    QString originCountryName = productEntryInfoObject.value("OriginCountryName").toString();   // 出产国家




    qDebug() << productId << productName;
//    if (10022053 != orderId) return;

    /// 仅下载付款后，待出库的订单
//    if (sOStatusCode != 1) return;

//    QSqlQuery query;
//    query.prepare(tr("select * from 订单 where 订单类型=:OrderType and 第三方订单号=:MerchantOrderID"));
//    query.bindValue(":OrderType", "kjt");
//    query.bindValue(":MerchantOrderID", QString::number(orderId));
//    if (!query.exec())
//    {
//        qInfo() << query.lastError().text();
//        _phData._success = false;
//    }
//    /// 已存在此订单编号，直接返回
//    if (query.first())
//    {
//        qInfo() << tr("此订单已存在。kjt系统订单号：")  + QString::number(orderId);
//        return;
//    }

//    query.prepare(tr("insert into 订单("
//                     "订单号, 订单类型, 第三方订单号, 下单日期, 跨境通订单状态, "
//                     "贸易类型, 审核日期, 发货日期, 商品总金额, 配送费用, "
//                     "税金, 支付手续费, 支付方式, 支付流水号, 付款状态, "
//                     "收货人, 手机号码, 收货地址, 邮政编码, 发件人姓名, "
//                     "发件人电话, 发件人地址, 发件地邮编, 注册地址, 运单号, 个人姓名, "
//                     "纳税人识别号, 注册电话, 电子邮件, 获取时间 "
//                     ") values ("
//                     ":OrderID, :OrderType, :MerchantOrderID, :OrderDate, :SOStatusCode, "
//                     ":TradeType, :AuditTime, :SOOutStockTime, :ProductAmount, :ShippingAmount, "
//                     ":TaxAmount, :CommissionAmount, :PayTypeSysNo, :PaySerialNumber, :PayStatusCode, "
//                     ":ReceiveName, :ReceivePhone, :ReceiveAddress, :ReceiveZip, :SenderName, "
//                     ":SenderTel, :SenderAddr, :SenderZip, :ReceiveAreaName, :TrackingNumber, :Name, "
//                     ":IDCardNumber, :PhoneNumber, :Email, :GetTime "
//                     ")"));

//    /// 订单号规则
//    /// 01（表示主订单） + 两位随机数 + 当前时间的日（两位） + 当前时间的月+70（两位） + 四位随机数
//    QString orderNumber = QString("%1%2%3%4%5")
//            .arg("01")
//            .arg(QString::number(qrand() % 100), 2, '0')
//            .arg(QDate::currentDate().toString("dd"))
//            .arg(QString::number(QDate::currentDate().month() + 70))
//            .arg(QString::number(qrand() % 10000), 4, '0');
//    query.bindValue(":OrderID", orderNumber);
//    query.bindValue(":OrderType", "kjt");               // 固定为 kjt
//    query.bindValue(":MerchantOrderID", orderId);
//    query.bindValue(":OrderDate", orderDate);
//    query.bindValue(":SOStatusCode", sOStatusCode);

//    query.bindValue(":TradeType", tradeType);
//    query.bindValue(":AuditTime", auditTime);
//    query.bindValue(":SOOutStockTime", sOOutStockTime);
//    query.bindValue(":ProductAmount", productAmount);
//    query.bindValue(":ShippingAmount", shippingAmount);

//    query.bindValue(":TaxAmount", taxAmount);
//    query.bindValue(":CommissionAmount", commissionAmount);
//    query.bindValue(":PayTypeSysNo", payTypeSysNo);
//    query.bindValue(":PaySerialNumber", paySerialNumber.isNull() ? "" : paySerialNumber);
//    query.bindValue(":PayStatusCode", payStatusCode);

//    query.bindValue(":ReceiveName", receiveName);
//    query.bindValue(":ReceivePhone", receivePhone.isNull() ? "" : receivePhone);
//    query.bindValue(":ReceiveAddress", receiveAddress);
//    query.bindValue(":ReceiveZip", receiveZip);
//    query.bindValue(":ReceiveAreaName", receiveAreaName.isNull() ? "" : receiveAreaName);
//    query.bindValue(":SenderName", senderName.isNull() ? "" : senderName);

//    query.bindValue(":SenderTel", senderTel.isNull() ? "" : senderTel);
//    query.bindValue(":SenderAddr", senderAddr.isNull() ? "" : senderAddr);
//    query.bindValue(":SenderZip", senderZip.isNull() ? "" : senderZip);
//    query.bindValue(":TrackingNumber", trackingNumber.isNull() ? "" : trackingNumber);
//    query.bindValue(":Name", name);

//    query.bindValue(":IDCardNumber", iDCardNumber);
//    query.bindValue(":PhoneNumber", phoneNumber.isNull() ? "" : phoneNumber);
//    query.bindValue(":Email", email);
//    query.bindValue(":GetTime", QDateTime::currentDateTime());              // 同步时间为当前时间
//    if (!query.exec())
//    {
////        qFatal(query.lastError().text().toStdString().c_str());
//        qInfo() << query.lastError().text();
//        _phData._success = false;
//    }
//    else
//    {
//        /// 将来插入库存信息数据
//        int parentId = query.lastInsertId().toInt();        // 订单id
//        if (parentId > 0)
//        {
//            QSqlQuery queryItemInfo;
//            foreach (SOItemInfo sOitemInfo, sOItemInfoList) {
//                queryItemInfo.prepare(tr("insert into 订单商品 ("
//                                         "订单id, 商品名称, 商品编号, 购买数量, 销售单价, "
//                                         "税金 "
//                                         ") values ("
//                                         ":OrderId, :ProductName, :ProductID, :Quantity, :ProductPrice, "
//                                         ":TaxPrice "
//                                         ")"));
//                queryItemInfo.bindValue(":OrderId", parentId);
//                queryItemInfo.bindValue(":ProductName", sOitemInfo._productName.isNull() ? "" : sOitemInfo._productName);
//                queryItemInfo.bindValue(":ProductID", sOitemInfo._productId.isNull() ? "" : sOitemInfo._productId);
//                queryItemInfo.bindValue(":Quantity", sOitemInfo._quantity);
//                queryItemInfo.bindValue(":ProductPrice", sOitemInfo._productPrice);
//                queryItemInfo.bindValue(":TaxPrice", sOitemInfo._taxPrice);

//                if (!queryItemInfo.exec())
//                {
//                    qInfo() << queryItemInfo.lastError().text();
//                    _phData._success = false;
//                }
//            }
//        }

//    }

}

QDateTime ProductDownload::convertKjtTime(const QString &kjtTime)
{
    /// 跨镜通将会把日期修改为 yyyy-MM-dd hh:mm:ss 形式。到时再修改此处
    qint64 utcTime = kjtTime.mid(kjtTime.indexOf("(") + 1, 13).toLongLong();
    return QDateTime::fromMSecsSinceEpoch(utcTime);      // 订单时间
}
