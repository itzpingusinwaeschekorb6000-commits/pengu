#include "catalog.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <algorithm>

namespace {
QJsonObject readObject(const QString &path, QString *error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = QStringLiteral("Cannot read %1").arg(path);
        return {};
    }
    QJsonParseError parse;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parse);
    if (parse.error != QJsonParseError::NoError || !document.isObject()) {
        *error = QStringLiteral("%1: %2").arg(path, parse.errorString());
        return {};
    }
    return document.object();
}
} // namespace

Catalog::Catalog(const QString &itemsPath, const QString &templatesPath, QObject *parent) : QObject(parent) {
    load(itemsPath, templatesPath);
}

void Catalog::load(const QString &itemsPath, const QString &templatesPath) {
    const QJsonObject root = readObject(itemsPath, &m_error);
    m_categories = root.value("categories").toArray();
    m_trays = root.value("trays").toArray();
    for (const auto &value : root.value("items").toArray()) {
        const QJsonObject object = value.toObject();
        Item item;
        item.id = object.value("id").toString();
        item.name = object.value("name");
        item.category = object.value("category").toString();
        item.tray = object.value("tray").toString();
        item.added = object.value("added").toInt();
        item.hidden = object.value("hidden").toBool();
        // A duplicate id would make purchases and saves ambiguous, so the
        // catalog refuses it loudly instead of letting one entry win.
        if (item.id.isEmpty() || m_items.contains(item.id)) {
            m_error = QStringLiteral("Duplicate or empty item id '%1'").arg(item.id);
            continue;
        }
        for (const auto &variantValue : object.value("variants").toArray()) {
            const QJsonObject v = variantValue.toObject();
            Variant variant;
            variant.id = v.value("id").toString();
            variant.itemId = item.id;
            variant.name = v.contains("name") ? v.value("name") : item.name;
            variant.price = std::max(0, v.value("price").toInt());
            variant.sprite = v.value("sprite").toString();
            variant.terrain = v.value("terrain").toString();
            const QJsonArray size = v.value("size").toArray();
            if (size.size() == 2) {
                variant.width = size.at(0).toDouble(40);
                variant.height = size.at(1).toDouble(40);
            }
            variant.solid = v.value("solid").toBool(variant.terrain.isEmpty());
            variant.glow = v.value("glow").toBool();
            variant.rest = v.value("rest").toBool();
            if (variant.id.isEmpty() || m_variants.contains(variant.id)) {
                m_error = QStringLiteral("Duplicate or empty variant id '%1'").arg(variant.id);
                continue;
            }
            m_variants.insert(variant.id, variant);
            m_variantOrder << variant.id;
            item.variants << variant.id;
        }
        if (item.variants.isEmpty())
            continue;
        m_items.insert(item.id, item);
        m_itemOrder << item.id;
    }
    m_templates = readObject(templatesPath, &m_error).value("groups").toArray();
}

void Catalog::setLanguage(const QString &language) {
    if (language == m_language)
        return;
    m_language = language;
    emit changed();
}

const Catalog::Variant *Catalog::variant(const QString &id) const {
    auto it = m_variants.constFind(id);
    return it == m_variants.constEnd() ? nullptr : &it.value();
}

const Catalog::Item *Catalog::item(const QString &id) const {
    auto it = m_items.constFind(id);
    return it == m_items.constEnd() ? nullptr : &it.value();
}

QString Catalog::text(const QJsonValue &value) const {
    if (value.isString())
        return value.toString();
    const QJsonObject object = value.toObject();
    const QString base = m_language.section('-', 0, 0);
    for (const QString &code : {m_language, base, QStringLiteral("en")})
        if (object.contains(code))
            return object.value(code).toString();
    return {};
}

int Catalog::minPrice(const Item &item) const {
    int price = INT_MAX;
    for (const auto &id : item.variants)
        price = std::min(price, m_variants.value(id).price);
    return price == INT_MAX ? 0 : price;
}

