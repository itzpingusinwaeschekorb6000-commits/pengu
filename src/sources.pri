# Sources shared by the app, the tests and the screenshot harness, so a new
# file is added in one place for Linux and Android alike.

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/catalog.h \
    $$PWD/game.h \
    $$PWD/habits.h \
    $$PWD/island.h \
    $$PWD/objectmodel.h \
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
    $$PWD/objectmodel.cpp \
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

# A generated header rather than -D flags: make does not rebuild objects for
# changed flags, so they kept old version numbers. qmake rewrites the header
# only when a value changed, and make then rebuilds what includes it.
version_h.input = $$PWD/pengu_version.h.in
version_h.output = $$OUT_PWD/pengu_version.h
QMAKE_SUBSTITUTES += version_h
INCLUDEPATH += $$OUT_PWD
