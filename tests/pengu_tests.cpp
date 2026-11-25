// Tests that try to break Pengu's logic: double payouts, time travel, broken
// save files, impossible placements and the like.

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>
#include <QTemporaryDir>
#include <QtTest>

#include "catalog.h"
#include "game.h"
#include "habits.h"
#include "island.h"
#include "profile.h"
#include "reminders.h"
#include "theme.h"
#include "translator.h"
#include "updater.h"

class PenguTests : public QObject {
    Q_OBJECT

    // Monday 2026-11-16 … Sunday 2026-11-22; "today" is Wednesday the 18th.
    // Plain members: moc cannot parse brace-initialized ones.
    const QDate monday = QDate(2026, 11, 16);
    const QDate today = QDate(2026, 11, 18);

    static QString iso(const QDate &d) { return d.toString(Qt::ISODate); }

    static void write(const QString &path, const QByteArray &contents) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(contents);
    }

private slots:
    // ---------------------------------------------------------------- profile

    void levelCurveMatchesTheOriginal() {
        // Monke's settings showed "Level 4 • 134 XP" and the stats "44/70".
        const auto info = Profile::levelFor(134);
        QCOMPARE(info.level, 4);
        QCOMPARE(info.into, 44);
        QCOMPARE(info.needed, 70);
        QCOMPARE(Profile::levelFor(0).level, 1);
        QCOMPARE(Profile::levelFor(9).level, 1);
        QCOMPARE(Profile::levelFor(10).level, 2);
        QCOMPARE(Profile::levelFor(-50).level, 1);
        // A corrupt save with an absurd number must not hang.
        QVERIFY(Profile::levelFor(INT_MAX).level <= 10000);
    }

    void spendingIsChecked() {
        Profile p;
        p.reward(30, 0);
        QVERIFY(!p.spend(31));
        QVERIFY(!p.spend(-5));
        QCOMPARE(p.fish(), 30);
        QVERIFY(p.spend(30));
        QCOMPARE(p.fish(), 0);
        p.reward(0, -100);
        QCOMPARE(p.xp(), 0);
    }

    void profileSurvivesGarbage() {
        Profile p;
        p.fromJson(QJsonObject{{"xp", -40}, {"penguinName", "   "}, {"weekStartsMonday", "yes"}});
        QCOMPARE(p.xp(), 0);
        QCOMPARE(p.penguinName(), QString("Pengu"));
        QVERIFY(p.weekStartsMonday()); // Wrong type falls back to the default.
        QVERIFY(!p.userId().isEmpty());
    }

    // ----------------------------------------------------------------- habits

    void invalidHabitsAreRejected() {
        Profile p;
        HabitBook book(&p);
        book.setToday(today);
        QVERIFY(book.add({{"name", ""}}).isEmpty());
        QVERIFY(book.add({{"name", "    "}}).isEmpty());
        QVERIFY(book.add({{"name", "Ok"}, {"reminderTime", "25:00"}}).isEmpty());
        const QString id = book.add({{"name", QString(300, 'x')}, {"difficulty", 99}, {"days", 0xFFFF}});
        QVERIFY(!id.isEmpty());
        const auto *h = book.find(id);
        QCOMPARE(h->name.size(), HabitBook::MaxNameLength);
        QCOMPARE(h->difficulty, 4);
        QCOMPARE(h->days, 0x7f);
        QVERIFY(!book.update(id, {{"name", " "}}));
        QCOMPARE(book.find(id)->name.size(), HabitBook::MaxNameLength); // Unchanged after a failed update.
        QVERIFY(!book.update("nope", {{"name", "x"}}));
    }

    void doneNeverPaysTwice() {
        Profile p;
        HabitBook book(&p);
        book.setToday(today);
        const QString id = book.add({{"name", "Swim"}, {"difficulty", 2}});
        QVERIFY(book.setStatus(id, iso(today), "done"));
        QVERIFY(book.setStatus(id, iso(today), "done"));
        QCOMPARE(p.fish(), 10);
        QCOMPARE(p.xp(), 10);
        // Done -> skipped takes the payout back; skipped -> done pays once.
        QVERIFY(book.setStatus(id, iso(today), "skipped"));
        QCOMPARE(p.fish(), 0);
        QVERIFY(book.setStatus(id, iso(today), "done"));
        QVERIFY(book.setStatus(id, iso(today), ""));
        QCOMPARE(p.fish(), 0);
        QCOMPARE(p.xp(), 0);
    }

    void refundUsesWhatWasPaid() {
        // Raising the difficulty after ticking must not refund more than was paid.
        Profile p;
        HabitBook book(&p);
        book.setToday(today);
        const QString id = book.add({{"name", "Read"}, {"difficulty", 0}});
        book.setStatus(id, iso(today), "done");
        QCOMPARE(p.fish(), 2);
        book.update(id, {{"difficulty", 4}});
        book.setStatus(id, iso(today), "");
        QCOMPARE(p.fish(), 0);
    }

    void undoAfterSpendingGoesNegative() {
        Profile p;
        HabitBook book(&p);
        book.setToday(today);
        const QString id = book.add({{"name", "Run"}, {"difficulty", 3}});
        book.setStatus(id, iso(today), "done");
        QVERIFY(p.spend(20));
        book.setStatus(id, iso(today), "");
        QCOMPARE(p.fish(), -20); // No free sardines from tick, buy, untick.
    }

    void noTimeTravel() {
        Profile p;
        HabitBook book(&p);
        book.setToday(today);
        const QString id = book.add({{"name", "Walk"}});
        QVERIFY(!book.setStatus(id, iso(today.addDays(1)), "done"));
        QVERIFY(!book.setStatus(id, iso(today.addDays(-1)), "done")); // Before it existed.
        QVERIFY(!book.setStatus(id, "not-a-date", "done"));
        QVERIFY(!book.setStatus(id, iso(today), "finished"));
        QVERIFY(!book.setStatus("ghost", iso(today), "done"));
        QCOMPARE(p.fish(), 0);
        book.shiftDay(1);
        QCOMPARE(book.viewDate(), iso(today));
    }

    void backfillFromAPastDay() {
        Profile p;
        HabitBook book(&p);
        book.setToday(today);
        book.shiftDay(-2);
        const QString id = book.add({{"name", "Stretch"}});
        QCOMPARE(book.find(id)->created, today.addDays(-2));
        QVERIFY(book.setStatus(id, iso(today.addDays(-2)), "done"));
    }

    void scheduleAndSections() {
        Profile p;
        HabitBook book(&p);
        book.setToday(today); // Wednesday
        const QString wed = book.add({{"name", "A"}, {"days", 1 << 2}});
        const QString mon = book.add({{"name", "B"}, {"days", 1 << 0}});
        const QString grouped = book.add({{"name", "C"}, {"group", "Gym"}});
        const QVariantList sections = book.sections("todo");
        // Ungrouped, Gym, then not scheduled today.
        QCOMPARE(sections.size(), 3);
        QCOMPARE(sections.at(0).toMap().value("habits").toList().size(), 1);
        QCOMPARE(sections.at(1).toMap().value("title").toString(), QString("Gym"));
        QCOMPARE(sections.at(2).toMap().value("key").toString(), QString("unscheduled"));
        QCOMPARE(sections.at(2).toMap().value("habits").toList().first().toMap().value("id").toString(), mon);
        book.setStatus(wed, iso(today), "done");
        QCOMPARE(book.sections("done").first().toMap().value("habits").toList().size(), 1);
        QCOMPARE(book.sections("todo").at(0).toMap().value("habits").toList().size(), 0);
        QCOMPARE(book.progress().value("scheduled").toInt(), 2);
        QCOMPARE(book.progress().value("done").toInt(), 1);
        Q_UNUSED(grouped);
    }

    void groupsSortIgnoringCaseAndAccents() {
        Profile p;
        HabitBook book(&p);
        book.setToday(today);
        QVERIFY(book.sections("todo").isEmpty()); // The page shows its "no habits" hint.
        QCOMPARE(book.sections("done").size(), 1);
        for (const char *group : {"Übungen", "Zähne", "Morgens", "abends"})
            book.add({{"name", "x"}, {"group", QString::fromUtf8(group)}});
        // Sections and the Group picker agree, A–Z on every platform.
        const QStringList order{"abends", "Morgens", "Übungen", "Zähne"};
        QCOMPARE(book.groups(), order);
        QStringList titles;
        for (const auto &section : book.sections("todo"))
            titles << section.toMap().value("title").toString();
        QCOMPARE(titles, QStringList() << QString() << order << QString()); // Ungrouped, …, unscheduled.
    }

    void perfectDays() {
        Profile p;
        HabitBook book(&p);
        book.setToday(monday);
        const QString a = book.add({{"name", "A"}});
        const QString b = book.add({{"name", "B"}});
        book.setToday(monday.addDays(3)); // Thursday
        // Mon: both done. Tue: done + skipped. Wed: both skipped. Thu: open.
        for (const auto &id : {a, b})
            book.setStatus(id, iso(monday), "done");
        book.setStatus(a, iso(monday.addDays(1)), "done");
        book.setStatus(b, iso(monday.addDays(1)), "skipped");
        book.setStatus(a, iso(monday.addDays(2)), "skipped");
        book.setStatus(b, iso(monday.addDays(2)), "skipped");
        QCOMPARE(book.perfectDay(monday), 1);
        QCOMPARE(book.perfectDay(monday.addDays(1)), 1);
        QCOMPARE(book.perfectDay(monday.addDays(2)), 0); // Skipping everything is not perfect.
        QCOMPARE(book.longestPerfectStreak(), 2);
        QCOMPARE(book.currentPerfectStreak(), 0);
        // Fix Wednesday; today (Thursday) still open must not break the streak.
        book.setStatus(a, iso(monday.addDays(2)), "done");
        QCOMPARE(book.currentPerfectStreak(), 3);
        book.setStatus(a, iso(monday.addDays(3)), "done");
        book.setStatus(b, iso(monday.addDays(3)), "done");
        QCOMPARE(book.currentPerfectStreak(), 4);
        QCOMPARE(book.progress().value("streak").toInt(), 4);
    }

    void daysWithoutPlansAreNeutral() {
        Profile p;
        HabitBook book(&p);
        book.setToday(monday);
        const QString weekdays = book.add({{"name", "Work"}, {"days", 0x1f}}); // Mon–Fri
        book.setToday(monday.addDays(7));                                      // Next Monday
        for (int i = 0; i < 5; ++i)
            book.setStatus(weekdays, iso(monday.addDays(i)), "done");
        book.setStatus(weekdays, iso(monday.addDays(7)), "done");
        // The weekend in between had nothing planned and does not break it.
        QCOMPARE(book.currentPerfectStreak(), 6);
        QCOMPARE(book.perfectDay(monday.addDays(5)), -1);
    }

    void streakGraceAndStrict() {
        Profile p;
        HabitBook book(&p);
        book.setToday(monday);
        const QString id = book.add({{"name", "Daily"}});
        book.setToday(monday.addDays(6));
        // Done, done, missed, done, missed, missed, done(today)
        for (int day : {0, 1, 3, 6})
            book.setStatus(id, iso(monday.addDays(day)), "done");
        QCOMPARE(book.longestStreak(*book.find(id)), 3); // One missed day is forgiven.
        p.setStrictStreaks(true);
        QCOMPARE(book.longestStreak(*book.find(id)), 2);
        // A skip pauses instead of breaking, even when strict.
        book.setStatus(id, iso(monday.addDays(2)), "skipped");
        QCOMPARE(book.longestStreak(*book.find(id)), 3);
    }

    void deletingKeepsHistory() {
        Profile p;
        HabitBook book(&p);
        book.setToday(today);
        const QString id = book.add({{"name", "Old"}, {"difficulty", 1}});
        book.setStatus(id, iso(today), "done");
        QVERIFY(book.remove(id));
        QVERIFY(!book.remove(id));
        QCOMPARE(p.fish(), 5);
        QCOMPARE(book.stats().value("totalCompleted").toInt(), 1);
        QCOMPARE(book.sections("done").first().toMap().value("habits").toList().size(), 0);
    }

    void heatmapShape() {
        Profile p;
        HabitBook book(&p);
        book.setToday(today);
        const QVariantList cells = book.heatmap(4);
        QCOMPARE(cells.size(), 28);
        QCOMPARE(cells.first().toMap().value("date").toString(), iso(monday.addDays(-21)));
        QVERIFY(!cells.at(23).toMap().value("future").toBool()); // Wednesday is today.
        QVERIFY(cells.at(24).toMap().value("future").toBool());
        p.setWeekStartsMonday(false);
        QCOMPARE(book.heatmap(1).first().toMap().value("date").toString(), iso(QDate(2026, 11, 15)));
        QCOMPARE(book.heatmap(0).size(), 7);
        QCOMPARE(book.heatmap(100000).size(), 104 * 7);
    }

    void midnightMovesToday() {
        Profile p;
        HabitBook book(&p);
        book.setToday(today);
        QVERIFY(book.viewingToday());
        book.setToday(today.addDays(1));
        QCOMPARE(book.viewDate(), iso(today.addDays(1)));
        // Someone browsing an older day stays there.
        book.shiftDay(-3);
        book.setToday(today.addDays(2));
        QCOMPARE(book.viewDate(), iso(today.addDays(-2)));
    }

    void browsingDaysSavesNothing() {
        Profile p;
        HabitBook book(&p);
        book.setToday(today);
        const QString id = book.add({{"name", "Walk"}});
        QSignalSpy changed(&book, &HabitBook::changed);
        QSignalSpy edited(&book, &HabitBook::edited);
        book.shiftDay(-1);
        book.goToday();
        book.goToday(); // Already there: not even a refresh.
        QCOMPARE(changed.size(), 2);
        QCOMPARE(edited.size(), 0);
        book.setStatus(id, iso(today), "done");
        QCOMPARE(edited.size(), 1);
    }

    void ticksDatedAheadSurviveFlyingWest() {
        // Ticked in Tokyo, reopened in Los Angeles where it is still the day before.
        Profile p;
        HabitBook book(&p);
        book.setToday(today.addDays(-1));
        const QString id = book.add({{"name", "Run"}, {"difficulty", 3}});
        book.setToday(today);
        QVERIFY(book.setStatus(id, iso(today), "done"));

        Profile p2;
        HabitBook west(&p2);
        west.setToday(today.addDays(-1));
        west.fromJson(book.toJson());
        west.setToday(today);
        QCOMPARE(west.status(id, today), QString("done"));
        QVERIFY(west.setStatus(id, iso(today), "done"));
        QCOMPARE(p2.fish(), 0); // Not paid a second time.
    }

    void habitsRoundTripAndSanitize() {
        Profile p;
        HabitBook book(&p);
        book.setToday(today);
        const QString id = book.add({{"name", "Keep"}, {"group", "G"}, {"reminder", true}, {"reminderTime", "07:30"}});
        book.setStatus(id, iso(today), "done");
        const QJsonObject saved = book.toJson();

        Profile p2;
        HabitBook copy(&p2);
        copy.setToday(today);
        copy.fromJson(saved);
        QCOMPARE(copy.count(), 1);
        QCOMPARE(copy.find(id)->reminderTime, QString("07:30"));
        QCOMPARE(copy.status(id, today), QString("done"));
        QCOMPARE(p2.fish(), 0); // Loading never pays out again.

        // Duplicates, nameless habits, bad dates and bad statuses are dropped.
        QJsonObject broken = saved;
        QJsonArray list = broken.value("habits").toArray();
        list.append(list.first());
        list.append(QJsonObject{{"id", "x"}, {"name", ""}});
        list.append(QJsonObject{{"id", "y"}, {"name", "Bad time"}, {"reminderTime", "99:99"}, {"created", "garbage"}});
        broken["habits"] = list;
        QJsonObject log = broken.value("log").toObject();
        log["2026-13-45"] = QJsonObject{{id, QJsonObject{{"status", "done"}}}};
        log[iso(today.addDays(-1))] = QJsonObject{{id, QJsonObject{{"status", "won"}, {"reward", -9}}}};
        broken["log"] = log;
        copy.fromJson(broken);
        QCOMPARE(copy.count(), 2);
        QCOMPARE(copy.find("y")->reminderTime, QString("09:00"));
        QCOMPARE(copy.find("y")->created, today);
        QCOMPARE(copy.stats().value("totalCompleted").toInt(), 1);
    }

    void ancientDatesCannotFreezeTheApp() {
        Profile p;
        HabitBook book(&p);
        book.setToday(today);
        const QJsonObject evil{
            {"habits", QJsonArray{QJsonObject{{"id", "a"}, {"name", "A"}, {"created", "0001-01-01"}}}},
            {"log", QJsonObject{{"0001-01-02", QJsonObject{{"a", QJsonObject{{"status", "done"}}}}},
                                {"9999-01-01", QJsonObject{{"a", QJsonObject{{"status", "done"}}}}}}}};
        book.fromJson(evil);
        QCOMPARE(book.find("a")->created, today);
        QElapsedTimer timer;
        timer.start();
        book.stats();
        book.heatmap(104);
        QVERIFY(timer.elapsed() < 500);
        QCOMPARE(book.stats().value("totalCompleted").toInt(), 0);
    }

    // ---------------------------------------------------------------- catalog

    void catalogLoadsCleanly() {
        Catalog catalog;
        QVERIFY2(catalog.error().isEmpty(), qPrintable(catalog.error()));
        QVERIFY(catalog.variantIds().size() > 30);
        for (const auto &id : catalog.variantIds()) {
            const auto *v = catalog.variant(id);
            QVERIFY2(v->terrain.isEmpty() == !v->sprite.isEmpty(), qPrintable(id));
            if (!v->sprite.isEmpty())
                QVERIFY2(QFile::exists(":/sprites/" + v->sprite), qPrintable(v->sprite));
            QVERIFY2(v->price > 0, qPrintable(id));
            QVERIFY2(!catalog.text(v->name).isEmpty(), qPrintable(id));
        }
        // Every tray an item uses exists, and so does every category.
        QSet<QString> trays, categories;
        for (const auto &t : catalog.trayCategories())
            trays << t.toMap().value("id").toString();
        for (const auto &c : catalog.categories())
            categories << c.toMap().value("id").toString();
        for (const auto &v : catalog.shopItems({}, "newest", {}, false, 0)) {
            const auto *item = catalog.item(v.toMap().value("id").toString());
            QVERIFY2(trays.contains(item->tray), qPrintable(item->id));
            QVERIFY2(categories.contains(item->category), qPrintable(item->id));
        }
        QVERIFY(catalog.templates().size() >= 5);
    }

    void catalogTranslatesAndFallsBack() {
        Catalog catalog;
        QCOMPARE(catalog.text(catalog.variant("campfire")->name), QString("Campfire"));
        catalog.setLanguage("de");
        QCOMPARE(catalog.text(catalog.variant("campfire")->name), QString("Lagerfeuer"));
        catalog.setLanguage("ja"); // No Japanese names: English.
        QCOMPARE(catalog.text(catalog.variant("campfire")->name), QString("Campfire"));
        catalog.setLanguage("de-AT"); // Regional falls back to the base language.
        QCOMPARE(catalog.text(catalog.variant("campfire")->name), QString("Lagerfeuer"));
    }

    void shopSearchSortFilter() {
        Catalog catalog;
        catalog.setLanguage("de");
        // German and English both find it, and variants count as hits.
        QCOMPARE(catalog.shopItems("lager", "newest", {}, false, 0).size(), 1);
        QCOMPARE(catalog.shopItems("CAMPFIRE", "newest", {}, false, 0).size(), 1);
        QVERIFY(catalog.shopItems("klein", "newest", {}, false, 0).size() >= 2);
        const auto byPrice = catalog.shopItems({}, "priceLow", {}, false, 0);
        for (int i = 1; i < byPrice.size(); ++i)
            QVERIFY(byPrice.at(i - 1).toMap().value("price").toInt() <= byPrice.at(i).toMap().value("price").toInt());
        for (const auto &v : catalog.shopItems({}, "newest", {"terrain"}, false, 0))
            QCOMPARE(v.toMap().value("category").toString(), QString("terrain"));
        for (const auto &v : catalog.shopItems({}, "newest", {}, true, 12))
            QVERIFY(v.toMap().value("price").toInt() <= 12);
        QVERIFY(catalog.shopItems("zzzz", "newest", {}, false, 0).isEmpty());
    }

    void brokenCatalogIsReported() {
        QTemporaryDir dir;
        write(dir.filePath("items.json"), "{\"items\": [\n"
"            {\"id\": \"a\", \"variants\": [{\"id\": \"v\", \"price\": 1, \"sprite\": \"x.svg\"}]},\n"
"            {\"id\": \"a\", \"variants\": [{\"id\": \"w\", \"price\": 1, \"sprite\": \"x.svg\"}]},\n"
"            {\"id\": \"b\", \"variants\": [{\"id\": \"v\", \"price\": 1, \"sprite\": \"x.svg\"}]}]}");
        write(dir.filePath("templates.json"), "{not json");
        Catalog catalog(dir.filePath("items.json"), dir.filePath("templates.json"));
        QVERIFY(!catalog.error().isEmpty());
        QCOMPARE(catalog.variantIds().size(), 1);
        QVERIFY(catalog.templates().isEmpty());
    }

    // ----------------------------------------------------------------- island

    void freshIslandHasACamp() {
        Catalog catalog;
        Profile p;
        Island island(&catalog, &p);
        island.seed();
        QVERIFY(island.objects().size() >= 6);
        const QVariantMap spot = island.restSpot();
        QVERIFY(spot.contains("x"));
        QVERIFY(island.isWalkable(spot.value("x").toInt(), spot.value("y").toInt()));
        // Seeded items count as owned, so they can be picked up and put back.
        QCOMPARE(island.owned("nest"), 1);
        QCOMPARE(island.available("nest"), 0);
        // The island is in the middle with water all around.
        QVERIFY(Island::isBaseLand(13, 13));
        QVERIFY(!Island::isBaseLand(0, 0));
        QVERIFY(!Island::isBaseLand(Island::Size - 1, 13));
        QVERIFY(!Island::isBaseLand(-1, 5));
    }

    void objectModelChangesOnlyWhatChanged() {
        Catalog catalog;
        Profile p;
        Island island(&catalog, &p);
        island.seed();
        QAbstractListModel *model = island.objectModel();
        QCOMPARE(model->rowCount(), island.objects().size());

        QSignalSpy changed(model, &QAbstractItemModel::dataChanged);
        QSignalSpy inserted(model, &QAbstractItemModel::rowsInserted);
        QSignalSpy removed(model, &QAbstractItemModel::rowsRemoved);
        QSignalSpy ground(&island, &Island::groundChanged);
        const QVariantMap first = island.objects().first().toMap();
        const int uid = first.value("uid").toInt();
        // An empty land tile next to the camp.
        int tx = -1, ty = -1;
        for (int y = 10; y < 18 && tx < 0; ++y)
            for (int x = 10; x < 18 && tx < 0; ++x)
                if (island.isLand(x, y) && !island.objectAt(x, y)) { tx = x; ty = y; }
        QVERIFY(tx >= 0);
        QVERIFY(island.move(uid, tx, ty));
        // One row changed, nothing rebuilt, and the ground stays as it is.
        QCOMPARE(changed.size(), 1);
        QCOMPARE(inserted.size(), 0);
        QCOMPARE(removed.size(), 0);
        QCOMPARE(ground.size(), 0);
        const QModelIndex row = changed.first().first().value<QModelIndex>();
        QCOMPARE(row.data(ObjectModel::UidRole).toInt(), uid);
        QCOMPARE(row.data(ObjectModel::TileXRole).toInt(), tx);

        QVERIFY(island.remove(uid));
        QCOMPARE(removed.size(), 1);
        QCOMPARE(model->rowCount(), island.objects().size());
    }

    void buyingAndPlacing() {
        Catalog catalog;
        Profile p;
        Island island(&catalog, &p);
        island.seed();
        QCOMPARE(island.buy("campfire"), QString("fish"));
        QCOMPARE(island.owned("campfire"), 0);
        QCOMPARE(island.buy("does-not-exist"), QString("unknown"));
        p.reward(60, 0);
        QCOMPARE(island.buy("campfire"), QString());
        QCOMPARE(p.fish(), 10);
        QCOMPARE(island.canPlace("campfire", 0, 0), QString("water"));
        QCOMPARE(island.canPlace("campfire", 14, 13), QString("occupied")); // The nest.
        QCOMPARE(island.canPlace("campfire", 99, 99), QString("bounds"));
        QVERIFY(island.place("campfire", 13, 14));
        QCOMPARE(island.canPlace("campfire", 15, 15), QString("none")); // Only one owned.
        const int uid = island.objectAt(13, 14);
        QVERIFY(uid > 0);
        QVERIFY(!island.move(uid, 14, 13)); // Onto the nest.
        QVERIFY(!island.move(uid, 0, 0));   // Into the sea.
        QVERIFY(island.move(uid, 15, 14));
        QVERIFY(island.flip(uid));
        QVERIFY(island.remove(uid));
        QCOMPARE(island.available("campfire"), 1);
        QVERIFY(!island.remove(uid));
    }

    void terrainGrowsTheIsland() {
        Catalog catalog;
        Profile p;
        Island island(&catalog, &p);
        island.seed();
        p.reward(100, 0);
        for (int i = 0; i < 3; ++i)
            QCOMPARE(island.buy("tile-sand"), QString());
        // Find a water tile next to the coast and one far out.
        int wx = -1, wy = -1;
        for (int x = 0; x < Island::Size && wx < 0; ++x)
            for (int y = 0; y < Island::Size; ++y)
                if (!island.isLand(x, y) && island.isLand(x + 1, y)) { wx = x; wy = y; break; }
        QVERIFY(wx >= 0);
        QCOMPARE(island.canPlace("tile-sand", 1, 1), QString("water"));
        QVERIFY(island.place("tile-sand", wx, wy));
        QVERIFY(island.isLand(wx, wy));
        QCOMPARE(island.canPlace("tile-sand", wx, wy), QString("terrain"));
        // Something standing on the new land keeps the tile from being lifted.
        p.reward(10, 0);
        island.buy("shell");
        QVERIFY(island.place("shell", wx, wy));
        QCOMPARE(island.removeTerrain(wx, wy), QString("occupied"));
        QVERIFY(island.remove(island.objectAt(wx, wy)));
        QCOMPARE(island.removeTerrain(wx, wy), QString());
        QVERIFY(!island.isLand(wx, wy));
        QCOMPARE(island.removeTerrain(wx, wy), QString("none"));
        QCOMPARE(island.available("tile-sand"), 3);
        // Terrain on base land just recolors it.
        QVERIFY(island.place("tile-sand", 13, 13));
        QCOMPARE(island.terrainKindAt(13, 13), QString("sand"));
    }

    void pathsAvoidObstacles() {
        Catalog catalog;
        Profile p;
        Island island(&catalog, &p);
        island.fromJson({}); // Bare island, nothing placed.
        QVERIFY(island.objects().isEmpty());
        const QVariantList straight = island.path(12, 13, 15, 13);
        QCOMPARE(straight.size(), 3);
        // A boulder in the way forces a detour.
        p.reward(100, 0);
        island.buy("boulder-small");
        QVERIFY(island.place("boulder-small", 13, 13));
        const QVariantList detour = island.path(12, 13, 15, 13);
        QVERIFY(detour.size() > 3);
        for (const auto &step : detour)
            QVERIFY(island.isWalkable(step.toMap().value("x").toInt(), step.toMap().value("y").toInt()));
        QVERIFY(island.path(12, 13, 0, 0).isEmpty()); // Water.
        QVERIFY(island.path(12, 13, 13, 13).isEmpty()); // Onto the boulder.
        // Plants do not block.
        island.buy("shell");
        QVERIFY(island.place("shell", 14, 13));
        QVERIFY(island.isWalkable(14, 13));
    }

    void islandLoadIsDefensive() {
        Catalog catalog;
        Profile p;
        Island island(&catalog, &p);
        const QJsonObject data{
            {"owned", QJsonObject{{"campfire", 1}, {"ghost-item", 5}, {"shell", -3}, {"tile-sand", 1}}},
            {"terrain", QJsonArray{QJsonObject{{"x", 1}, {"y", 1}, {"variant", "tile-sand"}},
                                   QJsonObject{{"x", 2}, {"y", 2}, {"variant", "tile-sand"}}}},
            {"objects", QJsonArray{
                QJsonObject{{"uid", 7}, {"variant", "campfire"}, {"x", 13}, {"y", 13}},
                QJsonObject{{"uid", 7}, {"variant", "campfire"}, {"x", 14}, {"y", 14}}, // Over the owned count.
                QJsonObject{{"uid", 8}, {"variant", "ghost-item"}, {"x", 12}, {"y", 12}},
                QJsonObject{{"uid", 9}, {"variant", "tile-sand"}, {"x", 12}, {"y", 12}}}}};
        island.fromJson(data);
        QCOMPARE(island.objects().size(), 1);
        QCOMPARE(island.owned("ghost-item"), 0);
        QCOMPARE(island.owned("shell"), 0);
        QVERIFY(island.isLand(1, 1));
        QVERIFY(!island.isLand(2, 2)); // Only one tile owned.
        // New objects never reuse a loaded uid.
        p.reward(10, 0);
        island.buy("shell");
        island.place("shell", 12, 13);
        QVERIFY(island.objectAt(12, 13) > 7);
    }

    // ------------------------------------------------------------------- game

    void gameStartsAndRoundTrips() {
        QTemporaryDir dir;
        const QString path = dir.filePath("nested/save.json");
        {
            Game game(path);
            game.load();
            QVERIFY(QFile::exists(path));
            QCOMPARE(game.profile()->fish(), 10);
            game.habits()->add({{"name", "Persist"}});
            game.island()->buy("shell");
            QVERIFY(game.save());
        }
        Game again(path);
        again.load();
        QVERIFY(again.loadError().isEmpty());
        QCOMPARE(again.habits()->count(), 1);
        QCOMPARE(again.profile()->fish(), 5);
        QCOMPARE(again.island()->owned("shell"), 1);
    }

    void corruptSaveIsKeptAside() {
        QTemporaryDir dir;
        const QString path = dir.filePath("save.json");
        write(path, "{ this is not json");
        Game game(path);
        game.load();
        QVERIFY(!game.loadError().isEmpty());
        const QStringList kept = QDir(dir.path()).entryList({"save.json.broken-*"});
        QCOMPARE(kept.size(), 1);
        QFile broken(dir.filePath(kept.first()));
        QVERIFY(broken.open(QIODevice::ReadOnly));
        QCOMPARE(broken.readAll(), QByteArray("{ this is not json"));
        QVERIFY(QFile::exists(path)); // A fresh save next to it.
    }

    void newerSaveIsNeverOverwritten() {
        QTemporaryDir dir;
        const QString path = dir.filePath("save.json");
        const QByteArray future = "{\"version\": 99, \"profile\": {\"fish\": 777}}";
        write(path, future);
        Game game(path);
        game.load();
        QVERIFY(!game.loadError().isEmpty());
        QVERIFY(!game.save());
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), future);
    }

    void unwritableSaveIsReported() {
        QTemporaryDir dir;
        write(dir.filePath("blocker"), "a file where a directory should be");
        Game game(dir.filePath("blocker/save.json"));
        game.load();
        QVERIFY(!game.save());
        QVERIFY(!game.saveError().isEmpty());
    }

    // -------------------------------------------------------------- reminders

    void remindersFireOncePerDay() {
        Profile p;
        HabitBook book(&p);
        book.setToday(today);
        Reminders reminders(&book, &p);
        QStringList sent;
        reminders.setSender([&](const QString &title, const QString &) { sent << title; });
        const QString id = book.add({{"name", "Floss"}, {"reminder", true}, {"reminderTime", "21:15"}});
        p.setMorningReminder(true);
        p.setEveningReminder(true);

        reminders.check(QDateTime(today, QTime(8, 5)));
        QVERIFY(sent.isEmpty()); // Notifications are off.
        p.setNotifications(true);
        reminders.check(QDateTime(today, QTime(7, 59)));
        QVERIFY(sent.isEmpty());
        reminders.check(QDateTime(today, QTime(8, 5)));
        reminders.check(QDateTime(today, QTime(8, 10)));
        QCOMPARE(sent.size(), 1);
        reminders.check(QDateTime(today, QTime(9, 0))); // Window over.
        QCOMPARE(sent.size(), 1);
        reminders.check(QDateTime(today, QTime(20, 1)));
        QCOMPARE(sent.size(), 2);
        // Done before its time: no habit reminder.
        book.setStatus(id, iso(today), "done");
        reminders.check(QDateTime(today, QTime(21, 20)));
        QCOMPARE(sent.size(), 2);
        // Next day everything is due again.
        reminders.check(QDateTime(today.addDays(1), QTime(8, 1)));
        QCOMPARE(sent.size(), 3);
    }

    void plannedRemindersCoverTheComingDays() {
        Profile p;
        HabitBook book(&p);
        book.setToday(today);
        Reminders reminders(&book, &p);
        const QString id = book.add({{"name", "Floss"}, {"reminder", true}, {"reminderTime", "21:15"}});
        p.setMorningReminder(true);
        p.setEveningReminder(true);
        const QDateTime noon(today, QTime(12, 0));
        QVERIFY(reminders.planned(noon, 3).isEmpty()); // Notifications are off.

        p.setNotifications(true);
        auto plan = reminders.planned(noon, 3);
        // Today's morning has passed: evening + habit today, then all three
        // on each of the next two days, in time order.
        QCOMPARE(plan.size(), 8);
        QCOMPARE(plan.first().key, "evening:" + iso(today));
        QCOMPARE(plan.at(1).key, "habit:" + id + ":" + iso(today));
        QCOMPARE(plan.at(2).key, "morning:" + iso(today.addDays(1)));
        for (int i = 1; i < plan.size(); ++i)
            QVERIFY(plan.at(i - 1).at <= plan.at(i).at);

        // Done today: no habit or evening reminder today, later days unchanged.
        book.setStatus(id, iso(today), "done");
        plan = reminders.planned(noon, 3);
        QCOMPARE(plan.size(), 6);
        QCOMPARE(plan.first().key, "morning:" + iso(today.addDays(1)));
    }

    void eveningSkipsWhenAllDoneAndLateRemindersWork() {
        Profile p;
        HabitBook book(&p);
        book.setToday(today);
        Reminders reminders(&book, &p);
        QStringList sent;
        reminders.setSender([&](const QString &title, const QString &) { sent << title; });
        p.setNotifications(true);
        p.setEveningReminder(true);
        const QString late = book.add({{"name", "Late"}, {"reminder", true}, {"reminderTime", "23:50"}});
        reminders.check(QDateTime(today, QTime(23, 59, 30)));
        QCOMPARE(sent, QStringList{"Late"}); // Past 20:30, so no evening one.
        book.setStatus(late, iso(today), "done");
        // The next evening it is open again (nothing ticked for the 19th).
        reminders.check(QDateTime(today.addDays(1), QTime(20, 5)));
        QCOMPARE(sent.size(), 2);
    }

    void firedRemindersSurviveARestart() {
        Profile p;
        HabitBook book(&p);
        book.setToday(today);
        p.setNotifications(true);
        p.setEveningReminder(true);
        book.add({{"name", "Floss"}});
        QStringList sent;
        auto sender = [&](const QString &title, const QString &) { sent << title; };
        Reminders first(&book, &p);
        first.setSender(sender);
        first.check(QDateTime(today, QTime(20, 5)));
        QCOMPARE(sent.size(), 1);
        // A new process inside the same window remembers it from the save...
        Reminders again(&book, &p);
        again.setSender(sender);
        again.fromJson(first.toJson());
        again.check(QDateTime(today, QTime(20, 10)));
        QCOMPARE(sent.size(), 1);
        // ...which is what keeps it from sending it twice.
        Reminders forgetful(&book, &p);
        forgetful.setSender(sender);
        forgetful.check(QDateTime(today, QTime(20, 10)));
        QCOMPARE(sent.size(), 2);
    }

    // ----------------------------------------------------------------- updater

    void updaterReadsReleaseInfo() {
        // version.json: a release built for a newer Qt than this system's is
        // left out (an older known one stays offered); unsafe file names are refused.
        Updater updater(nullptr, QString(), 5);
        const auto release = [](int build, const QByteArray &qt) {
            return "{\"build\":" + QByteArray::number(build) + ",\"version\":\"0.2." + QByteArray::number(build)
                   + "\",\"assets\":{\"" + Updater::platformKey().toUtf8() + "\":{\"file\":\"pengu.pkg\",\"qt\":\"" + qt + "\"}}}";
        };
        QVERIFY(updater.parseRelease(release(7, "99.0")));
        QCOMPARE(updater.latestVersion(), QString());
        QVERIFY(updater.parseRelease(release(7, "6.0")));
        QCOMPARE(updater.latestVersion(), QString("0.2.7"));
        QVERIFY(updater.parseRelease(release(8, "")));
        QCOMPARE(updater.latestVersion(), QString("0.2.8"));
        QVERIFY(updater.parseRelease(release(9, "99.0")));
        QCOMPARE(updater.latestVersion(), QString("0.2.8"));
        QVERIFY(!updater.parseRelease("{\"build\":10,\"assets\":{\"linux\":{\"file\":\"../x\"},\"android\":{\"file\":\"../x\"}}}"));
    }

    // ------------------------------------------------- theme and translations

    void themeFallsBack() {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#FFD60A"));
    }

    void translatorFollowsTheLanguageFile() {
        QTemporaryDir dir;
        write(dir.path() + "/language", "de\n");
        Translator translator(dir.path() + "/language", ":/i18n");
        QCOMPARE(translator.language(), QString("de"));
        QCOMPARE(translator.translate("Main", "Shop"), QString("Shop"));
        QCOMPARE(translator.translate("Main", "Settings"), QString("Einstellungen"));
    }

    void everyStringIsTranslatedToGerman() {
        // Collects qsTr("…") and tr("…") from the sources and checks de.json.
        QFile catalogFile(":/i18n/de.json");
        QVERIFY(catalogFile.open(QIODevice::ReadOnly));
        const QJsonObject de = QJsonDocument::fromJson(catalogFile.readAll()).object();
        QVERIFY(!de.isEmpty());
        const QRegularExpression call("\\b(?:qsTr|tr)\\(\"((?:[^\"\\\\]|\\\\.)*)\"");
        QStringList missing;
        const QDir src(PENGU_SOURCE_DIR);
        QStringList files;
        for (const auto &f : QDir(src.filePath("qml")).entryList({"*.qml"}))
            files << src.filePath("qml/" + f);
        for (const auto &f : src.entryList({"*.cpp"}))
            files << src.filePath(f);
        QVERIFY(files.size() > 20);
        for (const auto &path : files) {
            QFile file(path);
            QVERIFY(file.open(QIODevice::ReadOnly));
            const QString text = QString::fromUtf8(file.readAll());
            for (auto it = call.globalMatch(text); it.hasNext();) {
                QString key = it.next().captured(1);
                key.replace("\\\"", "\"").replace("\\\\", "\\");
                if (!de.contains(key))
                    missing << QFileInfo(path).fileName() + ": " + key;
            }
        }
        missing.removeDuplicates();
        QVERIFY2(missing.isEmpty(), qPrintable("\n" + missing.join("\n")));
    }
};

QTEST_MAIN(PenguTests)
#include "pengu_tests.moc"
