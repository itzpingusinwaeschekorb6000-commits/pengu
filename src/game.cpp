#include "game.h"

#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

Game::Game(const QString &savePath, QObject *parent)
    : QObject(parent),
      m_path(savePath.isEmpty() ? defaultSavePath() : savePath),
      m_habits(&m_profile),
      m_island(&m_catalog, &m_profile),
      m_reminders(&m_habits, &m_profile) {
    // Changes are batched into one write shortly after they stop.
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(400);
    connect(&m_saveTimer, &QTimer::timeout, this, &Game::save);
    for (auto signal : {&Profile::progressChanged, &Profile::settingsChanged})
        connect(&m_profile, signal, this, &Game::scheduleSave);
    connect(&m_habits, &HabitBook::changed, this, &Game::scheduleSave);
    connect(&m_island, &Island::changed, this, &Game::scheduleSave);

    // Moves "today" forward at midnight while the app stays open.
    m_clock.setInterval(30 * 1000);
    connect(&m_clock, &QTimer::timeout, &m_habits, &HabitBook::checkDayChange);
    m_clock.start();
}

Game::~Game() {
    if (m_saveTimer.isActive())
        save();
}

QString Game::defaultSavePath() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/pengu/pengu.json";
}

void Game::scheduleSave() {
    if (!m_loading)
        m_saveTimer.start();
}

bool Game::migrate(QJsonObject &root) {
    const int version = root.value("version").toInt(0);
    if (version < 1 || version > SchemaVersion)
        return false;
    // Future schema changes go here, one step at a time:
    // if (version < 2) { ...; root["version"] = 2; }
    return true;
}

void Game::load() {
    m_loading = true;
    QFile file(m_path);
    bool fresh = !file.exists();
    if (!fresh) {
        QJsonParseError parse;
        QJsonObject root;
        if (file.open(QIODevice::ReadOnly))
            root = QJsonDocument::fromJson(file.readAll(), &parse).object();
        file.close();
        const int version = root.value("version").toInt(0);
        if (version > SchemaVersion) {
            // Written by a newer Pengu: never overwrite it with less.
            m_readOnly = true;
            m_loadError = tr("This save was made by a newer version of Pengu. Changes will not be saved.");
            m_profile.fromJson(root.value("profile").toObject());
            m_habits.fromJson(root.value("habits").toObject());
            m_island.fromJson(root.value("island").toObject());
        } else if (root.isEmpty() || !migrate(root)) {
            // Keep the broken file next to the new one instead of losing it.
            const QString backup = m_path + ".broken-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
            QFile::rename(m_path, backup);
            m_loadError = tr("The save file could not be read and was set aside as %1. A new island was started.")
                              .arg(QFileInfo(backup).fileName());
            fresh = true;
        } else {
            m_profile.fromJson(root.value("profile").toObject());
            m_habits.fromJson(root.value("habits").toObject());
            m_island.fromJson(root.value("island").toObject());
        }
    }
    if (fresh) {
        m_island.seed();
        m_profile.reward(10, 0); // A few sardines to look around the shop.
    }
    m_loading = false;
    emit loadErrorChanged();
    if (fresh)
        save();
}

bool Game::save() {
    m_saveTimer.stop();
    if (m_readOnly)
        return false;
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    const QJsonObject root{{"version", SchemaVersion},
                           {"app", "pengu"},
                           {"profile", m_profile.toJson()},
                           {"habits", m_habits.toJson()},
                           {"island", m_island.toJson()}};
    // Written beside the target and renamed over it, so a crash mid-write
    // never leaves a half-written save behind.
    QSaveFile file(m_path);
    QString error;
    if (!file.open(QIODevice::WriteOnly))
        error = file.errorString();
    else if (file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0 || !file.commit())
        error = file.errorString();
    const QString message = error.isEmpty() ? QString() : tr("Could not save: %1").arg(error);
    if (message != m_saveError) {
        m_saveError = message;
        emit saveErrorChanged();
    }
    return error.isEmpty();
}

void Game::copyText(const QString &text) {
    if (auto *clipboard = QGuiApplication::clipboard())
        clipboard->setText(text);
}
