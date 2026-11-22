#include "profile.h"

#include <QUuid>
#include <algorithm>

Profile::Profile(QObject *parent) : QObject(parent), m_userId(QUuid::createUuid().toString(QUuid::WithoutBraces)) {}

Profile::LevelInfo Profile::levelFor(int xp) {
    int level = 1;
    int rest = std::max(0, xp);
    // Bounded so a corrupt save with a huge number cannot spin forever.
    while (level < 10000 && rest >= xpForLevel(level)) {
        rest -= xpForLevel(level);
        ++level;
    }
    return {level, rest, xpForLevel(level)};
}

void Profile::reward(int fish, int xp) {
    if (fish == 0 && xp == 0)
        return;
    m_fish += fish;
    m_xp = std::max(0, m_xp + xp);
    emit progressChanged();
}

bool Profile::spend(int amount) {
    if (amount < 0 || amount > m_fish)
        return false;
    m_fish -= amount;
    emit progressChanged();
    return true;
}

void Profile::resetProgress() {
    m_fish = 0;
    m_xp = 0;
    emit progressChanged();
}

#define PENGU_SETTER(name, member)            \
    void Profile::name(bool on) {             \
        if (member == on) return;             \
        member = on;                          \
        emit settingsChanged();               \
    }
PENGU_SETTER(setNotifications, m_notifications)
PENGU_SETTER(setEveningReminder, m_eveningReminder)
PENGU_SETTER(setMorningReminder, m_morningReminder)
PENGU_SETTER(setStrictStreaks, m_strictStreaks)
PENGU_SETTER(setWeekStartsMonday, m_weekStartsMonday)
#undef PENGU_SETTER

void Profile::setPenguinName(const QString &name) {
    const QString clean = name.trimmed().left(24);
    if (clean.isEmpty() || clean == m_penguinName)
        return;
    m_penguinName = clean;
    emit settingsChanged();
}

QJsonObject Profile::toJson() const {
    return {{"fish", m_fish},
            {"xp", m_xp},
            {"userId", m_userId},
            {"penguinName", m_penguinName},
            {"notifications", m_notifications},
            {"eveningReminder", m_eveningReminder},
            {"morningReminder", m_morningReminder},
            {"strictStreaks", m_strictStreaks},
            {"weekStartsMonday", m_weekStartsMonday}};
}

void Profile::fromJson(const QJsonObject &o) {
    m_fish = o.value("fish").toInt(0);
    m_xp = std::max(0, o.value("xp").toInt(0));
    if (!o.value("userId").toString().isEmpty())
        m_userId = o.value("userId").toString();
    if (!o.value("penguinName").toString().trimmed().isEmpty())
        m_penguinName = o.value("penguinName").toString().trimmed().left(24);
    m_notifications = o.value("notifications").toBool(false);
    m_eveningReminder = o.value("eveningReminder").toBool(false);
    m_morningReminder = o.value("morningReminder").toBool(false);
    m_strictStreaks = o.value("strictStreaks").toBool(false);
    m_weekStartsMonday = o.value("weekStartsMonday").toBool(true);
    emit progressChanged();
    emit settingsChanged();
}
