#pragma once

#include <QPointer>
#include <QQuickPaintedItem>

#include "island.h"

// Paints the isometric ground: open water, shallow water along the coast,
// sand, terrain tiles and the little cliff edge where land meets the sea.
// Objects and the penguin are QML items stacked on top of it.
class TileMap : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(Island *world READ island WRITE setIsland NOTIFY islandChanged)
    Q_PROPERTY(int tileWidth MEMBER m_tileWidth CONSTANT)
    Q_PROPERTY(int tileHeight MEMBER m_tileHeight CONSTANT)
    Q_PROPERTY(bool showGrid READ showGrid WRITE setShowGrid NOTIFY islandChanged)
    Q_PROPERTY(QPoint highlight READ highlight WRITE setHighlight NOTIFY islandChanged)
    Q_PROPERTY(bool highlightOk READ highlightOk WRITE setHighlightOk NOTIFY islandChanged)

public:
    static constexpr int TileWidth = 64;
    static constexpr int TileHeight = 32;

    explicit TileMap(QQuickItem *parent = nullptr);

    Island *island() const { return m_island; }
    void setIsland(Island *island);
    bool showGrid() const { return m_showGrid; }
    void setShowGrid(bool on);
    QPoint highlight() const { return m_highlight; }
    void setHighlight(const QPoint &tile);
    bool highlightOk() const { return m_highlightOk; }
    void setHighlightOk(bool ok);

    // Tile under a point in this item's coordinates, or (-1, -1).
    Q_INVOKABLE QPoint tileAt(qreal x, qreal y) const;
    // Center of a tile's top face in this item's coordinates.
    Q_INVOKABLE QPointF tileCenter(qreal x, qreal y) const;

    void paint(QPainter *painter) override;

signals:
    void islandChanged();

private:
    QPolygonF diamond(int x, int y) const;

    QPointer<Island> m_island;
    int m_tileWidth = TileWidth;
    int m_tileHeight = TileHeight;
    bool m_showGrid = true;
    QPoint m_highlight{-1, -1};
    bool m_highlightOk = true;
};
