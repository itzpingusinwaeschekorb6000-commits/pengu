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
#include <QMouseEvent>
#include <QPointingDevice>

#include "game.h"
#include "theme.h"
#include "tilemap.h"
#include "translator.h"
#include "updater.h"

static void pump(int ms);

static void mouse(QQuickWindow *w, QEvent::Type type, QPointF pos, Qt::MouseButton button, Qt::MouseButtons buttons) {
    QMouseEvent event(type, pos, w->mapToGlobal(pos), button, buttons, Qt::NoModifier);
    QGuiApplication::sendEvent(w, &event);
}

// Presses at a, moves in small steps to b, releases there.
static void drag(QQuickWindow *w, QPointF a, QPointF b, Qt::MouseButton button = Qt::LeftButton) {
    mouse(w, QEvent::MouseButtonPress, a, button, button);
    pump(30);
    for (int i = 1; i <= 12; ++i) {
        mouse(w, QEvent::MouseMove, a + (b - a) * (i / 12.0), Qt::NoButton, button);
        pump(15);
    }
    mouse(w, QEvent::MouseButtonRelease, b, button, Qt::NoButton);
    pump(50);
}

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
    // Not configured, so it never goes online; steps can still set its state.
    Updater updater(nullptr, QString());
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
    c->setContextProperty("updater", &updater);
    // PENGU_UPDATE_PREVIEW=available:0.2.99[:0.4] shows the update banner.
    if (const QStringList p = qEnvironmentVariable("PENGU_UPDATE_PREVIEW").split(':'); p.size() >= 2)
        updater.preview(p.at(0), p.at(1), p.value(2).toDouble());
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
        } else if (step.startsWith("click:") || step.startsWith("rclick:")) {
            const QStringList n = step.section(':', 1).split(',');
            const QPointF at(n.value(0).toDouble(), n.value(1).toDouble());
            drag(window, at, at, step.startsWith("r") ? Qt::RightButton : Qt::LeftButton);
            pump(250);
        } else if (step.startsWith("drag:")) {
            const QStringList n = step.mid(5).split(',');
            drag(window, QPointF(n.value(0).toDouble(), n.value(1).toDouble()),
                 QPointF(n.value(2).toDouble(), n.value(3).toDouble()));
            pump(300);
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
