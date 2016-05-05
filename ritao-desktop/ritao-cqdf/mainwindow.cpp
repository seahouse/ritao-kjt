#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTimer>
#include <QDebug>

#include "productupload.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _synchronizeType(STNone)
{
    ui->setupUi(this);

    _timer = new QTimer;
    connect(_timer, SIGNAL(timeout()), this, SLOT(sTimeout()));

    connect(ui->pbnStart, SIGNAL(clicked(bool)), this, SLOT(sStart()));

    _productUpload = new ProductUpload;
    connect(_productUpload, SIGNAL(finished(bool,QString)), this, SLOT(sProductUploadFinished(bool, QString)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::sStart()
{
    ui->pbnStart->setEnabled(false);

    _synchronizeType = STProductUpload;     // 暂时不考虑商品上传功能
//    _synchronizeType = STProductDownload;
    _timer->start(1000);
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
    case STOrderUpload:
//        on_pushButton_4_clicked();
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
    default:
        break;
    }

}

void MainWindow::sProductUploadFinished(bool success, const QString &msg)
{
    QString str = success == true ? "成功" : "失败";
    output(str + ": " + msg);

    _synchronizeType = STOrderUpload;
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
