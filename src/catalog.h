#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QStringList>
#include <QVariantList>

// The shop catalog and the habit templates, both read from JSON so new items
// and templates are added as data, not code. Names are objects keyed by
// language ({"en": "...", "de": "..."}) and fall back to English.
class Catalog : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)

public:
    struct Variant {
        QString id, itemId, sprite, terrain;
        QJsonValue name;
        int price = 0;
        double width = 40, height = 40; // Sprite size in world pixels.
        bool solid = true;              // Blocks the penguin's path.
        bool glow = false;              // Casts a warm light on the ground.
        bool rest = false;              // The penguin sits here (nest, burrow).
    };
    struct Item {
        QString id, category, tray;
        QJsonValue name;
        int added = 0; // Higher is newer, for "Sort: Newest".
        bool hidden = false;
        QStringList variants;
    };

    explicit Catalog(const QString &itemsPath = ":/data/items.json",
                     const QString &templatesPath = ":/data/templates.json", QObject *parent = nullptr);

    QString language() const { return m_language; }
    void setLanguage(const QString &language);
    QString error() const { return m_error; }

    const Variant *variant(const QString &id) const;
    const Item *item(const QString &id) const;
    QStringList variantIds() const { return m_variantOrder; }
    QString text(const QJsonValue &value) const;

    // Shop grid. sort: "newest", "priceLow", "priceHigh", "name".
    Q_INVOKABLE QVariantList shopItems(const QString &search, const QString &sort,
                                       const QStringList &categories, bool onlyAffordable, int fish) const;
    Q_INVOKABLE QVariantMap itemInfo(const QString &itemId) const;
    Q_INVOKABLE QVariantMap variantInfo(const QString &variantId) const;
    Q_INVOKABLE QVariantList categories() const;     // Shop filter categories.
    Q_INVOKABLE QVariantList trayCategories() const; // Build-mode tray.
    Q_INVOKABLE QVariantList templates() const;

signals:
    void changed();

private:
    void load(const QString &itemsPath, const QString &templatesPath);
    int minPrice(const Item &item) const;

    QString m_language = "en";
    QString m_error;
    QHash<QString, Variant> m_variants;
    QHash<QString, Item> m_items;
    QStringList m_itemOrder, m_variantOrder;
    QJsonArray m_categories, m_trays, m_templates;
};
