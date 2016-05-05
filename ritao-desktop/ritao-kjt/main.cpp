#include "mainwindow.h"
#include <QApplication>

#include "connection.h"

#include <QDir>
#include <QDate>
#include <QTextStream>
#include <QTextCodec>

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QDir dir(".");
    if (!dir.exists("logs"))
        dir.mkdir("logs");
    QFile file("logs/" + QDate::currentDate().toString("yyyy-MM-dd") + ".txt");
    if (file.open(QIODevice::Append))
    {
        QTextStream out(&file);
        QByteArray localMsg = msg.toLocal8Bit();
        switch (type) {
        case QtDebugMsg:
//            out << QString("Debug: %1 (%2:%3, %4)\n")
//                   .arg(localMsg.constData())
//                   .arg(context.file)
//                   .arg(QString::number(context.line))
//                   .arg(context.function);
            fprintf(stdout, "%s\n", localMsg.constData());
            fflush(stdout);
            break;
        case QtInfoMsg:
            out << QString("Info: (%1) %2\r\n")          //  (%2:%3, %4)
                   .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                   .arg(localMsg.constData())
//                   .arg(context.file)
//                   .arg(QString::number(context.line))
//                   .arg(context.function)
                   ;
            fprintf(stdout, "%s\n", localMsg.constData());
            fflush(stdout);
            break;
        case QtFatalMsg:
            out << QString("Fatal: (%1) %2\r\n")            // (%2:%3, %4)
                   .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                   .arg(localMsg.constData())
//                   .arg(context.file)
//                   .arg(QString::number(context.line))
//                   .arg(context.function)
                   ;
            fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            fflush(stderr);
            break;
        case QtSystemMsg:
            out << QString("Critical: %1 (%2:%3, %4)\n")
                   .arg(localMsg.constData())
                   .arg(context.file)
                   .arg(QString::number(context.line))
                   .arg(context.function);
            fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            break;
        default:
            break;
        }

        file.close();
    }

}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(myMessageOutput);
    QApplication a(argc, argv);

    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(codec);

    if (!createConnection())
        return -1;

    MainWindow w;
    w.show();

    return a.exec();
}
