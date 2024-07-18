include(../build_tools/SOUP/libmodbus.pri)

isEmpty(PROGRAM_DIR) {
    PROGRAM_DIR = ../program
}

TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        $$PROGRAM_DIR/main.c \
    $$PROGRAM_DIR/server.c \
    $$PROGRAM_DIR/log.c \
    $$PROGRAM_DIR/proxy.c \
    $$PROGRAM_DIR/raw_request_with_tid.c

HEADERS += \
    $$PROGRAM_DIR/server.h \
    $$PROGRAM_DIR/log.h \
    $$PROGRAM_DIR/proxy.h \
    $$PROGRAM_DIR/raw_request_with_tid.h \
    $$PROGRAM_DIR/modbus_compatibility.h
