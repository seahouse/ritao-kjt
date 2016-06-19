#-------------------------------------------------
#
# Project created by QtCreator 2016-05-05T10:09:01
#
#-------------------------------------------------

QT       += core gui
QT += sql network xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ritao-cqdf
TEMPLATE = app

DESTDIR = ../ritao_desktop_bin_cqdf

SOURCES += main.cpp\
        mainwindow.cpp \
    productupload.cpp \
    global.cpp \
    configglobal.cpp \
    orderupload.cpp \
    orderupload2hg.cpp \
    productupload2hg.cpp

HEADERS  += mainwindow.h \
    productupload.h \
    global.h \
    configglobal.h \
    connection.h \
    orderupload.h \
    orderupload2hg.h \
    productupload2hg.h

FORMS    += mainwindow.ui
