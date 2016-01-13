#-------------------------------------------------
#
# Project created by QtCreator 2015-12-30T16:17:47
#
#-------------------------------------------------

QT       += core gui
QT += network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ritao-kjt
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    ordercreatekjttoerp.cpp \
    global.cpp \
    rnetworkaccessmanager.cpp \
    orderupload.cpp

HEADERS  += mainwindow.h \
    connection.h \
    global.h \
    ordercreatekjttoerp.h \
    rnetworkaccessmanager.h \
    orderupload.h

FORMS    += mainwindow.ui
