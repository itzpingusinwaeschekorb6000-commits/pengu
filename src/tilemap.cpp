#include "tilemap.h"

#include <QPainter>
#include <QQuickWindow>
#include <QSGGeometryNode>
#include <QSGImageNode>
#include <QSGRendererInterface>
#include <QSGVertexColorMaterial>
#include <cmath>
#include <vector>

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

// Colored triangles, in painting order (later ones cover earlier ones).
class TileTriangles {
public:
    void triangle(QPointF a, QPointF b, QPointF c, const QColor &color) {
        // The vertex color material expects premultiplied alpha.
        const float alpha = float(color.alphaF());
        const auto r = uchar(color.red() * alpha), g = uchar(color.green() * alpha),
                   bl = uchar(color.blue() * alpha), al = uchar(color.alpha());
        for (const QPointF &p : {a, b, c}) {
            QSGGeometry::ColoredPoint2D v;
            v.set(float(p.x()), float(p.y()), r, g, bl, al);
            m_vertices.push_back(v);
        }
    }
    void quad(const QPolygonF &q, const QColor &color) {
        triangle(q[0], q[1], q[2], color);
        triangle(q[0], q[2], q[3], color);
    }
    // A line as a thin quad in world units, so it scales with the zoom like
    // the rest of the island.
    void line(QPointF a, QPointF b, qreal width, const QColor &color) {
        const QPointF d = b - a;
        const qreal length = std::hypot(d.x(), d.y());
        if (length <= 0)
            return;
        const QPointF n(-d.y() / length * width / 2, d.x() / length * width / 2);
        quad(QPolygonF{a + n, b + n, b - n, a - n}, color);
    }
    void outline(const QPolygonF &shape, qreal width, const QColor &color) {
        for (int i = 0; i < shape.size(); ++i)
            line(shape[i], shape[(i + 1) % shape.size()], width, color);
    }

    int count() const { return int(m_vertices.size() / 3); }

    // Software fallback: the same triangles with QPainter.
    void paint(QPainter *p) const {
        for (size_t i = 0; i + 2 < m_vertices.size(); i += 3) {
            const auto &v = m_vertices[i];
            const qreal a = v.a / 255.0;
            const QColor c = a > 0 ? QColor(int(v.r / a), int(v.g / a), int(v.b / a), v.a) : QColor(0, 0, 0, 0);
            const QPointF pts[3] = {{m_vertices[i].x, m_vertices[i].y},
                                    {m_vertices[i + 1].x, m_vertices[i + 1].y},
                                    {m_vertices[i + 2].x, m_vertices[i + 2].y}};
            p->setBrush(c);
            p->drawPolygon(pts, 3);
        }
    }

    void upload(QSGGeometryNode *node) const {
        auto *geometry = node->geometry();
        geometry->allocate(int(m_vertices.size()));
        std::copy(m_vertices.begin(), m_vertices.end(), geometry->vertexDataAsColoredPoint2D());
        geometry->markVertexDataDirty();
        node->markDirty(QSGNode::DirtyGeometry);
    }

private:
    std::vector<QSGGeometry::ColoredPoint2D> m_vertices;
};

namespace {
QSGGeometryNode *coloredNode() {
    auto *node = new QSGGeometryNode;
    auto *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), 0);
    geometry->setDrawingMode(QSGGeometry::DrawTriangles);
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);
    node->setMaterial(new QSGVertexColorMaterial);
    node->setFlag(QSGNode::OwnsMaterial);
    return node;
}
} // namespace

TileMap::TileMap(QQuickItem *parent) : QQuickItem(parent) {
    setFlag(ItemHasContents, true);
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
        connect(m_island, &Island::groundChanged, this, &TileMap::markGroundDirty);
    emit islandChanged();
    markGroundDirty();
}

void TileMap::markGroundDirty() {
    m_groundDirty = true;
    update();
}

void TileMap::setShowGrid(bool on) {
    if (on == m_showGrid)
        return;
    m_showGrid = on;
    emit showGridChanged();
    markGroundDirty();
}

void TileMap::setHighlight(const QPoint &tile) {
    if (tile == m_highlight)
        return;
    m_highlight = tile;
    m_highlightDirty = true;
    emit highlightChanged();
    update();
}

