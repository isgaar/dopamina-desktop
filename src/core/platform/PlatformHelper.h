/**
 * Location: src/core/platform/PlatformHelper.h
 * Function: Platform abstraction service providing host operating system
 * detection, title bar customization flags, and desktop environment metadata.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_PLATFORM_PLATFORMHELPER_H
#define DOPAMINE_CORE_PLATFORM_PLATFORMHELPER_H

#include <QObject>

namespace Dopamine::Core::Platform {

class PlatformHelper : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isWindows READ isWindows CONSTANT)
    Q_PROPERTY(bool isLinux READ isLinux CONSTANT)
    Q_PROPERTY(bool isKde READ isKde CONSTANT)
    Q_PROPERTY(bool useCustomTitleBar READ useCustomTitleBar CONSTANT)
    Q_PROPERTY(bool useSystemMenuExport READ useSystemMenuExport CONSTANT)

public:
    static PlatformHelper &instance();
    explicit PlatformHelper(QObject *parent = nullptr);

    bool isWindows() const;
    bool isLinux() const;
    bool isKde() const;
    bool useCustomTitleBar() const;
    bool useSystemMenuExport() const;
    Q_INVOKABLE QString detectHostLanguage() const;
    Q_INVOKABLE void setupWindow(QObject *win);
};

} // namespace Dopamine::Core::Platform

#endif // DOPAMINE_CORE_PLATFORM_PLATFORMHELPER_H
