QT += core gui quick quickcontrols2 svg network testlib
CONFIG += c++17 testcase
TARGET = pengu_tests
TEMPLATE = app

DEFINES += PENGU_SOURCE_DIR=\\\"$$PWD/../src\\\"

include(../src/sources.pri)
SOURCES += pengu_tests.cpp
