TARGET = SparrowUpdater

TEMPLATE = lib
CONFIG += staticlib
CONFIG += c++11

QT -= gui
QT += network

DESTDIR = $$lib_dir

SOURCES += \
    updaterclient.cpp \
    versionupdater.cpp

HEADERS += \
    updaterclient.h \
    versionupdater.h
