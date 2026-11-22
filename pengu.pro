QT += core gui qml quick quickcontrols2 svg

CONFIG += c++17 release
TARGET = pengu
TEMPLATE = app

HEADERS += \
    src/catalog.h \
    src/game.h \
    src/habits.h \
    src/island.h \
    src/profile.h \
    src/reminders.h \
    src/theme.h \
    src/tilemap.h \
    src/translator.h

SOURCES += \
    src/main.cpp \
    src/catalog.cpp \
    src/game.cpp \
    src/habits.cpp \
    src/island.cpp \
    src/profile.cpp \
    src/reminders.cpp \
    src/theme.cpp \
    src/tilemap.cpp \
    src/translator.cpp

RESOURCES += src/resources.qrc
