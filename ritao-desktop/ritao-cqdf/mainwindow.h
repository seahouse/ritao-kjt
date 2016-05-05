#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "global.h"

class ProductUpload;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum SynchronizeType
    {
        STNone,
        STProductUpload,                // 上传ERP商品到地服
        STProductDownload,              // 下载跨境通商品到ERP
        STProductPriceDownload,         // 下载跨境通商品价格到ERP
        STOrderUpload,                  // 上传ERP订单到跨境通
        STOrderDownload,                // 下载跨境通订单到ERP
        STOrderCreateKJTToERP,          // 获取时间区间内的的订单id列表
        STOrderInfoBatchGet,            // 获取订单详细信息并写入ERP数据库
    };

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void sStart();
    void sTimeout();

    void sProductUploadFinished(bool success, const QString &msg);

private:
    void output(const QString &msg, MsgType type = MTDebug);          // 将信息输出

private:
    Ui::MainWindow *ui;

    QTimer *_timer;

    SynchronizeType _synchronizeType;

    ProductUpload *_productUpload;
};

#endif // MAINWINDOW_H
