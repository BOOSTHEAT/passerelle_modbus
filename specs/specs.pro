include(gtest_dependency.pri)
include(../build_tools/SOUP/libmodbus.pri)

TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG += thread
CONFIG -= qt

INCLUDEPATH += ../program

HEADERS += \
    ServerBase.h \
    test_ServerSingleClient.h \
    test_ServerMultipleClients.h \
    modbus_helper.h \
    test_RequestProxy.h

SOURCES += \
        main.cpp \
    ../program/server.c \
    ../program/log.c \
    modbus_helper.cpp \
    ../program/proxy.c \
    ../program/raw_request_with_tid.c
