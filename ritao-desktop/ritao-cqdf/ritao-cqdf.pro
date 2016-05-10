#-------------------------------------------------
#
# Project created by QtCreator 2016-05-05T10:09:01
#
#-------------------------------------------------

QT       += core gui
QT += sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ritao-cqdf
TEMPLATE = app

DESTDIR = ../ritao_desktop_bin_cqdf

SOURCES += main.cpp\
        mainwindow.cpp \
    productupload.cpp \
    global.cpp \
    configglobal.cpp

HEADERS  += mainwindow.h \
    productupload.h \
    global.h \
    configglobal.h \
    connection.h

FORMS    += mainwindow.ui
