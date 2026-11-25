#pragma once

#include <QDateTime>
#include <QJsonObject>
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
    // Android only: Pengu's notifications are on, but the phone does not let
    // them through (permission denied, or switched off in its settings).
    Q_PROPERTY(bool blocked READ blocked NOTIFY blockedChanged)

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
    bool blocked() const { return m_blocked; }
    // Opens the phone's notification settings for Pengu.
    Q_INVOKABLE void openSystemSettings();
    // Plans again shortly, e.g. in a new language. Android only: the desktop
    // writes its texts when it sends them.
    void replan();

    // Today's sent reminders, kept in the save, so a restart inside a
    // reminder's window does not send it again.
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);

    // Every reminder from `now` through the next `days` days, in time order.
    // Today's habits that are already done or skipped are left out.
    QList<Planned> planned(const QDateTime &now, int days) const;

    static constexpr int WindowMinutes = 30;
    static constexpr int PlanDays = 7;

signals:
    void blockedChanged();
    void firedChanged();

private:
    bool due(const QTime &at, const QTime &now) const;
    void fire(const QString &key, const QString &title, const QString &body);
    // Today's reminders, whether or not their time has passed.
    QList<Planned> forDay(const QDate &day, const QDate &today) const;
    void schedulePlatform();
    void refreshBlocked();

    HabitBook *m_habits;
    Profile *m_profile;
    Sender m_sender;
    QSet<QString> m_fired;
    QTimer m_timer;
    QTimer m_replan;
    bool m_blocked = false;
    bool m_testPending = false;
};
