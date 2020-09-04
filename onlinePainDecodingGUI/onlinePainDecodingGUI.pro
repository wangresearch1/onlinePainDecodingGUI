#-------------------------------------------------
#
# Project created by QtCreator 2017-04-07T11:42:30
#
#-------------------------------------------------

QT       += core gui
QT += printsupport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = onlinePainDecodingGUI
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
        paindetectclient.cpp \
    qcustomplot.cpp \
    algorithms.cpp \
    configManager.cpp \
    dialog.cpp \
    dodialog.cpp \
    welcomedialog.cpp \
    dataCollector.cpp \
    plexDoManager.cpp \
    plexDoManager.cpp

HEADERS  += paindetectclient.h \
    qcustomplot.h \
    qcustomplot.h \
    algorithms.h \
    bufferManager.h \
    configManager.h \
    INIReader.h \
    dialog.h \
    dodialog.h \
    welcomedialog.h \
    dataCollector.h \
    qthreadmanager.h \
    plexDoManager.h \
    plexDoManager.h

FORMS    += paindetectclient.ui \
    dialog.ui \
    dodialog.ui \
    welcomedialog.ui

INCLUDEPATH += D:\Silas\eigen \
               D:\Silas\CandC++ClientDevelopmentKit_0\ClientSDK\include \
               D:\Silas\LBFGS\include \
#               'C:\Users\Plexon\Documents\IC Imaging Control 3.4\classlib\include' \
#               'C:\Users\Plexon\Documents\IC Imaging Control 3.4\samples\vc10\Common'
#INCLUDEPATH += E:\Silas\eigen \
#               E:\Silas\CandC++ClientDevelopmentKit_0\ClientSDK\include \
#               E:\Silas\LBFGS\include \

LIBS += -LD:\Silas\CandC++ClientDevelopmentKit_0\ClientSDK\lib -lPlexClient \
        -LD:\Silas\CandC++ClientDevelopmentKit_0\ClientSDK\lib -lPlexDO \
#        -LD:\Silas\PlexDoExpClient_v2\PlexDoExpClient_v2 -lTIS_UDSHL11d \
#LIBS += -LE:\Silas\CandC++ClientDevelopmentKit_0\ClientSDK\lib -lPlexClient \
#        -LE:\Silas\CandC++ClientDevelopmentKit_0\ClientSDK\lib -lPlexDO \

DISTFILES +=

