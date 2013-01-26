#-------------------------------------------------
#
# Project created by QtCreator 2013-01-26T08:07:59
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = traypost
TEMPLATE = app


SOURCES += main.cpp tray.cpp

HEADERS  += tray.h

QMAKE_CXXFLAGS += -std=c++0x
