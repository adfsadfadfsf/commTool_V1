#-------------------------------------------------
#
# Project created by QtCreator 2017-06-08T16:20:14
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = commTool
TEMPLATE = app


SOURCES += main.cpp\
        widget.cpp

HEADERS  += widget.h

FORMS    += widget.ui

RESOURCES += \
    image.qrc

RC_FILE += myico.rc
