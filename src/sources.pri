# Sources shared by the app, the tests and the screenshot harness, so a new
# file is added in one place for Linux and Android alike.

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/catalog.h \
    $$PWD/game.h \
    $$PWD/habits.h \
    $$PWD/island.h \
    $$PWD/profile.h \
    $$PWD/reminders.h \
    $$PWD/theme.h \
    $$PWD/tilemap.h \
    $$PWD/translator.h \
    $$PWD/updater.h

SOURCES += \
    $$PWD/catalog.cpp \
    $$PWD/game.cpp \
    $$PWD/habits.cpp \
    $$PWD/island.cpp \
    $$PWD/profile.cpp \
    $$PWD/reminders.cpp \
    $$PWD/theme.cpp \
    $$PWD/tilemap.cpp \
    $$PWD/translator.cpp \
    $$PWD/updater.cpp

RESOURCES += $$PWD/resources.qrc

# One version for every platform: the marketing version from VERSION plus the
# build number, which is the commit count (CI passes PENGU_BUILD explicitly).
# The updater compares build numbers, so every pushed commit is newer.
PENGU_BUILD = $$(PENGU_BUILD)
isEmpty(PENGU_BUILD): PENGU_BUILD = $$system(git -C $$PWD rev-list --count HEAD 2>/dev/null)
isEmpty(PENGU_BUILD): PENGU_BUILD = 0
PENGU_VERSION = $$cat($$PWD/../VERSION).$$PENGU_BUILD
# The GitHub repository releases are published to (owner/name).
PENGU_REPO = $$cat($$PWD/../REPO)

DEFINES += PENGU_BUILD=$$PENGU_BUILD \
           PENGU_VERSION=\\\"$$PENGU_VERSION\\\" \
           PENGU_REPO=\\\"$$PENGU_REPO\\\"
