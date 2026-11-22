#include "tilemap.h"

#include <QPainter>
#include <cmath>

namespace {
double hash(int x, int y) {
    quint32 h = quint32(x + 101) * 2654435761u ^ quint32(y + 7) * 2246822519u;
    h ^= h >> 15;
    h *= 2246822519u;
    h ^= h >> 13;
    return double(h % 1000) / 1000.0;
}

QColor terrainColor(const QString &kind, double n) {
    if (kind == "grass")
        return QColor::fromHsvF(0.24, 0.38 + n * 0.06, 0.78 + n * 0.04);
    if (kind == "boardwalk")
        return QColor("#b98a5a");
    if (kind == "straw")
        return QColor("#dcc06a");
    if (kind == "granite")
        return QColor::fromHsvF(0.08, 0.08, 0.72 + n * 0.05);
    // Pale Cape sand, with a little per-tile variation.
    return QColor::fromHsvF(0.12, 0.20 + n * 0.05, 0.94 - n * 0.03);
}
} // namespace

TileMap::TileMap(QQuickItem *parent) : QQuickPaintedItem(parent) {
    setAntialiasing(true);
    setImplicitWidth(Island::Size * TileWidth);
    setImplicitHeight(Island::Size * TileHeight + 16);
}

void TileMap::setIsland(Island *island) {
    if (m_island == island)
        return;
    if (m_island)
        disconnect(m_island, nullptr, this, nullptr);
    m_island = island;
    if (m_island)
        connect(m_island, &Island::changed, this, [this] { update(); });
    emit islandChanged();
    update();
}

void TileMap::setShowGrid(bool on) {
    if (on == m_showGrid)
        return;
    m_showGrid = on;
    emit islandChanged();
    update();
}

void TileMap::setHighlight(const QPoint &tile) {
    if (tile == m_highlight)
        return;
    m_highlight = tile;
    emit islandChanged();
    update();
}

void TileMap::setHighlightOk(bool ok) {
    if (ok == m_highlightOk)
        return;
    m_highlightOk = ok;
    emit islandChanged();
    update();
}

QPointF TileMap::tileCenter(qreal x, qreal y) const {
    const qreal originX = Island::Size * TileWidth / 2.0;
    return {originX + (x - y) * TileWidth / 2.0, (x + y) * TileHeight / 2.0 + TileHeight / 2.0};
}

QPoint TileMap::tileAt(qreal px, qreal py) const {
    const qreal originX = Island::Size * TileWidth / 2.0;
    const qreal a = (px - originX) / (TileWidth / 2.0);
    const qreal b = py / (TileHeight / 2.0);
    const int x = int(std::floor((a + b) / 2.0));
    const int y = int(std::floor((b - a) / 2.0));
    return Island::inBounds(x, y) ? QPoint(x, y) : QPoint(-1, -1);
}

QPolygonF TileMap::diamond(int x, int y) const {
    const QPointF c = tileCenter(x, y);
    return QPolygonF{QPointF(c.x(), c.y() - TileHeight / 2.0), QPointF(c.x() + TileWidth / 2.0, c.y()),
                     QPointF(c.x(), c.y() + TileHeight / 2.0), QPointF(c.x() - TileWidth / 2.0, c.y())};
}

void TileMap::paint(QPainter *p) {
    if (!m_island)
        return;
    p->setRenderHint(QPainter::Antialiasing, true);
    const int n = Island::Size;
    static const QColor deep[3] = {QColor("#2b4fbf"), QColor("#3157c9"), QColor("#3962d6")};
    static const QColor shallow[3] = {QColor("#79b4e0"), QColor("#5a8fd8"), QColor("#4370cf")};
    const QPen waterLine(QColor(15, 30, 90, 70), 1);
    const QPen landLine(QColor(110, 90, 50, 60), 1);

    // Back to front so cliff edges overlap correctly.
    for (int sum = 0; sum <= 2 * (n - 1); ++sum) {
        for (int x = 0; x < n; ++x) {
            const int y = sum - x;
            if (y < 0 || y >= n)
                continue;
            const QPolygonF shape = diamond(x, y);
            const double h = hash(x, y);
            if (m_island->isLand(x, y)) {
                const QColor top = terrainColor(m_island->terrainKindAt(x, y), h);
                p->setPen(m_showGrid ? landLine : Qt::NoPen);
                p->setBrush(top);
                p->drawPolygon(shape);
                if (m_island->terrainKindAt(x, y) == "boardwalk") {
                    // Planks across the tile.
                    p->setPen(QPen(QColor(90, 60, 35, 150), 1));
                    for (int i = 1; i < 4; ++i) {
                        const qreal t = i / 4.0;
                        p->drawLine(shape[0] * (1 - t) + shape[3] * t, shape[1] * (1 - t) + shape[2] * t);
                    }
                }
            } else {
                const int d = m_island->shoreDistance(x, y);
                const int shade = h < 0.33 ? 0 : h < 0.75 ? 1 : 2;
                QColor c = d <= 1 ? shallow[0] : d == 2 ? shallow[1] : deep[shade];
                // A loose checker like the original's ocean.
                if (d >= 3 && ((x / 2 + y / 2) % 2 == 0))
                    c = c.lighter(106);
                p->setPen(m_showGrid ? waterLine : Qt::NoPen);
                p->setBrush(c);
                p->drawPolygon(shape);
            }
        }
    }

    // A low cliff toward the viewer where land meets water, drawn after all
    // tiles so the water in front does not paint over it.
    for (int sum = 0; sum <= 2 * (n - 1); ++sum) {
        for (int x = 0; x < n; ++x) {
            const int y = sum - x;
            if (y < 0 || y >= n || !m_island->isLand(x, y))
                continue;
            const bool right = !m_island->isLand(x + 1, y), left = !m_island->isLand(x, y + 1);
            if (!right && !left)
                continue;
            const QPolygonF shape = diamond(x, y);
            const QColor side = terrainColor(m_island->terrainKindAt(x, y), hash(x, y)).darker(140);
            const qreal depth = 7;
            p->setPen(QPen(QColor(60, 50, 30, 150), 1));
            p->setBrush(side);
            if (right)
                p->drawPolygon(QPolygonF{shape[1], shape[2], shape[2] + QPointF(0, depth), shape[1] + QPointF(0, depth)});
            if (left)
                p->drawPolygon(QPolygonF{shape[3], shape[2], shape[2] + QPointF(0, depth), shape[3] + QPointF(0, depth)});
        }
    }

    if (Island::inBounds(m_highlight.x(), m_highlight.y())) {
        const QColor c = m_highlightOk ? QColor(120, 230, 120, 120) : QColor(240, 80, 80, 120);
        p->setPen(QPen(c.darker(140), 2));
        p->setBrush(c);
        p->drawPolygon(diamond(m_highlight.x(), m_highlight.y()));
    }
}
