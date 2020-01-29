#-------------------------------------------------
#
# Project created by QtCreator 2019-11-27T10:24:23
#
#-------------------------------------------------

QT       += core gui serialport network
QT       += httpserver

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qApp
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        ../../src/core/drivers/ExtControl.cpp \
        helper_and_stubs.cpp \
        httpserver.cpp \
        main.cpp \
        mainwindow.cpp \
        options.cpp \
        serial.cpp

HEADERS += \
        ../../src/core/drivers/ExtControl.h \
        helper_and_stubs.h \
        httpserver.h \
        mainwindow.h \
        ProgramData.h \
        options.h \
        serial.h
FORMS += \
        mainwindow.ui \
        options.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
DEFINES += ENABLE_EXTERNAL_CONTROL
DEFINES += EXTCONTROL_DEBUG
DEFINES += QT
unix:QMAKE_RPATHDIR += /home/jose/Qt/5.12.2/gcc_64/lib
