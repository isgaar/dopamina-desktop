/**
 * Location: src/shared/Theme.cpp
 * Function: Implementation of reactive design tokens bridged to the XML
 * palette catalog and the localized string manager.
 * All Rights Reserved isgaar
 */
#include "Theme.h"
#include "colors/ColorManager.h"
#include "i18n/StringManager.h"

namespace Dopamine::Shared {

Theme &Theme::instance() {
    static Theme s_instance;
    return s_instance;
}

Theme::Theme(QObject *parent)
    : QObject(parent) {
    auto &colors = Colors::ColorManager::instance();
    connect(&colors, &Colors::ColorManager::paletteChanged,
            this, &Theme::paletteChanged);
    auto &strings = I18n::StringManager::instance();
    connect(&strings, &I18n::StringManager::localeChanged,
            this, &Theme::languageChanged);
}

QString Theme::currentTheme() const {
    return Colors::ColorManager::instance().currentTheme();
}

bool Theme::isSpanish() const {
    return I18n::StringManager::instance().currentLocale()
        .startsWith(QStringLiteral("es"));
}

QColor Theme::color(const QString &key) const {
    return Colors::ColorManager::instance().get(key);
}

QColor Theme::primary() const {
    return color(QStringLiteral("primary"));
}
QColor Theme::primaryHover() const {
    return color(QStringLiteral("primary_hover"));
}
QColor Theme::primaryDim() const {
    return color(QStringLiteral("primary_dim"));
}
QColor Theme::accent() const {
    return color(QStringLiteral("accent"));
}
QColor Theme::accentLight() const {
    return color(QStringLiteral("accent_light"));
}
QColor Theme::accentOrange() const {
    return color(QStringLiteral("accent_orange"));
}

QColor Theme::bgDarkest() const {
    return color(QStringLiteral("bg_darkest"));
}
QColor Theme::bgDark() const {
    return color(QStringLiteral("bg_dark"));
}
QColor Theme::bgSurface() const {
    return color(QStringLiteral("bg_surface"));
}
QColor Theme::bgCard() const {
    return color(QStringLiteral("bg_card"));
}
QColor Theme::bgSelected() const {
    return color(QStringLiteral("bg_selected"));
}
QColor Theme::borderDark() const {
    return color(QStringLiteral("border_dark"));
}
QColor Theme::borderHighlight() const {
    return color(QStringLiteral("border_highlight"));
}

QColor Theme::textLight() const {
    return color(QStringLiteral("text_light"));
}
QColor Theme::textMuted() const {
    return color(QStringLiteral("text_muted"));
}
QColor Theme::textDim() const {
    return color(QStringLiteral("text_dim"));
}

QColor Theme::success() const {
    return color(QStringLiteral("success"));
}
QColor Theme::danger() const {
    return color(QStringLiteral("danger"));
}
QColor Theme::warning() const {
    return color(QStringLiteral("warning"));
}

QString Theme::string(const QString &key) const {
    return I18n::StringManager::instance().get(key);
}

QString Theme::formatDuration(double seconds) const {
    int total = static_cast<int>(seconds);
    int m = total / 60;
    int s = total % 60;
    int h = m / 60;
    m %= 60;
    if (h > 0) {
        return QString("%1:%2:%3")
            .arg(h)
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'));
    }
    return QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0'));
}

} // namespace Dopamine::Shared
