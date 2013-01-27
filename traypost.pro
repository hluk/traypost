#-------------------------------------------------
#
# Project created by QtCreator 2013-01-26T08:07:59
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = traypost
TEMPLATE = app

SOURCES += main.cpp \
    tray.cpp \
    launcher.cpp \
    console_reader.cpp \
    log_dialog.cpp

HEADERS  += tray.h \
    launcher.h \
    console_reader.h \
    log_dialog.h

QMAKE_CXXFLAGS += -std=c++0x

FORMS += \
    log_dialog.ui
