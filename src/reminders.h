#pragma once

#include <QDateTime>
#include <QObject>
#include <QSet>
#include <QTimer>
#include <functional>

class HabitBook;
class Profile;

// Desktop notifications while Pengu runs: a morning nudge, an evening
// reminder about open habits, and per-habit reminders at their time.
class Reminders : public QObject {
    Q_OBJECT

public:
    using Sender = std::function<void(const QString &title, const QString &body)>;

    Reminders(HabitBook *habits, Profile *profile, QObject *parent = nullptr);

    void setSender(Sender sender) { m_sender = std::move(sender); }
    // Fires whatever is due at `now`; each reminder at most once per day.
    void check(const QDateTime &now);
    Q_INVOKABLE void sendTest();

    static constexpr int WindowMinutes = 30;

private:
    bool due(const QTime &at, const QTime &now) const;
    void fire(const QString &key, const QString &title, const QString &body);

    HabitBook *m_habits;
    Profile *m_profile;
    Sender m_sender;
    QSet<QString> m_fired;
    QTimer m_timer;
};
