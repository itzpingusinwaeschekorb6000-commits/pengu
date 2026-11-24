// pengu — a habit tracker where every finished habit earns sardines for an
// African penguin and its island.

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QSurfaceFormat>
#include <QUrl>

#include "game.h"
#include "theme.h"
#include "tilemap.h"
#include "translator.h"
#include "updater.h"

int main(int argc, char *argv[]) {
    // The island ground is drawn as GPU triangles; multisampling smooths
    // their diagonal edges.
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    QGuiApplication app(argc, argv);
    app.setApplicationName("pengu");
    app.setApplicationVersion(PENGU_VERSION);

    // Matches the window to pengu.desktop, so the compositor (Wayland app_id)
    // and the launcher pick up the installed icon.
    app.setDesktopFileName("pengu");
    app.setWindowIcon(QIcon::fromTheme("pengu", QIcon(":/sprites/penguin_head.svg")));

    QQuickStyle::setStyle("Material");

    // Installed before anything is translated, and every qsTr() binding is
    // re-evaluated when the Omarchy language changes.
    Translator translator;
    app.installTranslator(&translator);

    Theme theme;
    // A save path argument makes it easy to try things on a scratch island.
    Game game(app.arguments().size() > 1 ? app.arguments().at(1) : QString());
    game.catalog()->setLanguage(translator.language());
    QObject::connect(&translator, &Translator::changed, game.catalog(),
                     [&] { game.catalog()->setLanguage(translator.language()); });
    game.load();
    Updater updater;

    qmlRegisterType<TileMap>("Pengu", 1, 0, "TileMap");
    qmlRegisterUncreatableType<Island>("Pengu", 1, 0, "Island", "Provided by the game");

    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    auto *context = engine.rootContext();
    context->setContextProperty("translator", &translator);
    context->setContextProperty("theme", &theme);
    context->setContextProperty("game", &game);
    context->setContextProperty("catalog", game.catalog());
    context->setContextProperty("profile", game.profile());
    context->setContextProperty("habits", game.habits());
    context->setContextProperty("island", game.island());
    context->setContextProperty("reminders", game.reminders());
    context->setContextProperty("updater", &updater);
    engine.load(QUrl("qrc:/qml/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;

    const int code = app.exec();
    game.save();
    return code;
}
