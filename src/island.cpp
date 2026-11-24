#include "island.h"

#include "catalog.h"
#include "profile.h"

#include <QJsonArray>
#include <QQueue>
#include <QRandomGenerator>
#include <QSet>
#include <cmath>

namespace {
int key(int x, int y) { return y * Island::Size + x; }

// A small fixed hash so the coastline is irregular but the same every run.
double noise(int x, int y) {
    quint32 h = quint32(x) * 374761393u + quint32(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return double(h ^ (h >> 16)) / 4294967295.0;
}
} // namespace

Island::Island(Catalog *catalog, Profile *profile, QObject *parent)
    : QObject(parent), m_catalog(catalog), m_profile(profile) {}

void Island::touch() {
    ++m_revision;
    QList<ObjectModel::Row> rows;
    rows.reserve(m_objects.size());
    for (const auto &o : m_objects)
        if (const auto *v = m_catalog->variant(o.variant))
            rows.append({o.uid, o.variant, o.x, o.y, o.flipped, v->sprite, v->width, v->height, v->glow,
                         m_catalog->text(v->name)});
    m_model.sync(rows);
    emit changed();
}

void Island::touchGround() {
    emit groundChanged();
    touch();
}

bool Island::isBaseLand(int x, int y) {
    if (!inBounds(x, y))
        return false;
    const double dx = x - 13.5, dy = y - 13.5;
    return std::sqrt(dx * dx + dy * dy) <= 3.7 + noise(x, y) * 1.1;
}

bool Island::isLand(int x, int y) const {
    return inBounds(x, y) && (isBaseLand(x, y) || m_terrain.contains(key(x, y)));
}

QString Island::terrainAt(int x, int y) const { return m_terrain.value(key(x, y)); }

QString Island::terrainKindAt(int x, int y) const {
    if (const auto *v = m_catalog->variant(terrainAt(x, y)))
        return v->terrain;
    return isBaseLand(x, y) ? QStringLiteral("sand") : QString();
}

int Island::objectAt(int x, int y) const {
    for (const auto &o : m_objects)
        if (o.x == x && o.y == y)
            return o.uid;
    return 0;
}

bool Island::isWalkable(int x, int y) const {
    if (!isLand(x, y))
        return false;
    for (const auto &o : m_objects) {
        if (o.x != x || o.y != y)
            continue;
        const auto *v = m_catalog->variant(o.variant);
        return v && !v->solid;
    }
    return true;
}

int Island::shoreDistance(int x, int y) const {
    int best = 3;
    for (int dy = -2; dy <= 2; ++dy)
        for (int dx = -2; dx <= 2; ++dx)
            if (isLand(x + dx, y + dy))
                best = std::min(best, std::max(std::abs(dx), std::abs(dy)));
    return best;
}

QVariantList Island::objects() const {
    QVariantList list;
    for (const auto &o : m_objects) {
        const auto *v = m_catalog->variant(o.variant);
        if (!v)
            continue;
        list << QVariantMap{{"uid", o.uid},         {"variant", o.variant}, {"x", o.x},
                            {"y", o.y},             {"flipped", o.flipped}, {"sprite", v->sprite},
                            {"width", v->width},    {"height", v->height},  {"glow", v->glow},
                            {"name", m_catalog->text(v->name)}};
    }
    return list;
}

int Island::placedCount(const QString &variant) const {
    int n = 0;
    for (const auto &o : m_objects)
        n += o.variant == variant;
    for (const auto &t : m_terrain)
        n += t == variant;
    return n;
}

int Island::owned(const QString &variant, int) const { return m_owned.value(variant); }

int Island::available(const QString &variant, int) const {
    return std::max(0, m_owned.value(variant) - placedCount(variant));
}

QString Island::canPlace(const QString &variant, int x, int y) const {
    const auto *v = m_catalog->variant(variant);
    if (!v)
        return "unknown";
    if (!inBounds(x, y))
        return "bounds";
    if (available(variant) <= 0)
        return "none";
    if (!v->terrain.isEmpty()) {
        if (m_terrain.contains(key(x, y)))
            return "terrain";
        // New land must touch existing land, so the island grows in one piece.
        if (!isLand(x, y) && !(isLand(x + 1, y) || isLand(x - 1, y) || isLand(x, y + 1) || isLand(x, y - 1)))
            return "water";
        return {};
    }
    if (!isLand(x, y))
        return "water";
    if (objectAt(x, y))
        return "occupied";
    return {};
}

bool Island::place(const QString &variant, int x, int y) {
    if (!canPlace(variant, x, y).isEmpty())
        return false;
    if (!m_catalog->variant(variant)->terrain.isEmpty()) {
        m_terrain.insert(key(x, y), variant);
        touchGround();
    } else {
        m_objects << Placed{m_nextUid++, variant, x, y, false};
        touch();
    }
    return true;
}

Island::Placed *Island::findPlaced(int uid) {
    for (auto &o : m_objects)
        if (o.uid == uid)
            return &o;
    return nullptr;
}

bool Island::move(int uid, int x, int y) {
    Placed *o = findPlaced(uid);
    if (!o || !isLand(x, y))
        return false;
    if (o->x == x && o->y == y)
        return true;
    if (objectAt(x, y))
        return false;
    o->x = x;
    o->y = y;
    touch();
    return true;
}

bool Island::flip(int uid) {
    Placed *o = findPlaced(uid);
    if (!o)
        return false;
    o->flipped = !o->flipped;
    touch();
    return true;
}

bool Island::remove(int uid) {
    const auto before = m_objects.size();
    m_objects.removeIf([&](const Placed &o) { return o.uid == uid; });
    if (m_objects.size() == before)
        return false;
    touch(); // Back in the inventory: owned stays, placed drops.
    return true;
}

QString Island::removeTerrain(int x, int y) {
    if (!m_terrain.contains(key(x, y)))
        return "none";
    // Lifting a tile off open water would sink whatever stands on it.
    if (!isBaseLand(x, y) && objectAt(x, y))
        return "occupied";
    m_terrain.remove(key(x, y));
    touchGround();
    return {};
}

QString Island::buy(const QString &variant) {
    const auto *v = m_catalog->variant(variant);
    if (!v)
        return "unknown";
    if (!m_profile->spend(v->price))
        return "fish";
    m_owned[variant] += 1;
    touch();
    return {};
}

QVariantList Island::inventory(const QString &tray, int) const {
    QVariantList list;
    for (const auto &id : m_catalog->variantIds()) {
        const auto *v = m_catalog->variant(id);
        const auto *item = m_catalog->item(v->itemId);
        if (!item || (!tray.isEmpty() && item->tray != tray))
            continue;
        const int count = available(id);
        if (count <= 0)
            continue;
        list << QVariantMap{{"variant", id},
                            {"name", m_catalog->text(v->name)},
                            {"sprite", v->sprite},
                            {"terrain", v->terrain},
                            {"count", count}};
    }
    return list;
}

QVariantList Island::path(int fromX, int fromY, int toX, int toY) const {
    if (!inBounds(fromX, fromY) || !isWalkable(toX, toY))
        return {};
    QHash<int, int> previous;
    QQueue<int> queue;
    const int start = key(fromX, fromY), goal = key(toX, toY);
    previous.insert(start, -1);
    queue.enqueue(start);
    static const int steps[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (!queue.isEmpty()) {
        const int current = queue.dequeue();
        if (current == goal)
            break;
        const int cx = current % Size, cy = current / Size;
        for (const auto &s : steps) {
            const int nx = cx + s[0], ny = cy + s[1];
            const int k = key(nx, ny);
            if (!isWalkable(nx, ny) || previous.contains(k))
                continue;
            previous.insert(k, current);
            queue.enqueue(k);
        }
    }
    if (!previous.contains(goal))
        return {};
    QVariantList route;
    for (int k = goal; k != start; k = previous.value(k))
        route.prepend(QVariantMap{{"x", k % Size}, {"y", k / Size}});
    return route;
}

QVariantMap Island::randomWalkableTile() const {
    QList<int> tiles;
    for (int y = 0; y < Size; ++y)
        for (int x = 0; x < Size; ++x)
            if (isWalkable(x, y))
                tiles << key(x, y);
    if (tiles.isEmpty())
        return {};
    const int k = tiles.at(QRandomGenerator::global()->bounded(int(tiles.size())));
    return {{"x", k % Size}, {"y", k / Size}};
}

QVariantMap Island::restSpot() const {
    for (const auto &o : m_objects) {
        const auto *v = m_catalog->variant(o.variant);
        if (!v || !v->rest)
            continue;
        // Sits just in front of it (toward the viewer), or beside it.
        static const int around[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
        for (const auto &a : around)
            if (isWalkable(o.x + a[0], o.y + a[1]))
                return {{"x", o.x + a[0]}, {"y", o.y + a[1]}, {"faceX", o.x}, {"faceY", o.y}};
    }
    return {};
}

void Island::seed() {
    m_objects.clear();
    m_terrain.clear();
    m_owned.clear();
    m_nextUid = 1;
    // The starting camp: a nest, a milkwood tree and a few plants. They count
    // as owned so removing them puts them in the inventory.
    const struct { const char *variant; int x, y; } start[] = {
        {"nest", 14, 13},         {"milkwood-large", 13, 11}, {"protea-pink", 12, 15},
        {"protea-pink", 11, 16},  {"protea-white", 12, 17},  {"dune-grass", 16, 12},
        {"dune-grass", 15, 17},   {"boulder-small", 17, 14},
    };
    for (const auto &s : start) {
        if (!m_catalog->variant(s.variant) || !isLand(s.x, s.y) || objectAt(s.x, s.y))
            continue;
        m_owned[s.variant] += 1;
        m_objects << Placed{m_nextUid++, s.variant, s.x, s.y, false};
    }
    touchGround();
}

QJsonObject Island::toJson() const {
    QJsonArray objects;
    for (const auto &o : m_objects)
        objects << QJsonObject{{"uid", o.uid}, {"variant", o.variant}, {"x", o.x}, {"y", o.y}, {"flipped", o.flipped}};
    QJsonArray terrain;
    for (auto it = m_terrain.cbegin(); it != m_terrain.cend(); ++it)
        terrain << QJsonObject{{"x", it.key() % Size}, {"y", it.key() / Size}, {"variant", it.value()}};
    QJsonObject owned;
    for (auto it = m_owned.cbegin(); it != m_owned.cend(); ++it)
        owned.insert(it.key(), it.value());
    return {{"objects", objects}, {"terrain", terrain}, {"owned", owned}};
}

void Island::fromJson(const QJsonObject &object) {
    m_objects.clear();
    m_terrain.clear();
    m_owned.clear();
    m_nextUid = 1;
    const QJsonObject owned = object.value("owned").toObject();
    for (auto it = owned.begin(); it != owned.end(); ++it)
        if (m_catalog->variant(it.key()) && it.value().toInt() > 0)
            m_owned.insert(it.key(), std::min(it.value().toInt(), 9999));

    // Terrain first, so objects standing on added land are still on land.
    for (const auto &value : object.value("terrain").toArray()) {
        const QJsonObject t = value.toObject();
        const int x = t.value("x").toInt(-1), y = t.value("y").toInt(-1);
        const QString variant = t.value("variant").toString();
        const auto *v = m_catalog->variant(variant);
        if (!inBounds(x, y) || !v || v->terrain.isEmpty() || m_terrain.contains(key(x, y)))
            continue;
        if (placedCount(variant) >= m_owned.value(variant))
            continue; // Never more placed than owned, whatever the file says.
        m_terrain.insert(key(x, y), variant);
    }
    QSet<int> uids;
    for (const auto &value : object.value("objects").toArray()) {
        const QJsonObject o = value.toObject();
        Placed p{o.value("uid").toInt(), o.value("variant").toString(), o.value("x").toInt(-1),
                 o.value("y").toInt(-1), o.value("flipped").toBool()};
        const auto *v = m_catalog->variant(p.variant);
        if (!v || !v->terrain.isEmpty() || !isLand(p.x, p.y) || objectAt(p.x, p.y))
            continue;
        if (placedCount(p.variant) >= m_owned.value(p.variant))
            continue;
        if (p.uid <= 0 || uids.contains(p.uid))
            p.uid = 0; // Assigned below.
        uids.insert(p.uid);
        m_objects << p;
    }
    for (const auto &o : m_objects)
        m_nextUid = std::max(m_nextUid, o.uid + 1);
    for (auto &o : m_objects)
        if (o.uid == 0)
            o.uid = m_nextUid++;
    touchGround();
}
