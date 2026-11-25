#include "updater.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibraryInfo>
#include <QNetworkReply>
#include <QSaveFile>
#include <QStandardPaths>
#include <QSysInfo>
#include <QUrl>
#include <QVersionNumber>

#include <memory>

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
    // Releases carry an x86_64 package only.
    m_supported = configured && QCoreApplication::applicationFilePath().startsWith("/usr/")
                  && QSysInfo::buildCpuArchitecture() == QLatin1String("x86_64")
                  && !QStandardPaths::findExecutable("pacman").isEmpty()
                  && !QStandardPaths::findExecutable("pkexec").isEmpty();
#endif
    if (!m_supported)
        return;
    // Left over from the last run: installed by now, or given up. A 33 MB APK
    // would also push the app data over Android's backup limit.
    QDir(downloadDirectory()).removeRecursively();

    m_timer.setInterval(CheckEveryMinutes * 60 * 1000);
    connect(&m_timer, &QTimer::timeout, this, &Updater::check);
    m_timer.start();
    // Usually the first activation below has checked already.
    QTimer::singleShot(3000, this, [this] {
        if (!m_lastCheck.isValid())
            check();
    });
    // Phones keep apps alive in the background for days; check on return.
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState s) {
        if (s != Qt::ApplicationActive)
            return;
#if defined(Q_OS_ANDROID)
        // Back from the system installer without updating: offer it again.
        // (On Linux pacman is still running when the password prompt closes.)
        if (m_state == "installing")
            setState("available");
#endif
        if (!m_lastCheck.isValid() || m_lastCheck.secsTo(QDateTime::currentDateTime()) > RecheckOnResumeMinutes * 60)
            check();
    });

    connect(&m_installer, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        if (status == QProcess::NormalExit && code == 0)
            setState("restart");
        // pkexec: 126 means the password prompt was cancelled, 127 that it
        // was refused or no polkit agent could ask.
        else if (status == QProcess::NormalExit && code == 126)
            setState("available");
        else
            setState("error", code == 127 ? tr("Pengu was not allowed to install the update.")
                                          : tr("Installing failed (pacman exit code %1).").arg(code));
    });
    connect(&m_installer, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart)
            setState("error", tr("Could not start the installer."));
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
    // Built against a newer Qt than this system has (Linux): it would not
    // start, so wait for the system upgrade.
    if (QVersionNumber::fromString(asset.value("qt").toString()) > QLibraryInfo::version())
        return true;
    if (build != m_latestBuild)
        m_readyFile.clear();
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
    // Re-checks while an update is offered run quietly.
    if (m_state != "available")
        setState("checking");

    QNetworkRequest request(QUrl("https://github.com/" + m_repo + "/releases/latest/download/version.json"));
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    request.setTransferTimeout(20000);
    m_reply = m_network.get(request);
    connect(m_reply, &QNetworkReply::finished, this, [this, reply = m_reply] {
        reply->deleteLater();
        QString error;
        if (reply->error() != QNetworkReply::NoError) {
            // No release yet, or offline: try again later.
            if (reply->error() != QNetworkReply::ContentNotFoundError)
                error = reply->errorString();
        } else if (!parseRelease(reply->readAll())) {
            error = tr("The release information could not be read.");
        } else {
            setState(m_latestBuild > m_build ? "available" : "current");
            return;
        }
        // A failed re-check keeps offering the update found before.
        if (m_latestBuild > m_build)
            setState("available");
        else
            setState("idle", error);
    });
}

QString Updater::downloadDirectory() const {
    // On Android this is the app's files directory, which Qt's FileProvider
    // shares with the system installer.
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/updates";
}

void Updater::install() {
    if (m_state != "available" && !(m_state == "error" && m_latestBuild > m_build))
        return;
    // "Update now" during a quiet re-check: drop the check.
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
    }
    // Back from the installer without updating: the file is still there.
    if (!m_readyFile.isEmpty() && QFileInfo::exists(m_readyFile))
        installFile(m_readyFile);
    else
        download();
}

void Updater::download() {
    m_progress = 0;
    QDir().mkpath(downloadDirectory());
    const QString path = downloadDirectory() + "/" + m_file;
    // Saved and hashed as it arrives, so the APK is never held in memory.
    auto file = std::make_shared<QSaveFile>(path);
    auto hash = std::make_shared<QCryptographicHash>(QCryptographicHash::Sha256);
    if (!file->open(QIODevice::WriteOnly)) {
        setState("error", tr("Could not save the update: %1").arg(file->errorString()));
        return;
    }
    setState("downloading");

    const QUrl url("https://github.com/" + m_repo + "/releases/download/v" + m_latestVersion + "/" + m_file);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    request.setTransferTimeout(60000);
    m_reply = m_network.get(request);
    const auto take = [reply = m_reply, file, hash] {
        const QByteArray chunk = reply->readAll();
        hash->addData(chunk);
        file->write(chunk);
    };
    connect(m_reply, &QNetworkReply::readyRead, this, take);
    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) {
            m_progress = double(received) / double(total);
            emit changed();
        }
    });
    connect(m_reply, &QNetworkReply::finished, this, [this, reply = m_reply, file, hash, take, path] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            setState("error", tr("Download failed: %1").arg(reply->errorString()));
            return;
        }
        take();
        // An unfinished QSaveFile is thrown away, never left half written.
        if (!m_sha256.isEmpty() && hash->result().toHex() != m_sha256.toLatin1()) {
            setState("error", tr("The download was damaged. Please try again."));
            return;
        }
        if (!file->commit()) {
            setState("error", tr("Could not save the update: %1").arg(file->errorString()));
            return;
        }
        m_readyFile = path;
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
