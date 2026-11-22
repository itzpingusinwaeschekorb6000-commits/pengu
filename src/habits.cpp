#include "habits.h"

#include "profile.h"

#include <QJsonArray>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>
#include <algorithm>

namespace {
// Weekday bit for a date, Monday = bit 0.
int dayBit(const QDate &date) { return 1 << (date.dayOfWeek() - 1); }

QString clip(const QString &text, int length) { return text.trimmed().left(length); }

bool validTime(const QString &time) {
    static const QRegularExpression pattern("^([01]\\d|2[0-3]):[0-5]\\d$");
    return pattern.match(time).hasMatch();
}
} // namespace

HabitBook::HabitBook(Profile *profile, QObject *parent) : QObject(parent), m_profile(profile) {
    m_view = today();
    m_lastToday = today();
    // Streaks and week dots depend on these preferences.
    connect(m_profile, &Profile::settingsChanged, this, [this] {
        ++m_revision;
        emit changed();
    });
}

int HabitBook::rewardFor(int difficulty) {
    static const int rewards[] = {2, 5, 10, 20, 35};
    return rewards[std::clamp(difficulty, 0, 4)];
}

QDate HabitBook::today() const { return m_todayOverride.isValid() ? m_todayOverride : QDate::currentDate(); }

void HabitBook::setToday(const QDate &date) {
    m_todayOverride = date;
    checkDayChange();
}

void HabitBook::checkDayChange() {
    const QDate now = today();
    if (now == m_lastToday)
        return;
    // Someone looking at "today" keeps looking at today after midnight.
    if (m_view == m_lastToday || m_view > now)
        m_view = now;
    m_lastToday = now;
    touch();
}

void HabitBook::touch() {
    ++m_revision;
    emit changed();
}

const HabitBook::Habit *HabitBook::find(const QString &id) const {
    for (const auto &habit : m_habits)
        if (habit.id == id)
            return &habit;
    return nullptr;
}

bool HabitBook::isScheduled(const Habit &habit, const QDate &date) const {
    if (!date.isValid() || date < habit.created)
        return false;
    return habit.days == 0 || (habit.days & dayBit(date));
}

QString HabitBook::status(const QString &id, const QDate &date) const {
    return m_log.value(date).value(id).status;
}

void HabitBook::shiftDay(int delta) {
    const QDate target = m_view.addDays(delta);
    // The future cannot be ticked off, and there is nothing before the first habit.
    if (target > today() || (delta < 0 && target < firstDay().addDays(-365)))
        return;
    m_view = target;
    touch();
}

void HabitBook::goToday() {
    m_view = today();
    touch();
}

bool HabitBook::applyFields(Habit &habit, const QVariantMap &fields) const {
    if (fields.contains("name"))
        habit.name = clip(fields.value("name").toString(), MaxNameLength);
    if (habit.name.isEmpty())
        return false;
    if (fields.contains("description"))
        habit.description = clip(fields.value("description").toString(), MaxTextLength);
    if (fields.contains("twoMinute"))
        habit.twoMinute = clip(fields.value("twoMinute").toString(), MaxTextLength);
    if (fields.contains("group"))
        habit.group = clip(fields.value("group").toString(), 40);
    if (fields.contains("difficulty"))
        habit.difficulty = std::clamp(fields.value("difficulty").toInt(), 0, 4);
    if (fields.contains("days"))
        habit.days = fields.value("days").toInt() & 0x7f;
    if (fields.contains("reminder"))
        habit.reminder = fields.value("reminder").toBool();
    if (fields.contains("reminderTime")) {
        const QString time = fields.value("reminderTime").toString().trimmed();
        if (!validTime(time))
            return false;
        habit.reminderTime = time;
    }
    return true;
}

