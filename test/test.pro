TARGET = test

TEMPLATE = app
CONFIG += c++11

QT += widgets network

DESTDIR = $$bin_dir

LIBS += -lSparrowUpdater
LIBPATH += $$lib_dir

INCLUDEPATH += $$src_dir

DEFINES += GIT_CURRENT=\\\"$$version_git\\\"

SOURCES += \
    main.cpp \
    mainwindow.cpp

FORMS += \
    mainwindow.ui

HEADERS += \
    mainwindow.h