void TileMap::setHighlightOk(bool ok) {
    if (ok == m_highlightOk)
        return;
    m_highlightOk = ok;
    m_highlightDirty = true;
    emit highlightChanged();
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

QSGNode *TileMap::updatePaintNode(QSGNode *old, UpdatePaintNodeData *) {
    // Qt's software renderer (no GPU) cannot draw geometry nodes: paint the
    // same triangles into an image instead.
    if (window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software) {
        auto *node = static_cast<QSGImageNode *>(old);
        if (!node)
            node = window()->createImageNode();
        if (!old || m_groundDirty || m_highlightDirty) {
            constexpr qreal Scale = 2;
            QImage image((boundingRect().size() * Scale).toSize(), QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            QPainter painter(&image);
            painter.scale(Scale, Scale);
            painter.setPen(Qt::NoPen);
            TileTriangles ground, highlight;
            buildGround(ground);
            buildHighlight(highlight);
            ground.paint(&painter);
            highlight.paint(&painter);
            painter.end();
            node->setTexture(window()->createTextureFromImage(image));
            node->setOwnsTexture(true);
            node->setRect(boundingRect());
            m_groundDirty = m_highlightDirty = false;
        }
        return node;
    }

    // Root with two children: the ground, then the highlight on top.
    QSGNode *root = old;
    if (!root) {
        root = new QSGNode;
        root->appendChildNode(coloredNode());
        root->appendChildNode(coloredNode());
        m_groundDirty = m_highlightDirty = true;
    }
    auto *ground = static_cast<QSGGeometryNode *>(root->firstChild());
    auto *highlight = static_cast<QSGGeometryNode *>(root->lastChild());
    if (m_groundDirty) {
        TileTriangles t;
        buildGround(t);
        t.upload(ground);
        m_groundDirty = false;
    }
    if (m_highlightDirty) {
        TileTriangles t;
        buildHighlight(t);
        t.upload(highlight);
        m_highlightDirty = false;
    }
    return root;
}

void TileMap::buildGround(TileTriangles &t) {
    if (m_island) {
        const int n = Island::Size;
        static const QColor deep[3] = {QColor("#2b4fbf"), QColor("#3157c9"), QColor("#3962d6")};
        static const QColor shallow[3] = {QColor("#79b4e0"), QColor("#5a8fd8"), QColor("#4370cf")};
        const QColor waterLine(15, 30, 90, 70);
        const QColor landLine(110, 90, 50, 60);
        const QColor plank(90, 60, 35, 150);

        // Back to front so cliff edges overlap correctly.
        for (int sum = 0; sum <= 2 * (n - 1); ++sum) {
            for (int x = 0; x < n; ++x) {
                const int y = sum - x;
                if (y < 0 || y >= n)
                    continue;
                const QPolygonF shape = diamond(x, y);
                const double h = hash(x, y);
                if (m_island->isLand(x, y)) {
                    const QString kind = m_island->terrainKindAt(x, y);
                    t.quad(shape, terrainColor(kind, h));
                    if (m_showGrid)
                        t.outline(shape, 1, landLine);
                    if (kind == "boardwalk") {
                        // Planks across the tile.
                        for (int i = 1; i < 4; ++i) {
                            const qreal f = i / 4.0;
                            t.line(shape[0] * (1 - f) + shape[3] * f, shape[1] * (1 - f) + shape[2] * f, 1, plank);
                        }
                    }
                } else {
                    const int d = m_island->shoreDistance(x, y);
                    const int shade = h < 0.33 ? 0 : h < 0.75 ? 1 : 2;
                    QColor c = d <= 1 ? shallow[0] : d == 2 ? shallow[1] : deep[shade];
                    // A loose checker like the original's ocean.
                    if (d >= 3 && ((x / 2 + y / 2) % 2 == 0))
                        c = c.lighter(106);
                    t.quad(shape, c);
                    if (m_showGrid)
                        t.outline(shape, 1, waterLine);
                }
            }
        }

        // A low cliff toward the viewer where land meets water, drawn after
        // all tiles so the water in front does not cover it.
        const QColor edge(60, 50, 30, 150);
        const qreal depth = 7;
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
                const QPointF down(0, depth);
                for (const auto &face : {right ? QPolygonF{shape[1], shape[2], shape[2] + down, shape[1] + down} : QPolygonF(),
                                         left ? QPolygonF{shape[3], shape[2], shape[2] + down, shape[3] + down} : QPolygonF()}) {
                    if (face.isEmpty())
                        continue;
                    t.quad(face, side);
                    t.outline(face, 1, edge);
                }
            }
        }
    }
    m_groundTriangles = t.count();
}

void TileMap::buildHighlight(TileTriangles &t) const {
    if (Island::inBounds(m_highlight.x(), m_highlight.y())) {
        const QColor c = m_highlightOk ? QColor(120, 230, 120, 120) : QColor(240, 80, 80, 120);
        const QPolygonF shape = diamond(m_highlight.x(), m_highlight.y());
        t.quad(shape, c);
        t.outline(shape, 2, c.darker(140));
    }
}
