#pragma once

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QVariantList>

#include "objectmodel.h"

class Catalog;
class Profile;

// The penguin island: a square isometric grid of water with a sandy island in
// the middle, terrain tiles that recolor or extend the land, decorations
// placed on single tiles, and the inventory of what has been bought.
class Island : public QObject {
    Q_OBJECT
    Q_PROPERTY(int size READ size CONSTANT)
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(QVariantList objects READ objects NOTIFY changed)
    // The same objects as a list model that changes row by row (for views).
    Q_PROPERTY(QAbstractListModel *objectModel READ objectModel CONSTANT)

public:
    static constexpr int Size = 28;

    struct Placed {
        int uid = 0;
        QString variant;
        int x = 0, y = 0;
        bool flipped = false;
    };

    Island(Catalog *catalog, Profile *profile, QObject *parent = nullptr);

    int size() const { return Size; }
    int revision() const { return m_revision; }
    QVariantList objects() const;
    QAbstractListModel *objectModel() { return &m_model; }

    static bool isBaseLand(int x, int y);
    static bool inBounds(int x, int y) { return x >= 0 && y >= 0 && x < Size && y < Size; }

    Q_INVOKABLE bool isLand(int x, int y) const;
    Q_INVOKABLE QString terrainAt(int x, int y) const; // Terrain variant id, or "".
    QString terrainKindAt(int x, int y) const;         // "sand", "grass", "boardwalk", ...
    Q_INVOKABLE int objectAt(int x, int y) const;      // uid, or 0.
    Q_INVOKABLE bool isWalkable(int x, int y) const;
    // Distance in tiles to the nearest land, capped at 3 (for shallow water).
    int shoreDistance(int x, int y) const;

    // An empty string means yes; otherwise a reason: "unknown", "none",
    // "bounds", "water", "occupied", "terrain".
    Q_INVOKABLE QString canPlace(const QString &variant, int x, int y) const;
    Q_INVOKABLE bool place(const QString &variant, int x, int y);
    Q_INVOKABLE bool move(int uid, int x, int y);
    Q_INVOKABLE bool flip(int uid);
    Q_INVOKABLE bool remove(int uid);
    Q_INVOKABLE QString removeTerrain(int x, int y); // "" or a reason.

    // "" when bought; otherwise "unknown" or "fish".
    Q_INVOKABLE QString buy(const QString &variant);
    Q_INVOKABLE int owned(const QString &variant, int revision = 0) const;
    Q_INVOKABLE int available(const QString &variant, int revision = 0) const;
    Q_INVOKABLE QVariantList inventory(const QString &tray, int revision = 0) const;

    // 4-way shortest path over walkable tiles, as [{x, y}, ...] excluding the start.
    Q_INVOKABLE QVariantList path(int fromX, int fromY, int toX, int toY) const;
    Q_INVOKABLE QVariantMap randomWalkableTile() const;
    // Where the penguin sits: next to a nest or burrow if there is one.
    Q_INVOKABLE QVariantMap restSpot() const;

    void seed(); // A fresh island for a new player.
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &object);

signals:
    void changed();
    // Only the ground (land, terrain tiles) changed; objects alone do not
    // emit this, so the tile map is not rebuilt when a tree moves.
    void groundChanged();

private:
    int placedCount(const QString &variant) const;
    Placed *findPlaced(int uid);
    void touch();
    void touchGround();

    Catalog *m_catalog;
    Profile *m_profile;
    QList<Placed> m_objects;
    QHash<int, QString> m_terrain; // key y * Size + x
    QHash<QString, int> m_owned;
    ObjectModel m_model;
    int m_nextUid = 1;
    int m_revision = 0;
};