QString HabitBook::add(const QVariantMap &fields) {
    Habit habit;
    if (!applyFields(habit, fields))
        return {};
    habit.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    // A new habit starts on the day being viewed if that is in the past,
    // so a habit can be back-filled from the day it was really started.
    habit.created = std::min(m_view, today());
    m_habits << habit;
    touch();
    return habit.id;
}

bool HabitBook::update(const QString &id, const QVariantMap &fields) {
    for (auto &habit : m_habits) {
        if (habit.id != id)
            continue;
        Habit copy = habit;
        if (!applyFields(copy, fields))
            return false;
        habit = copy;
        touch();
        return true;
    }
    return false;
}

bool HabitBook::remove(const QString &id) {
    const auto before = m_habits.size();
    m_habits.removeIf([&](const Habit &h) { return h.id == id; });
    if (m_habits.size() == before)
        return false;
    // The log keeps its entries: what was done stays counted in the stats,
    // and the sardines already earned are not taken away.
    touch();
    return true;
}

bool HabitBook::setStatus(const QString &id, const QString &dateText, const QString &status) {
    const Habit *habit = find(id);
    const QDate date = QDate::fromString(dateText, Qt::ISODate);
    if (!habit || !date.isValid() || date > today() || date < habit->created)
        return false;
    if (status != "done" && status != "skipped" && !status.isEmpty())
        return false;

    auto &day = m_log[date];
    const Entry old = day.value(id);
    if (old.status == status) {
        if (day.isEmpty())
            m_log.remove(date);
        return true; // Ticking twice must never pay twice.
    }
    if (old.status == "done")
        m_profile->reward(-old.reward, -old.reward);

    if (status.isEmpty()) {
        day.remove(id);
        if (day.isEmpty())
            m_log.remove(date);
    } else {
        Entry entry{status, 0};
        if (status == "done") {
            entry.reward = rewardFor(habit->difficulty);
            m_profile->reward(entry.reward, entry.reward);
        }
        day.insert(id, entry);
    }
    touch();
    return true;
}

QVariantMap HabitBook::view(const Habit &habit, const QDate &date) const {
    // Seven dots for the week that holds `date`, starting on the chosen weekday.
    const int startOffset = m_profile->weekStartsMonday() ? date.dayOfWeek() - 1 : date.dayOfWeek() % 7;
    const QDate start = date.addDays(-startOffset);
    QVariantList week;
    for (int i = 0; i < 7; ++i) {
        const QDate d = start.addDays(i);
        const QString s = status(habit.id, d);
        // 3 done, 2 skipped, 1 scheduled but open/missed, 0 not scheduled or future.
        week << (s == "done" ? 3 : s == "skipped" ? 2 : (d <= today() && isScheduled(habit, d)) ? 1 : 0);
    }
    return {{"id", habit.id},
            {"name", habit.name},
            {"description", habit.description},
            {"twoMinute", habit.twoMinute},
            {"group", habit.group},
            {"difficulty", habit.difficulty},
            {"reward", rewardFor(habit.difficulty)},
            {"days", habit.days},
            {"reminder", habit.reminder},
            {"reminderTime", habit.reminderTime},
            {"status", status(habit.id, date)},
            {"scheduled", isScheduled(habit, date)},
            {"week", week}};
}

QVariantMap HabitBook::habit(const QString &id) const {
    const Habit *h = find(id);
    return h ? view(*h, m_view) : QVariantMap{};
}

