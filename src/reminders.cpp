#include "reminders.h"

#include "habits.h"
#include "profile.h"

#include <QProcess>
#include <QStandardPaths>

Reminders::Reminders(HabitBook *habits, Profile *profile, QObject *parent)
    : QObject(parent), m_habits(habits), m_profile(profile) {
    m_sender = [](const QString &title, const QString &body) {
        const QString notify = QStandardPaths::findExecutable("notify-send");
        if (!notify.isEmpty())
            QProcess::startDetached(notify, {"--app-name=Pengu", "--icon=pengu", title, body});
    };
    m_timer.setInterval(20 * 1000);
    connect(&m_timer, &QTimer::timeout, this, [this] { check(QDateTime::currentDateTime()); });
    m_timer.start();
}

bool Reminders::due(const QTime &at, const QTime &now) const {
    // A window rather than an exact minute, so a sleeping laptop or a busy
    // timer still delivers the reminder shortly after its time.
    // Late reminders (23:45) would wrap past midnight, so their window ends at
    // the end of the day instead.
    if (!at.isValid() || now < at)
        return false;
    const QTime end = at.addSecs(WindowMinutes * 60);
    return end < at || now < end;
}

void Reminders::fire(const QString &key, const QString &title, const QString &body) {
    if (m_fired.contains(key))
        return;
    m_fired.insert(key);
    m_sender(title, body);
}

void Reminders::check(const QDateTime &now) {
    if (!m_profile->notifications())
        return;
    const QString day = now.date().toString(Qt::ISODate);
    const QTime time = now.time();
    const QString name = m_profile->penguinName();

    if (m_profile->morningReminder() && due(QTime(8, 0), time))
        fire("morning:" + day, tr("Good morning!"),
             tr("%1 is waiting on the island. Small steps today add up.").arg(name));

    int open = 0;
    for (const auto &habit : m_habits->habits()) {
        if (!m_habits->isScheduled(habit, now.date()) || !m_habits->status(habit.id, now.date()).isEmpty())
            continue;
        ++open;
        if (habit.reminder && due(QTime::fromString(habit.reminderTime, "HH:mm"), time))
            fire("habit:" + habit.id + ":" + day, habit.name,
                 habit.twoMinute.isEmpty() ? tr("Time for this habit.")
                                           : tr("Even the 2-minute version counts: %1").arg(habit.twoMinute));
    }
    if (m_profile->eveningReminder() && open > 0 && due(QTime(20, 0), time))
        fire("evening:" + day, tr("Habits still open"),
             tr("%1 reminds you: %2 habits left for today.").arg(name).arg(open));

    // Keep the memory of fired reminders to today only.
    m_fired.removeIf([&](const QString &key) { return !key.endsWith(day); });
}

void Reminders::sendTest() {
    m_sender(tr("Notifications are on"), tr("%1 will remind you here.").arg(m_profile->penguinName()));
}
