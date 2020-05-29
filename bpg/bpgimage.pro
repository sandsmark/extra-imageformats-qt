TEMPLATE = lib
CONFIG  += qt plugin
TARGET   = bpg
SOURCES += bpghandler.cpp
HEADERS += bpg-decl.h
OTHER_FILES += extensions.json

BPGLIB_PATH = $$(BPGLIB_PATH)
isEmpty( BPGLIB_PATH ) {
  BPGLIB_PATH = /usr/local/lib64
}

BPGINC_PATH = $$(BPGINC_PATH)
!isEmpty( BPGINC_PATH ) {
  INCLUDEPATH += $$BPGINC_PATH
}

LIBS        += -L$$BPGLIB_PATH -lbpg