QVariantList Catalog::shopItems(const QString &search, const QString &sort, const QStringList &categories,
                                bool onlyAffordable, int fish) const {
    const QString needle = search.trimmed();
    QList<const Item *> found;
    for (const auto &id : m_itemOrder) {
        // item() returns a pointer into the hash; const operator[] would hand
        // back a temporary copy, and pointers to it would dangle.
        const Item &item = *this->item(id);
        if (item.hidden)
            continue;
        if (!categories.isEmpty() && !categories.contains(item.category))
            continue;
        if (onlyAffordable && minPrice(item) > fish)
            continue;
        if (!needle.isEmpty()) {
            // Matches the item or any variant, in the current language and in
            // English, so "banana" style searches find every related item.
            bool hit = text(item.name).contains(needle, Qt::CaseInsensitive)
                    || item.name.toObject().value("en").toString().contains(needle, Qt::CaseInsensitive);
            for (const auto &vid : item.variants)
                hit = hit || text(variant(vid)->name).contains(needle, Qt::CaseInsensitive);
            if (!hit)
                continue;
        }
        found << &item;
    }
    std::stable_sort(found.begin(), found.end(), [&](const Item *a, const Item *b) {
        if (sort == "priceLow")
            return minPrice(*a) < minPrice(*b);
        if (sort == "priceHigh")
            return minPrice(*a) > minPrice(*b);
        if (sort == "name")
            return text(a->name).localeAwareCompare(text(b->name)) < 0;
        return a->added > b->added;
    });
    QVariantList result;
    for (const Item *item : found) {
        const Variant &first = *variant(item->variants.first());
        result << QVariantMap{{"id", item->id},
                              {"name", text(item->name)},
                              {"category", item->category},
                              {"sprite", first.sprite},
                              {"terrain", first.terrain},
                              {"price", minPrice(*item)},
                              {"variantCount", item->variants.size()},
                              {"variant", first.id}};
    }
    return result;
}

QVariantMap Catalog::variantInfo(const QString &variantId) const {
    const Variant *v = variant(variantId);
    if (!v)
        return {};
    return {{"id", v->id},       {"item", v->itemId},   {"name", text(v->name)}, {"price", v->price},
            {"sprite", v->sprite}, {"terrain", v->terrain}, {"width", v->width}, {"height", v->height},
            {"solid", v->solid}, {"glow", v->glow},     {"rest", v->rest}};
}

QVariantMap Catalog::itemInfo(const QString &itemId) const {
    const Item *i = item(itemId);
    if (!i)
        return {};
    QVariantList variants;
    for (const auto &id : i->variants)
        variants << variantInfo(id);
    return {{"id", i->id}, {"name", text(i->name)}, {"category", i->category}, {"tray", i->tray},
            {"variants", variants}};
}

QVariantList Catalog::categories() const {
    QVariantList result;
    for (const auto &value : m_categories)
        result << QVariantMap{{"id", value.toObject().value("id").toString()},
                              {"name", text(value.toObject().value("name"))}};
    return result;
}

QVariantList Catalog::trayCategories() const {
    QVariantList result;
    for (const auto &value : m_trays) {
        const QJsonObject tray = value.toObject();
        result << QVariantMap{{"id", tray.value("id").toString()},
                              {"name", text(tray.value("name"))},
                              {"sprite", tray.value("sprite").toString()},
                              {"terrain", tray.value("terrain").toString()}};
    }
    return result;
}

QVariantList Catalog::templates() const {
    QVariantList groups;
    for (const auto &value : m_templates) {
        const QJsonObject group = value.toObject();
        QVariantList habits;
        for (const auto &h : group.value("habits").toArray()) {
            const QJsonObject habit = h.toObject();
            habits << QVariantMap{{"name", text(habit.value("name"))},
                                  {"description", text(habit.value("description"))},
                                  {"twoMinute", text(habit.value("twoMinute"))},
                                  {"difficulty", std::clamp(habit.value("difficulty").toInt(2), 0, 4)}};
        }
        groups << QVariantMap{{"title", text(group.value("title"))}, {"habits", habits}};
    }
    return groups;
}
