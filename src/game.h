#pragma once

#include <QObject>
#include <QTimer>

#include "catalog.h"
#include "habits.h"
#include "island.h"
#include "profile.h"
#include "reminders.h"

// Owns every part of the game and keeps them on disk. Each part saves itself
// under its own key, so a new feature adds a key without touching the others.
class Game : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString loadError READ loadError NOTIFY loadErrorChanged)
    Q_PROPERTY(QString saveError READ saveError NOTIFY saveErrorChanged)
    Q_PROPERTY(QString savePath READ savePath CONSTANT)

public:
    static constexpr int SchemaVersion = 1;

    explicit Game(const QString &savePath = {}, QObject *parent = nullptr);
    ~Game() override;

    Catalog *catalog() { return &m_catalog; }
    Profile *profile() { return &m_profile; }
    HabitBook *habits() { return &m_habits; }
    Island *island() { return &m_island; }
    Reminders *reminders() { return &m_reminders; }

    QString loadError() const { return m_loadError; }
    QString saveError() const { return m_saveError; }
    QString savePath() const { return m_path; }

    void load();
    Q_INVOKABLE bool save();
    Q_INVOKABLE void copyText(const QString &text);

    static QString defaultSavePath();
    // Upgrades an older save in place; returns false if it cannot be read.
    static bool migrate(QJsonObject &root);

signals:
    void loadErrorChanged();
    void saveErrorChanged();

private:
    void scheduleSave();

    QString m_path;
    Catalog m_catalog;
    Profile m_profile;
    HabitBook m_habits;
    Island m_island;
    Reminders m_reminders;
    QTimer m_saveTimer, m_clock;
    QString m_loadError, m_saveError;
    bool m_readOnly = false;
    bool m_loading = false;
};
