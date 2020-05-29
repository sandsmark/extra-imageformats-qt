TEMPLATE = lib
CONFIG  += qt plugin
TARGET   = fuif
SOURCES += fuifhandler.cpp

INCLUDEPATH += $$(FUIFINC_PATH)
LIBS        += -L$$(FUIFLIB_PATH) -lfuif

QMAKE_CXXFLAGS += -std=c++11
QMAKE_LFLAGS   += -std=c++11
