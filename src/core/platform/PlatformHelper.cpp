/**
 * Location: src/core/platform/PlatformHelper.cpp
 * Function: Platform abstraction implementation isolating operating system
 * checks and desktop environment features for agnostic UI layer consumption.
 * All Rights Reserved isgaar
 */
#include "core/platform/PlatformHelper.h"
#include <QLocale>
#include <QQuickWindow>

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

namespace Dopamine::Core::Platform {

PlatformHelper &PlatformHelper::instance() {
    static PlatformHelper helper;
    return helper;
}

PlatformHelper::PlatformHelper(QObject *parent)
    : QObject(parent) {}

bool PlatformHelper::isWindows() const {
#if defined(Q_OS_WIN)
    return true;
#else
    return false;
#endif
}

bool PlatformHelper::isLinux() const {
#if defined(Q_OS_LINUX)
    return true;
#else
    return false;
#endif
}

bool PlatformHelper::isKde() const {
#if defined(Q_OS_LINUX)
    const QByteArray desk = qgetenv("XDG_CURRENT_DESKTOP").toUpper();
    return desk.contains("KDE") ||
           qEnvironmentVariableIsSet("KDE_FULL_SESSION");
#else
    return false;
#endif
}

bool PlatformHelper::useCustomTitleBar() const {
    return true;
}

bool PlatformHelper::useSystemMenuExport() const {
    return isKde();
}

QString PlatformHelper::detectHostLanguage() const {
#if defined(Q_OS_WIN)
    if (PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_SPANISH ||
        PRIMARYLANGID(GetSystemDefaultUILanguage()) == LANG_SPANISH) {
        return QStringLiteral("es");
    }
    ULONG nLangs = 0, len = 0;
    if (GetUserPreferredUILanguages(0x8, &nLangs, nullptr, &len) && len > 0) {
        std::wstring b(len, L'\0');
        if (GetUserPreferredUILanguages(0x8, &nLangs, b.data(), &len)) {
            QString s = QString::fromWCharArray(b.data(), (qsizetype)len);
            if (s.startsWith(QLatin1String("es"), Qt::CaseInsensitive))
                return QStringLiteral("es");
        }
    }
    return QStringLiteral("en");
#else
    const QString locale = QLocale::system().name().toLower();
    if (locale.startsWith("es")) return QStringLiteral("es");
    return QStringLiteral("en");
#endif
}

void PlatformHelper::setupWindow(QObject *win) {
    if (!win) return;
    auto *window = qobject_cast<QQuickWindow *>(win);
    if (!window) return;
    window->setColor(QColor(Qt::transparent));
}

} // namespace Dopamine::Core::Platform