QVariantList HabitBook::sections(const QString &tab, int) const {
    QVariantList result;
    if (tab == "done" || tab == "skipped") {
        QVariantList list;
        for (const auto &h : m_habits)
            if (status(h.id, m_view) == tab)
                list << view(h, m_view);
        result << QVariantMap{{"key", tab}, {"title", QString()}, {"habits", list}};
        return result;
    }

    // To-dos: scheduled habits by group ("" = ungrouped first, then A–Z),
    // then everything that exists but is not scheduled on this weekday.
    QMap<QString, QVariantList> grouped;
    QVariantList unscheduled;
    grouped[QString()]; // The ungrouped section is always shown, like in Monke.
    for (const auto &h : m_habits) {
        if (h.created > m_view || !status(h.id, m_view).isEmpty())
            continue;
        if (isScheduled(h, m_view))
            grouped[h.group] << view(h, m_view);
        else
            unscheduled << view(h, m_view);
    }
    for (auto it = grouped.cbegin(); it != grouped.cend(); ++it)
        result << QVariantMap{{"key", "g:" + it.key()}, {"title", it.key()}, {"habits", it.value()}};
    result << QVariantMap{{"key", "unscheduled"}, {"title", QString()}, {"habits", unscheduled}};
    return result;
}

int HabitBook::doneCount(const QDate &date) const {
    int n = 0;
    for (const auto &entry : m_log.value(date))
        n += entry.status == "done";
    return n;
}

QVariantMap HabitBook::progress(int) const {
    const QDate now = today();
    int scheduled = 0, done = 0, fish = 0;
    for (const auto &h : m_habits) {
        if (!isScheduled(h, now))
            continue;
        ++scheduled;
        done += status(h.id, now) == "done";
    }
    for (const auto &entry : m_log.value(now))
        fish += entry.reward;
    return {{"done", done}, {"scheduled", scheduled}, {"fish", fish}, {"streak", currentPerfectStreak()}};
}

int HabitBook::perfectDay(const QDate &date) const {
    int scheduled = 0, done = 0;
    for (const auto &h : m_habits) {
        if (!isScheduled(h, date))
            continue;
        ++scheduled;
        const QString s = status(h.id, date);
        if (s.isEmpty())
            return 0;
        done += s == "done";
    }
    if (scheduled == 0)
        return -1;
    // Skipping everything is not a perfect day; at least one has to be done.
    return done > 0 ? 1 : 0;
}

QDate HabitBook::firstDay() const {
    QDate first = today();
    for (const auto &h : m_habits)
        if (h.created.isValid())
            first = std::min(first, h.created);
    for (auto it = m_log.cbegin(); it != m_log.cend(); ++it)
        first = std::min(first, it.key());
    return first;
}

int HabitBook::currentPerfectStreak() const {
    const QDate first = firstDay();
    int streak = 0;
    for (QDate d = today(); d >= first; d = d.addDays(-1)) {
        const int p = perfectDay(d);
        if (p == 1)
            ++streak;
        else if (p == 0 && d != today()) // Today is still in progress.
            break;
    }
    return streak;
}

int HabitBook::longestPerfectStreak() const {
    int best = 0, run = 0;
    for (QDate d = firstDay(); d <= today(); d = d.addDays(1)) {
        const int p = perfectDay(d);
        if (p == 1)
            best = std::max(best, ++run);
        else if (p == 0 && d != today())
            run = 0;
    }
    return best;
}

int HabitBook::longestStreak(const Habit &habit) const {
    // Skipped days pause a streak. A missed day ends it in strict mode;
    // otherwise one missed day in a row is forgiven as a grace period.
    const bool strict = m_profile->strictStreaks();
    int best = 0, run = 0;
    bool missedLast = false;
    for (QDate d = habit.created; d.isValid() && d <= today(); d = d.addDays(1)) {
        if (!isScheduled(habit, d))
            continue;
        const QString s = status(habit.id, d);
        if (s == "done") {
            best = std::max(best, ++run);
            missedLast = false;
        } else if (s.isEmpty() && d != today()) {
            if (strict || missedLast)
                run = 0;
            missedLast = true;
        }
    }
    // Done entries of days the habit is no longer scheduled on still count.
    return best;
}

