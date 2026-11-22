#pragma once

#include <QJsonObject>
#include <QObject>

// The player's progress (XP, level, sardines) and preferences. Habits and,
// later, other features (focus sessions) pay out through reward().
class Profile : public QObject {
    Q_OBJECT
    Q_PROPERTY(int fish READ fish NOTIFY progressChanged)
    Q_PROPERTY(int xp READ xp NOTIFY progressChanged)
    Q_PROPERTY(int level READ level NOTIFY progressChanged)
    Q_PROPERTY(int levelXp READ levelXp NOTIFY progressChanged)
    Q_PROPERTY(int levelXpNeeded READ levelXpNeeded NOTIFY progressChanged)
    Q_PROPERTY(QString userId READ userId NOTIFY progressChanged)
    Q_PROPERTY(QString penguinName READ penguinName WRITE setPenguinName NOTIFY settingsChanged)
    Q_PROPERTY(bool notifications READ notifications WRITE setNotifications NOTIFY settingsChanged)
    Q_PROPERTY(bool eveningReminder READ eveningReminder WRITE setEveningReminder NOTIFY settingsChanged)
    Q_PROPERTY(bool morningReminder READ morningReminder WRITE setMorningReminder NOTIFY settingsChanged)
    Q_PROPERTY(bool strictStreaks READ strictStreaks WRITE setStrictStreaks NOTIFY settingsChanged)
    Q_PROPERTY(bool weekStartsMonday READ weekStartsMonday WRITE setWeekStartsMonday NOTIFY settingsChanged)

public:
    explicit Profile(QObject *parent = nullptr);

    // XP needed to go from `level` to the next one: 10, 30, 50, 70, ...
    static int xpForLevel(int level) { return 20 * level - 10; }
    struct LevelInfo { int level, into, needed; };
    static LevelInfo levelFor(int xp);

    int fish() const { return m_fish; }
    int xp() const { return m_xp; }
    int level() const { return levelFor(m_xp).level; }
    int levelXp() const { return levelFor(m_xp).into; }
    int levelXpNeeded() const { return levelFor(m_xp).needed; }
    QString userId() const { return m_userId; }
    QString penguinName() const { return m_penguinName; }
    bool notifications() const { return m_notifications; }
    bool eveningReminder() const { return m_eveningReminder; }
    bool morningReminder() const { return m_morningReminder; }
    bool strictStreaks() const { return m_strictStreaks; }
    bool weekStartsMonday() const { return m_weekStartsMonday; }

    void setPenguinName(const QString &name);
    void setNotifications(bool on);
    void setEveningReminder(bool on);
    void setMorningReminder(bool on);
    void setStrictStreaks(bool on);
    void setWeekStartsMonday(bool on);

    // Positive to pay out, negative to take a payout back (undoing a habit).
    // Sardines may go below zero then, so undo can never be used to keep
    // sardines that were already spent.
    void reward(int fish, int xp);
    bool spend(int amount);
    Q_INVOKABLE void resetProgress();

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &object);

signals:
    void progressChanged();
    void settingsChanged();

private:
    int m_fish = 0;
    int m_xp = 0;
    QString m_userId;
    QString m_penguinName = "Pengu";
    bool m_notifications = false;
    bool m_eveningReminder = false;
    bool m_morningReminder = false;
    bool m_strictStreaks = false;
    bool m_weekStartsMonday = true;
};
