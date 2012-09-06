#-------------------------------------------------
#
# Project created by QtCreator 2012-09-01T01:00:07
#
#-------------------------------------------------

QT       += core gui

TARGET = rover
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    mediabrowserqlistview.cpp

HEADERS  += mainwindow.h \
    mediabrowserqlistview.h

INCLUDEPATH += /usr/inlude/
#INCLUDEPATH += /usr/inlude/opencv2

LIBS +=  \
-lopencv_core \
-lopencv_imgproc \
-lopencv_highgui \
-lopencv_ml \
-lopencv_video \
-lopencv_features2d \
-lopencv_calib3d \
-lopencv_objdetect \
-lopencv_contrib \
-lopencv_legacy \
-lopencv_flann
#LIBS += -lcv -lhighgui

FORMS    += mainwindow.ui
