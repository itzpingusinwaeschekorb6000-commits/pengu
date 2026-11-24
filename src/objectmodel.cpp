#include "objectmodel.h"

#include <QHash>
#include <QSet>

QVariant ObjectModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const Row &r = m_rows.at(index.row());
    switch (role) {
    case UidRole: return r.uid;
    case VariantRole: return r.variant;
    case TileXRole: return r.x;
    case TileYRole: return r.y;
    case FlippedRole: return r.flipped;
    case SpriteRole: return r.sprite;
    case WidthRole: return r.width;
    case HeightRole: return r.height;
    case GlowRole: return r.glow;
    case NameRole: return r.name;
    }
    return {};
}

QHash<int, QByteArray> ObjectModel::roleNames() const {
    // tileX/tileY rather than x/y, which QML items already have.
    return {{UidRole, "uid"},         {VariantRole, "variant"}, {TileXRole, "tileX"},
            {TileYRole, "tileY"},     {FlippedRole, "flipped"}, {SpriteRole, "sprite"},
            {WidthRole, "spriteWidth"}, {HeightRole, "spriteHeight"}, {GlowRole, "glow"},
            {NameRole, "name"}};
}

void ObjectModel::sync(const QList<Row> &rows) {
    QHash<int, int> wanted; // uid -> index in rows
    for (int i = 0; i < rows.size(); ++i)
        wanted.insert(rows.at(i).uid, i);

    // Gone: remove from the back so indexes stay valid.
    for (int i = int(m_rows.size()) - 1; i >= 0; --i) {
        if (wanted.contains(m_rows.at(i).uid))
            continue;
        beginRemoveRows({}, i, i);
        m_rows.removeAt(i);
        endRemoveRows();
    }

    // Changed: update in place.
    QSet<int> present;
    for (int i = 0; i < m_rows.size(); ++i) {
        const Row &next = rows.at(wanted.value(m_rows.at(i).uid));
        present.insert(next.uid);
        if (next == m_rows.at(i))
            continue;
        m_rows[i] = next;
        emit dataChanged(index(i), index(i));
    }

    // New: append.
    for (const Row &r : rows) {
        if (present.contains(r.uid))
            continue;
        beginInsertRows({}, int(m_rows.size()), int(m_rows.size()));
        m_rows.append(r);
        endInsertRows();
    }
}
