#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "configglobal.h"

#include <QTimer>
#include <QDebug>

#include "productupload.h"
#include "orderupload2hg.h"
#include "orderupload.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _synchronizeType(STNone)
{
    ui->setupUi(this);

    g_paramsMap["appkey"] = g_config.cqdfAppkey();
    g_paramsMap["format"] = g_config.cqdfFormat();               // 接口返回结果类型:json
    g_paramsMap["encrypt"] = g_config.cqdfEncrypt();               // 由接口提供方指定的接口版本

    _timer = new QTimer;
    connect(_timer, SIGNAL(timeout()), this, SLOT(sTimeout()));

    connect(ui->pbnStart, SIGNAL(clicked(bool)), this, SLOT(sStart()));

    _productUpload = new ProductUpload;
    connect(_productUpload, SIGNAL(finished(bool,QString)), this, SLOT(sProductUploadFinished(bool, QString)));

    _orderUpload2HG = new OrderUpload2HG;
    connect(_orderUpload2HG, SIGNAL(finished(bool,QString)), this, SLOT(sOrderUpload2HGFinished(bool, QString)));

    _orderUpload = new OrderUpload;
    connect(_orderUpload, SIGNAL(finished(bool,QString)), this, SLOT(sOrderUploadFinished(bool, QString)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::sStart()
{
    ui->pbnStart->setEnabled(false);

    _synchronizeType = STProductUpload;
//    _synchronizeType = STProductDownload;
    _timer->start(1000);
}

void MainWindow::sEnd()
{
    ui->pbnStart->setEnabled(true);

    _synchronizeType = STNone;
//    _synchronizeType = STProductDownload;
    _timer->start(g_config.intervalMinutes() * 60 * 1000);     // 30分钟
}

void MainWindow::sTimeout()
{
    _timer->stop();

    switch (_synchronizeType) {
    case STProductUpload:
        _productUpload->upload();
//        on_pushButton_6_clicked();
        break;
    case STProductDownload:
//        sDownloadProduct();
        break;
    case STOrderUpload2HG:
        _orderUpload2HG->upload();
        break;
    case STOrderUpload:
        _orderUpload->upload();
        break;
    case STOrderDownload:
//        on_pushButton_7_clicked();
        break;
//    case STProductCreate:
//        synchronizeProductCreate();
//        break;
    case STOrderInfoBatchGet:
//        orderInfoBatchGet();
        break;
    case STNone:
        sStart();
        break;
    case STMax:
        sEnd();
        break;
    default:
        break;
    }

}

void MainWindow::sProductUploadFinished(bool success, const QString &msg)
{
    QString str = success == true ? "成功" : "失败";
    output(str + ": " + msg);

    _synchronizeType = STOrderUpload2HG;
    _timer->start(1000);
}

void MainWindow::sOrderUpload2HGFinished(bool success, const QString &msg)
{
    QString str = success == true ? "成功" : "失败";
    output(str + ": " + msg);

    _synchronizeType = STOrderUpload;
    _timer->start(1000);
}

void MainWindow::sOrderUploadFinished(bool success, const QString &msg)
{
    QString str = success == true ? "成功" : "失败";
    output(str + ": " + msg);

    _synchronizeType = STMax;
    _timer->start(1000);
}

void MainWindow::output(const QString &msg, MsgType type)
{
    ui->textEdit->append(msg);

    switch (type) {
    case MTDebug:
        qDebug() << msg;
        break;
    case MTInfo:
        qInfo() << msg;
        break;
    default:
        break;
    }
}

void MainWindow::on_pushButton_clicked()
{
    _orderUpload2HG->uploadToHG();
}
