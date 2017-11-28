#-------------------------------------------------
#
# Project created by QtCreator 2016-10-16T14:52:53
#
#-------------------------------------------------

CONFIG += c++1z

QT += core gui
QT += widgets

TARGET = SeamCarving
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp \
        QtOpencvCore.cpp \
    cv_utility.cpp

HEADERS  += MainWindow.hpp \
        QtOpencvCore.hpp \
    cv_utility.h

FORMS    +=

macx {

    # MAC Compiler Flags
}

win32 {
    # Windows Compiler Flags
}


unix {


#	INCLUDEPATH += /usr/include

#	LIBS += -L/usr/local/lib \
	LIBS +=	-lopencv_core \
			-lopencv_highgui \
			-lopencv_imgproc \
			-lopencv_imgcodecs

#	QMAKE_CXXFLAGS += -std=c++11 -Wall -pedantic -Wno-unknown-pragmas
#	QMAKE_CXXFLAGS_WARN_ON = -Wno-unused-variable -Wno-reorder
}
