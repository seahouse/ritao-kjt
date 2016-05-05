#ifndef CONNECTION
#define CONNECTION

#include "configglobal.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>

static bool createConnection()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid())
    {
        db = QSqlDatabase::addDatabase("QODBC");
#ifndef QT_NO_DEBUG
        db.setDatabaseName("DRIVER={SQL SERVER};SERVER=" + g_config.dbHost() + ";DATABASE=" + g_config.dbName() + ";");
        db.setUserName(g_config.dbUsername());
        db.setPassword(g_config.dbPassword());
//        db.setDatabaseName("DRIVER={SQL SERVER};SERVER=120.55.165.90,9001;DATABASE=dayamoy;");
//        db.setUserName("dayamoyadmin");
//        db.setPassword("dayamoyadmin2015");
#else
        db.setDatabaseName("DRIVER={SQL SERVER};SERVER=120.55.165.90,9001;DATABASE=dayamoy;");
        db.setUserName("dayamoyadmin");
        db.setPassword("dayamoyadmin2015");
#endif

    }

//    QString dns = "DRIVER={SQL SERVER};SERVER=" + hostName + ";DATABASE=" + dbName + ";";


    if (!db.open()) {
        QMessageBox::critical(0, QObject::tr("Cannot open database"),
            QString(QObject::tr("连接数据库失败: ") + db.lastError().text() + "\n" +
                     QObject::tr("请确保连接信息正确，如需帮助，请与数据管理员联系。\n\n") +
                     QObject::tr("点击'取消'键关闭。")), QMessageBox::Cancel);
        return false;
    }

    return true;
}

#endif // CONNECTION

