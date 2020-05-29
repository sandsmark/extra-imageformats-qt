TEMPLATE = lib
CONFIG  += qt plugin
TARGET   = fuif
SOURCES += fuifhandler.cpp
HEADERS += fuif-decl.h

INCLUDEPATH += libfuif
LIBS        += -L$$(FUIFLIB_PATH) -lfuif

QMAKE_CXXFLAGS += -std=c++11
QMAKE_LFLAGS   += -std=c++11

# libfuif (which is dead, so bundled here)

SOURCES += \
    libfuif/image/image.cpp \
    libfuif/transform/transform.cpp \
    libfuif/maniac/bit.cpp \
    libfuif/maniac/chance.cpp \
    libfuif/encoding/encoding.cpp \
    libfuif/io.cpp
