/**
 * Location: src/features/settings/app/SettingsWindowCapturer.cpp
 * Function: Implementation of offscreen snapshot capture utility for
 * SettingsWindow panels and in-window modal confirmation dialogs.
 * All Rights Reserved isgaar
 */
#include "SettingsWindowCapturer.h"
#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QThread>

namespace Dopamine::Features::Settings::App {

bool captureSettingsPanels(QQmlApplicationEngine *engine,
                           const QString &outputDir) {
    QDir().mkpath(outputDir);
    QQuickWindow *win = nullptr;
    if (!engine->rootObjects().isEmpty()) {
        win = engine->rootObjects().first()->findChild<QQuickWindow *>(
            QStringLiteral("settingsWindow"));
    }
    if (!win) {
        for (QWindow *w : QGuiApplication::allWindows()) {
            if (w->objectName() == QStringLiteral("settingsWindow")) {
                win = qobject_cast<QQuickWindow *>(w);
                break;
            }
        }
    }
    if (!win) return false;

    win->show();
    for (int i = 0; i < 15; ++i) {
        QCoreApplication::processEvents();
        QThread::msleep(20);
    }

    auto *stack = win->findChild<QQuickItem *>(QStringLiteral("settingsStack"));

    auto cap = [&](int idx, const QString &filename) {
        if (stack) stack->setProperty("currentIndex", idx);
        for (int i = 0; i < 8; ++i) {
            QCoreApplication::processEvents();
            QThread::msleep(20);
        }
        QImage img = win->grabWindow();
        return img.save(outputDir + QLatin1Char('/') + filename);
    };

    cap(0, QStringLiteral("settings_server.png"));
    cap(1, QStringLiteral("settings_appearance.png"));
    cap(2, QStringLiteral("settings_about.png"));

    QObject *ctrl = qvariant_cast<QObject *>(
        engine->rootContext()->contextProperty(
            QStringLiteral("settingsController")));
    if (ctrl) {
        if (stack) stack->setProperty("currentIndex", 0);
        ctrl->setProperty("draftApiUrl",
            QStringLiteral("http://192.168.1.100:8000"));
        for (int i = 0; i < 6; ++i) {
            QCoreApplication::processEvents();
            QThread::msleep(20);
        }
        win->grabWindow().save(outputDir +
            QStringLiteral("/settings_server_dirty.png"));
    }

    win->setProperty("showConfirm", true);
    for (int i = 0; i < 8; ++i) {
        QCoreApplication::processEvents();
        QThread::msleep(20);
    }
    win->grabWindow().save(outputDir + QStringLiteral("/settings_confirm.png"));

    return true;
}

} // namespace Dopamine::Features::Settings::App
