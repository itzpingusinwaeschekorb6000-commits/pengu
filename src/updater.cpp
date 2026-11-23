#include "updater.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>

namespace {
constexpr int CheckEveryMinutes = 6 * 60;
// Coming back to the app checks again, but not more often than this.
constexpr int RecheckOnResumeMinutes = 30;
}

Updater::Updater(QObject *parent, const QString &repo, int build)
    : QObject(parent), m_repo(repo), m_build(build) {
    const bool configured = m_repo.contains('/') && !m_repo.startsWith("OWNER/");
#if defined(Q_OS_ANDROID)
    m_supported = configured;
#elif defined(Q_OS_LINUX)
    // Only the installed package updates itself; a build from the source tree
    // is updated with git, and pacman must be there to install the package.
    m_supported = configured && QCoreApplication::applicationFilePath().startsWith("/usr/")
                  && !QStandardPaths::findExecutable("pacman").isEmpty()
                  && !QStandardPaths::findExecutable("pkexec").isEmpty();
#endif
    if (!m_supported)
        return;

    m_timer.setInterval(CheckEveryMinutes * 60 * 1000);
    connect(&m_timer, &QTimer::timeout, this, &Updater::check);
    m_timer.start();
    QTimer::singleShot(3000, this, &Updater::check);
    // Phones keep apps alive in the background for days; check on return.
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState s) {
        if (s != Qt::ApplicationActive)
            return;
        // Back from the system installer without updating: offer it again.
        if (m_state == "installing")
            setState("available");
        if (!m_lastCheck.isValid() || m_lastCheck.secsTo(QDateTime::currentDateTime()) > RecheckOnResumeMinutes * 60)
            check();
    });

    connect(&m_installer, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        if (status == QProcess::NormalExit && code == 0)
            setState("restart");
        else
            // 126/127: the password prompt was cancelled.
            setState("available", code == 126 || code == 127 ? QString()
                                                             : tr("Installing failed (pacman exit code %1).").arg(code));
    });
    connect(&m_installer, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart)
            setState("available", tr("Could not start the installer."));
    });
}

QString Updater::platformKey() {
#if defined(Q_OS_ANDROID)
    return "android";
#else
    return "linux";
#endif
}

void Updater::setState(const QString &state, const QString &error) {
    m_state = state;
    m_error = error;
    emit changed();
}

void Updater::preview(const QString &state, const QString &latestVersion, double progress) {
    m_latestVersion = latestVersion;
    m_latestBuild = m_build + 1;
    m_progress = progress;
    setState(state, state == "error" ? tr("Download failed: %1").arg("Host not found") : QString());
}

bool Updater::parseRelease(const QByteArray &json) {
    const QJsonObject root = QJsonDocument::fromJson(json).object();
    const QJsonObject asset = root.value("assets").toObject().value(platformKey()).toObject();
    const int build = root.value("build").toInt(0);
    const QString file = asset.value("file").toString();
    // A plain file name only: it is appended to the release's download URL.
    if (build <= 0 || file.isEmpty() || file.contains('/') || file.startsWith('.'))
        return false;
    m_latestBuild = build;
    m_latestVersion = root.value("version").toString(QString::number(build));
    m_notes = root.value("notes").toString();
    m_file = file;
    m_sha256 = asset.value("sha256").toString().toLower();
    return true;
}

void Updater::check() {
    if (!m_supported || m_reply || m_state == "downloading" || m_state == "installing" || m_state == "restart")
        return;
    m_lastCheck = QDateTime::currentDateTime();
    setState("checking");

    QNetworkRequest request(QUrl("https://github.com/" + m_repo + "/releases/latest/download/version.json"));
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    request.setTransferTimeout(20000);
    m_reply = m_network.get(request);
    connect(m_reply, &QNetworkReply::finished, this, [this, reply = m_reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            // No release yet, or offline: nothing to offer, try again later.
            setState("idle", reply->error() == QNetworkReply::ContentNotFoundError ? QString() : reply->errorString());
            return;
        }
        if (!parseRelease(reply->readAll())) {
            setState("idle", tr("The release information could not be read."));
            return;
        }
        setState(m_latestBuild > m_build ? "available" : "current");
    });
}

QString Updater::downloadDirectory() const {
    // On Android this is the app's files directory, which Qt's FileProvider
    // shares with the system installer.
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/updates";
}

void Updater::install() {
    if (m_state == "available" || (m_state == "error" && m_latestBuild > m_build))
        download();
}

void Updater::download() {
    if (m_reply)
        return;
    m_progress = 0;
    setState("downloading");

    const QUrl url("https://github.com/" + m_repo + "/releases/download/v" + m_latestVersion + "/" + m_file);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    request.setTransferTimeout(60000);
    m_reply = m_network.get(request);
    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) {
            m_progress = double(received) / double(total);
            emit changed();
        }
    });
    connect(m_reply, &QNetworkReply::finished, this, [this, reply = m_reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            setState("error", tr("Download failed: %1").arg(reply->errorString()));
            return;
        }
        const QByteArray data = reply->readAll();
        if (!m_sha256.isEmpty()
            && QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex() != m_sha256.toLatin1()) {
            setState("error", tr("The download was damaged. Please try again."));
            return;
        }
        QDir().mkpath(downloadDirectory());
        const QString path = downloadDirectory() + "/" + m_file;
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
            setState("error", tr("Could not save the update: %1").arg(file.errorString()));
            return;
        }
        installFile(path);
    });
}

void Updater::installFile(const QString &path) {
    m_progress = 1;
    setState("installing");
#if defined(Q_OS_ANDROID)
    // Qt shares the file through its FileProvider and opens it with the
    // system package installer, which asks the user to confirm the update
    // (and, the first time, to allow installing from Pengu).
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path)))
        setState("error", tr("Android did not open the installer."));
#else
    m_installer.start(QStandardPaths::findExecutable("pkexec"),
                      {QStandardPaths::findExecutable("pacman"), "-U", "--noconfirm", path});
#endif
}

void Updater::restart() {
#if !defined(Q_OS_ANDROID)
    QProcess::startDetached(QCoreApplication::applicationFilePath(), {});
    QCoreApplication::quit();
#endif
}
