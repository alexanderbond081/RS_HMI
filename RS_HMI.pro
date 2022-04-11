#-------------------------------------------------
#
# Project created by QtCreator 2014-02-28T13:00:23
#
#-------------------------------------------------

QT += core gui
QT += serialport
CONFIG += c++11
CONFIG += debug
#CONFIG += warn_off
#CONFIG += -Wno-zero-as-null-pointer-constant
#QMAKE_CXXFLAGS += -Wno-expansion-to-defined -Wno-zero-as-null-pointer-constant -Wno-old-style-cast
#QMAKE_CXXFLAGS_WARN_OFF += -Wno-zero-as-null-pointer-constant
#QMAKE_CXXFLAGS_WARN_ON += -Wno-zero-as-null-pointer-constant
#QMAKE_CXX += -Wno-zero-as-null-pointer-constant

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = RS_HMI
TEMPLATE = app


SOURCES += main.cpp\
    about.cpp \
    dataeditor.cpp \
        mainwindow.cpp \
    datacollection.cpp \
    fatekcommrs.cpp \
    poweroff.cpp \
    viewelement.cpp \
    viewtabcollection.cpp \
    plcregister.cpp \
    hmi.cpp \
    resources.cpp \
    commwindow.cpp \
    plccommrs485.cpp \
    datasaveload.cpp \
    hmisecurity.cpp \
    multilist.cpp \
    customdelegates.cpp \
    logmodel.cpp \
    diagramlog.cpp

HEADERS  += mainwindow.h \
    about.h \
    datacollection.h \
    dataeditor.h \
    fatekcommrs.h \
    poweroff.h \
    registerIOinterface.h \
    viewelement.h \
    viewtabcollection.h \
    resources.h \
    plcregister.h \
    plcregistervalidator.h \
    hmi.h \
    commwindow.h \
    plccommrs485.h \
    datasaveload.h \
    hmisecurity.h \
    multilist.h \
    customdelegates.h \
    logmodel.h \
    diagramlog.h

FORMS    += mainwindow.ui \
    about.ui \
    commwindow.ui

OTHER_FILES += \
    viewData.xml

RESOURCES += \
    icons.qrc

DISTFILES += \
    toDoList.txt
