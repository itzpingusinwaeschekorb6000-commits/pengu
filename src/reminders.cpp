#include "reminders.h"

#include "habits.h"
#include "profile.h"

#include <QJsonArray>
#include <algorithm>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>

#ifdef Q_OS_ANDROID
#include <QGuiApplication>
#include <QJniObject>
#include <utility>
#endif

namespace {
const QTime Morning(8, 0);
const QTime Evening(20, 0);

#ifdef Q_OS_ANDROID
// The Java side lives in android/src/app/pengu/island/Reminders.java.
constexpr const char *JavaReminders = "app/pengu/island/Reminders";

// Calls a static method of Reminders.java; its first argument is always the
// app's context. The JNI signature is spelled out: Qt would otherwise derive
// it from the C++ argument types (java.lang.Object), and Java finds no such
// method.
template <typename Result = void, typename... Args>
Result callJava(const char *method, const char *signature, Args... args) {
    return QJniObject::callStaticMethod<Result>(JavaReminders, method, signature,
                                                QNativeInterface::QAndroidApplication::context().object(), args...);
}

// Whether the phone lets Pengu's notifications through.
bool allowed() {
    return callJava<jboolean>("allowed", "(Landroid/content/Context;)Z");
}
#endif
}

Reminders::Reminders(HabitBook *habits, Profile *profile, QObject *parent)
    : QObject(parent), m_habits(habits), m_profile(profile) {
#ifdef Q_OS_ANDROID
    m_sender = [](const QString &title, const QString &body) {
        callJava("show", "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)V",
                 QJniObject::fromString(title).object<jstring>(), QJniObject::fromString(body).object<jstring>());
    };
    // Habits and settings change in bursts (a form, a drag); plan once after.
    m_replan.setSingleShot(true);
    m_replan.setInterval(1500);
    connect(&m_replan, &QTimer::timeout, this, &Reminders::schedulePlatform);
    connect(m_habits, &HabitBook::edited, &m_replan, qOverload<>(&QTimer::start));
    connect(m_profile, &Profile::settingsChanged, &m_replan, qOverload<>(&QTimer::start));
    connect(m_profile, &Profile::settingsChanged, this, &Reminders::refreshBlocked);
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state != Qt::ApplicationActive) {
            // Timers stop in the background: hand a pending plan over now.
            if (m_replan.isActive()) {
                m_replan.stop();
                schedulePlatform();
            }
            return;
        }
        // Back from the permission dialog or the phone's settings.
        refreshBlocked();
        if (std::exchange(m_testPending, false) && m_profile->notifications() && !m_blocked)
            sendTest();
    });
    // Once a day the plan slides forward, even if nothing changed.
    m_timer.setInterval(6 * 60 * 60 * 1000);
    connect(&m_timer, &QTimer::timeout, this, &Reminders::schedulePlatform);
    m_timer.start();
    m_replan.start();
#else
    m_sender = [](const QString &title, const QString &body) {
        const QString notify = QStandardPaths::findExecutable("notify-send");
        if (!notify.isEmpty())
            // "--" ends the options, so a habit named "-5 kg" is no option.
            QProcess::startDetached(notify, {"--app-name=Pengu", "--icon=pengu", "--", title, body});
    };
    m_timer.setInterval(20 * 1000);
    connect(&m_timer, &QTimer::timeout, this, [this] { check(QDateTime::currentDateTime()); });
    m_timer.start();
#endif
}

bool Reminders::due(const QTime &at, const QTime &now) const {
    // A window rather than an exact minute, so a sleeping laptop or a busy
    // timer still delivers the reminder shortly after its time.
    // Late reminders (23:45) would wrap past midnight, so their window ends at
    // the end of the day instead.
    if (!at.isValid() || now < at)
        return false;
    const QTime end = at.addSecs(WindowMinutes * 60);
    return end < at || now < end;
}

void Reminders::fire(const QString &key, const QString &title, const QString &body) {
    if (m_fired.contains(key))
        return;
    m_fired.insert(key);
    emit firedChanged();
    m_sender(title, body);
}

