#pragma once

#include <QDate>
#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QVariantList>

class Profile;

// Habits, the day-by-day log of what was done or skipped, and everything
// derived from it: payouts, streaks, perfect days, stats and the heatmap.
class HabitBook : public QObject {
    Q_OBJECT
    // Bumped on every change; QML passes it into calls so bindings re-run.
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(QString viewDate READ viewDate NOTIFY changed)
    Q_PROPERTY(QString today READ todayIso NOTIFY changed)
    Q_PROPERTY(bool viewingToday READ viewingToday NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)

public:
    struct Habit {
        QString id, name, description, twoMinute, group;
        int difficulty = 2; // 0 very easy … 4 very hard
        int days = 0;       // Bit 0 = Monday … bit 6 = Sunday; 0 means every day.
        bool reminder = false;
        QString reminderTime = "09:00";
        QDate created;
    };
    struct Entry {
        QString status; // "done" or "skipped"
        int reward = 0; // What was paid out, so undo refunds exactly that.
    };

    explicit HabitBook(Profile *profile, QObject *parent = nullptr);

    static int rewardFor(int difficulty);
    static constexpr int MaxNameLength = 80;
    static constexpr int MaxTextLength = 500;

    int revision() const { return m_revision; }
    QString viewDate() const { return m_view.toString(Qt::ISODate); }
    QString todayIso() const { return today().toString(Qt::ISODate); }
    bool viewingToday() const { return m_view == today(); }
    int count() const { return m_habits.size(); }

    QDate today() const;
    void setToday(const QDate &date); // Tests pin the clock; empty date follows the system.
    void checkDayChange();            // Called by a timer so midnight moves "today".

    const QList<Habit> &habits() const { return m_habits; }
    const Habit *find(const QString &id) const;
    bool isScheduled(const Habit &habit, const QDate &date) const;
    QString status(const QString &id, const QDate &date) const;

    Q_INVOKABLE void shiftDay(int delta);
    Q_INVOKABLE void goToday();

    // Returns the new habit's id, or an empty string if the input is invalid.
    Q_INVOKABLE QString add(const QVariantMap &fields);
    Q_INVOKABLE bool update(const QString &id, const QVariantMap &fields);
    Q_INVOKABLE bool remove(const QString &id);
    // status: "done", "skipped" or "" (back to to-do). Pays out or refunds.
    Q_INVOKABLE bool setStatus(const QString &id, const QString &date, const QString &status);

    Q_INVOKABLE QVariantMap habit(const QString &id) const;
    // tab: "todo", "done", "skipped" — sections of the viewed day.
    Q_INVOKABLE QVariantList sections(const QString &tab, int revision = 0) const;
    Q_INVOKABLE QVariantMap progress(int revision = 0) const;
    Q_INVOKABLE QVariantMap stats(int revision = 0) const;
    Q_INVOKABLE QVariantList heatmap(int weeks, int revision = 0) const;
    Q_INVOKABLE QStringList groups(int revision = 0) const;
    Q_INVOKABLE int rewardForDifficulty(int difficulty) const { return rewardFor(difficulty); }

    // 1 perfect, 0 not perfect, -1 nothing was scheduled (neutral).
    int perfectDay(const QDate &date) const;
    int currentPerfectStreak() const;
    int longestPerfectStreak() const;
    int longestStreak(const Habit &habit) const;
    int doneCount(const QDate &date) const;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &object);

signals:
    void changed(); // Anything shown changed, including the day being viewed.
    void edited();  // Habits or the log changed: something to save and replan.

private:
    bool applyFields(Habit &habit, const QVariantMap &fields) const;
    QVariantMap view(const Habit &habit, const QDate &date) const;
    QDate firstDay() const;
    void touch();
    void markEdited();

    Profile *m_profile;
    QList<Habit> m_habits;
    QHash<QDate, QHash<QString, Entry>> m_log;
    QDate m_view;
    QDate m_todayOverride;
    QDate m_lastToday;
    int m_revision = 0;
};
