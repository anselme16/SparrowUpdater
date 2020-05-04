TARGET = SparrowUpdater

TEMPLATE = lib
CONFIG += staticlib
CONFIG += c++11

QT -= gui
QT += network

DESTDIR = $$lib_dir

SOURCES += \
    basicupdater.cpp \
    updaterclient.cpp \
    versionupdater.cpp

HEADERS += \
    basicupdater.h \
    updaterclient.h \
    versionupdater.h
