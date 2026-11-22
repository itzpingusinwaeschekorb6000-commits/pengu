QT += core gui quick quickcontrols2 svg testlib
CONFIG += c++17 testcase
TARGET = pengu_tests
TEMPLATE = app

INCLUDEPATH += ../src
DEFINES += PENGU_SOURCE_DIR=\\\"$$PWD/../src\\\"

HEADERS += ../src/catalog.h ../src/game.h ../src/habits.h ../src/island.h ../src/profile.h \
           ../src/reminders.h ../src/theme.h ../src/translator.h
SOURCES += pengu_tests.cpp ../src/catalog.cpp ../src/game.cpp ../src/habits.cpp ../src/island.cpp \
           ../src/profile.cpp ../src/reminders.cpp ../src/theme.cpp ../src/translator.cpp
RESOURCES += ../src/resources.qrc
