#pragma once

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QTimer>

class QNetworkReply;

// Finds newer builds on the project's GitHub releases and installs them:
// on Android it downloads the APK and hands it to the system installer, on
// Linux it downloads the Arch package and installs it with pkexec pacman.
//
// Every release carries a version.json next to its files:
//   { "build": 42, "version": "0.2.42", "notes": "...",
//     "assets": { "android": { "file": "pengu.apk", "sha256": "..." },
//                 "linux":   { "file": "pengu-x86_64.pkg.tar.zst", "sha256": "..." } } }
class Updater : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(int build READ build CONSTANT)
    Q_PROPERTY(bool supported READ supported CONSTANT)
    Q_PROPERTY(QString state READ state NOTIFY changed)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY changed)
    Q_PROPERTY(QString notes READ notes NOTIFY changed)
    Q_PROPERTY(double progress READ progress NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)

public:
    // States: idle, checking, current, available, downloading, installing,
    // restart (Linux: installed, restart to use it), error.
    explicit Updater(QObject *parent = nullptr, const QString &repo = QStringLiteral(PENGU_REPO),
                     int build = PENGU_BUILD);

    QString version() const { return QStringLiteral(PENGU_VERSION); }
    int build() const { return m_build; }
    bool supported() const { return m_supported; }
    QString state() const { return m_state; }
    QString latestVersion() const { return m_latestVersion; }
    QString notes() const { return m_notes; }
    double progress() const { return m_progress; }
    QString error() const { return m_error; }

    Q_INVOKABLE void check();
    Q_INVOKABLE void install();
    Q_INVOKABLE void restart();

    // Puts the updater into a state without going online, for the
    // screenshot harness.
    void preview(const QString &state, const QString &latestVersion, double progress = 0);

    // Which entry of version.json's "assets" this build installs.
    static QString platformKey();
    // Parses version.json; false when it is unusable or has no file for us.
    bool parseRelease(const QByteArray &json);

signals:
    void changed();

private:
    void setState(const QString &state, const QString &error = {});
    void download();
    void installFile(const QString &path);
    QString downloadDirectory() const;

    QString m_repo;
    int m_build;
    bool m_supported = false;
    QString m_state = "idle";
    QString m_latestVersion, m_notes, m_error, m_file, m_sha256;
    int m_latestBuild = 0;
    double m_progress = 0;
    QDateTime m_lastCheck;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QProcess m_installer;
    QTimer m_timer;
};
