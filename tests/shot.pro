QT += core gui qml quick quickcontrols2 svg network
CONFIG += c++17
TARGET = shot
TEMPLATE = app

include(../src/sources.pri)
SOURCES += shot.cpp
