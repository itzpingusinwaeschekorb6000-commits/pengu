// Offscreen screenshot harness: loads the real QML against a scratch save,
// runs JavaScript steps on the window and saves what the user would see.
// Usage: shot <save.json> <out.png> [width height] [js step]... ; a step of
// "wait:<ms>" pauses, "png:<file>" grabs an extra picture.

#include <QElapsedTimer>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QJSValue>
#include <QTimer>

#include "game.h"
#include "theme.h"
#include "tilemap.h"
#include "translator.h"

static void pump(int ms) {
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
}

int main(int argc, char *argv[]) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);
    const QStringList args = app.arguments();
    if (args.size() < 3)
        return 2;
    QQuickStyle::setStyle("Material");
    Translator translator;
    app.installTranslator(&translator);
    Theme theme(qEnvironmentVariable("PENGU_THEME_DIR", "/nonexistent"));
    Game game(args.at(1));
    game.catalog()->setLanguage(translator.language());
    game.load();
    qmlRegisterType<TileMap>("Pengu", 1, 0, "TileMap");
    qmlRegisterUncreatableType<Island>("Pengu", 1, 0, "Island", "Provided by the game");
    QQmlApplicationEngine engine;
    auto *c = engine.rootContext();
    c->setContextProperty("translator", &translator);
    c->setContextProperty("theme", &theme);
    c->setContextProperty("game", &game);
    c->setContextProperty("catalog", game.catalog());
    c->setContextProperty("profile", game.profile());
    c->setContextProperty("habits", game.habits());
    c->setContextProperty("island", game.island());
    c->setContextProperty("reminders", game.reminders());
    engine.load(QUrl("qrc:/qml/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    // Steps see the window (and its QML functions) and the game objects.
    for (QObject *o : {static_cast<QObject *>(window), static_cast<QObject *>(game.habits()),
                       static_cast<QObject *>(game.island()), static_cast<QObject *>(game.profile()),
                       static_cast<QObject *>(window->contentItem())})
        QQmlEngine::setObjectOwnership(o, QQmlEngine::CppOwnership);
    QJSValue global = engine.globalObject();
    global.setProperty("window", engine.newQObject(window));
    global.setProperty("habits", engine.newQObject(game.habits()));
    global.setProperty("island", engine.newQObject(game.island()));
    global.setProperty("profile", engine.newQObject(game.profile()));
    global.setProperty("root", engine.newQObject(window->contentItem()));
    int i = 3;
    if (args.size() > 4 && args.at(3).toInt() > 0) {
        window->resize(args.at(3).toInt(), args.at(4).toInt());
        i = 5;
    }
    pump(1200);
    for (; i < args.size(); ++i) {
        const QString step = args.at(i);
        if (step.startsWith("wait:")) {
            pump(step.mid(5).toInt());
        } else if (step.startsWith("png:")) {
            window->grabWindow().save(step.mid(4));
        } else {
            const QJSValue result = engine.evaluate(step);
            if (result.isError())
                qWarning("step failed: %s: %s", qPrintable(step), qPrintable(result.toString()));
            pump(300);
        }
    }
    pump(400);
    window->grabWindow().save(args.at(2));
    game.save();
    return 0;
}