QVariantMap HabitBook::stats(int) const {
    int total = 0, most = 0, longest = 0;
    for (auto it = m_log.cbegin(); it != m_log.cend(); ++it) {
        const int n = doneCount(it.key());
        total += n;
        most = std::max(most, n);
    }
    for (const auto &h : m_habits)
        longest = std::max(longest, longestStreak(h));
    return {{"totalCompleted", total},
            {"mostInOneDay", most},
            {"longestHabitStreak", longest},
            {"longestPerfectStreak", longestPerfectStreak()},
            {"perfectStreak", currentPerfectStreak()}};
}

QVariantList HabitBook::heatmap(int weeks, int) const {
    weeks = std::clamp(weeks, 1, 104);
    const QDate now = today();
    const int offset = m_profile->weekStartsMonday() ? now.dayOfWeek() - 1 : now.dayOfWeek() % 7;
    const QDate start = now.addDays(-offset - 7 * (weeks - 1));
    QVariantList cells; // Column by column (one week per column), oldest first.
    for (int i = 0; i < weeks * 7; ++i) {
        const QDate d = start.addDays(i);
        const bool future = d > now;
        cells << QVariantMap{{"date", d.toString(Qt::ISODate)},
                             {"count", future ? 0 : doneCount(d)},
                             {"perfect", !future && perfectDay(d) == 1},
                             {"future", future}};
    }
    return cells;
}

QStringList HabitBook::groups(int) const {
    QSet<QString> set;
    for (const auto &h : m_habits)
        if (!h.group.isEmpty())
            set.insert(h.group);
    QStringList list(set.begin(), set.end());
    list.sort(Qt::CaseInsensitive);
    return list;
}

QJsonObject HabitBook::toJson() const {
    QJsonArray habits;
    for (const auto &h : m_habits)
        habits << QJsonObject{{"id", h.id},
                              {"name", h.name},
                              {"description", h.description},
                              {"twoMinute", h.twoMinute},
                              {"group", h.group},
                              {"difficulty", h.difficulty},
                              {"days", h.days},
                              {"reminder", h.reminder},
                              {"reminderTime", h.reminderTime},
                              {"created", h.created.toString(Qt::ISODate)}};
    QJsonObject log;
    for (auto it = m_log.cbegin(); it != m_log.cend(); ++it) {
        QJsonObject day;
        for (auto e = it.value().cbegin(); e != it.value().cend(); ++e)
            day.insert(e.key(), QJsonObject{{"status", e.value().status}, {"reward", e.value().reward}});
        log.insert(it.key().toString(Qt::ISODate), day);
    }
    return {{"habits", habits}, {"log", log}};
}

void HabitBook::fromJson(const QJsonObject &object) {
    m_habits.clear();
    m_log.clear();
    QSet<QString> ids;
    for (const auto &value : object.value("habits").toArray()) {
        const QJsonObject o = value.toObject();
        Habit h;
        h.id = o.value("id").toString();
        if (h.id.isEmpty() || ids.contains(h.id))
            continue;
        // A broken reminder time must not cost the whole habit.
        QVariantMap fields = o.toVariantMap();
        if (!validTime(fields.value("reminderTime").toString()))
            fields.remove("reminderTime");
        if (!applyFields(h, fields))
            continue;
        h.created = QDate::fromString(o.value("created").toString(), Qt::ISODate);
        if (!h.created.isValid())
            h.created = today();
        ids.insert(h.id);
        m_habits << h;
    }
    const QJsonObject log = object.value("log").toObject();
    for (auto it = log.begin(); it != log.end(); ++it) {
        const QDate date = QDate::fromString(it.key(), Qt::ISODate);
        if (!date.isValid())
            continue;
        const QJsonObject day = it.value().toObject();
        for (auto e = day.begin(); e != day.end(); ++e) {
            const QJsonObject entry = e.value().toObject();
            const QString status = entry.value("status").toString();
            if (status != "done" && status != "skipped")
                continue;
            m_log[date].insert(e.key(), {status, std::max(0, entry.value("reward").toInt())});
        }
    }
    m_view = today();
    m_lastToday = today();
    touch();
}
