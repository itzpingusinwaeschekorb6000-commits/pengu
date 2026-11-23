#pragma once

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QSet>
#include <QTimer>
#include <functional>

class HabitBook;
class Profile;

// Reminders: a morning nudge, an evening reminder about open habits, and
// per-habit reminders at their time.
//
// On the desktop they are sent by a timer while Pengu runs. Android stops
// apps in the background, so there Pengu hands the reminders of the coming
// days to the system's alarm service instead, and re-plans whenever habits
// or settings change. Both use the same rules (planned()).
class Reminders : public QObject {
    Q_OBJECT

public:
    using Sender = std::function<void(const QString &title, const QString &body)>;

    struct Planned {
        QDateTime at;
        QString key;
        QString title;
        QString body;
    };

    Reminders(HabitBook *habits, Profile *profile, QObject *parent = nullptr);

    void setSender(Sender sender) { m_sender = std::move(sender); }
    // Fires whatever is due at `now`; each reminder at most once per day.
    void check(const QDateTime &now);
    Q_INVOKABLE void sendTest();

    // Every reminder from `now` through the next `days` days, in time order.
    // Today's habits that are already done or skipped are left out.
    QList<Planned> planned(const QDateTime &now, int days) const;

    static constexpr int WindowMinutes = 30;
    static constexpr int PlanDays = 7;

private:
    bool due(const QTime &at, const QTime &now) const;
    void fire(const QString &key, const QString &title, const QString &body);
    // Today's reminders, whether or not their time has passed.
    QList<Planned> forDay(const QDate &day, const QDate &today) const;
    void schedulePlatform();

    HabitBook *m_habits;
    Profile *m_profile;
    Sender m_sender;
    QSet<QString> m_fired;
    QTimer m_timer;
    QTimer m_replan;
};