QList<Reminders::Planned> Reminders::forDay(const QDate &day, const QDate &today) const {
    QList<Planned> list;
    if (!m_profile->notifications())
        return list;
    const QString date = day.toString(Qt::ISODate);
    const QString name = m_profile->penguinName();

    if (m_profile->morningReminder())
        list.append({QDateTime(day, Morning), "morning:" + date, tr("Good morning!"),
                     tr("%1 is waiting on the island. Small steps today add up.").arg(name)});

    int open = 0;
    for (const auto &habit : m_habits->habits()) {
        if (!m_habits->isScheduled(habit, day))
            continue;
        // Only today's progress is known; later days start with all open.
        if (day == today && !m_habits->status(habit.id, day).isEmpty())
            continue;
        ++open;
        const QTime at = QTime::fromString(habit.reminderTime, "HH:mm");
        if (habit.reminder && at.isValid())
            list.append({QDateTime(day, at), "habit:" + habit.id + ":" + date, habit.name,
                         habit.twoMinute.isEmpty() ? tr("Time for this habit.")
                                                   : tr("Even the 2-minute version counts: %1").arg(habit.twoMinute)});
    }
    if (m_profile->eveningReminder() && open > 0)
        list.append({QDateTime(day, Evening), "evening:" + date, tr("Habits still open"),
                     tr("%1 reminds you: %n habits left for today.", nullptr, open).arg(name)});
    return list;
}

QList<Reminders::Planned> Reminders::planned(const QDateTime &now, int days) const {
    QList<Planned> all;
    for (int i = 0; i < days; ++i)
        for (const auto &p : forDay(now.date().addDays(i), now.date()))
            if (p.at > now)
                all.append(p);
    std::stable_sort(all.begin(), all.end(), [](const Planned &a, const Planned &b) { return a.at < b.at; });
    return all;
}

void Reminders::check(const QDateTime &now) {
    const QString day = now.date().toString(Qt::ISODate);
    for (const auto &p : forDay(now.date(), now.date()))
        if (due(p.at.time(), now.time()))
            fire(p.key, p.title, p.body);
    // Keep the memory of fired reminders to today only.
    if (m_fired.removeIf([&](const QString &key) { return !key.endsWith(day); }))
        emit firedChanged();
}

QJsonObject Reminders::toJson() const {
    QStringList fired(m_fired.begin(), m_fired.end());
    fired.sort();
    return {{"fired", QJsonArray::fromStringList(fired)}};
}

void Reminders::fromJson(const QJsonObject &json) {
    m_fired.clear();
    for (const auto &key : json.value("fired").toArray())
        m_fired.insert(key.toString());
}

void Reminders::schedulePlatform() {
#ifdef Q_OS_ANDROID
    // No permission request here: this runs after every change, and the
    // dialog would pop up out of the blue. Settings shows `blocked` instead.
    QJsonArray items;
    for (const auto &p : planned(QDateTime::currentDateTime(), PlanDays))
        items.append(QJsonObject{{"at", p.at.toMSecsSinceEpoch()}, {"title", p.title}, {"body", p.body}});
    const QString json = QString::fromUtf8(QJsonDocument(items).toJson(QJsonDocument::Compact));
    callJava("schedule", "(Landroid/content/Context;Ljava/lang/String;)V", QJniObject::fromString(json).object<jstring>());
#endif
}

void Reminders::sendTest() {
#ifdef Q_OS_ANDROID
    // Android 13+ asks the user before an app may notify, and drops what is
    // sent before the answer: the test follows when Pengu is back in front.
    if (!allowed()) {
        m_testPending = true;
        callJava("requestPermission", "(Landroid/content/Context;)V");
        return;
    }
#endif
    m_sender(tr("Notifications are on"), tr("%1 will remind you here.").arg(m_profile->penguinName()));
}

void Reminders::refreshBlocked() {
#ifdef Q_OS_ANDROID
    const bool blocked = m_profile->notifications() && !allowed();
    if (blocked != m_blocked) {
        m_blocked = blocked;
        emit blockedChanged();
    }
#endif
}

void Reminders::openSystemSettings() {
#ifdef Q_OS_ANDROID
    callJava("openSettings", "(Landroid/content/Context;)V");
#endif
}

void Reminders::replan() {
#ifdef Q_OS_ANDROID
    m_replan.start();
#endif
}
