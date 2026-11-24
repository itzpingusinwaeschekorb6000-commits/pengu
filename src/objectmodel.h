#pragma once

#include <QAbstractListModel>
#include <QList>

class Island;

// The island's placed objects as a list model for QML. When the island
// changes it compares by uid and reports only what actually changed
// (one row moved, one added, one removed), so the view keeps every other
// object's item and sprite instead of rebuilding all of them.
class ObjectModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role { UidRole = Qt::UserRole + 1, VariantRole, TileXRole, TileYRole, FlippedRole,
                SpriteRole, WidthRole, HeightRole, GlowRole, NameRole };

    struct Row {
        int uid = 0;
        QString variant;
        int x = 0, y = 0;
        bool flipped = false;
        QString sprite;
        double width = 0, height = 0;
        bool glow = false;
        QString name;
        bool operator==(const Row &o) const {
            return uid == o.uid && variant == o.variant && x == o.x && y == o.y && flipped == o.flipped
                   && sprite == o.sprite && width == o.width && height == o.height && glow == o.glow
                   && name == o.name;
        }
    };

    explicit ObjectModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : int(m_rows.size()); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Brings the model to `rows` with the fewest changes.
    void sync(const QList<Row> &rows);

private:
    QList<Row> m_rows;
};
