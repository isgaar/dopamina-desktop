/**
 * Location: src/shared/colors/ColorManager.cpp
 * Function: Implementation of the XML palette catalog supporting the five
 * Android themes, cached previews, and live palette notifications.
 * All Rights Reserved isgaar
 */
#include "ColorManager.h"
#include <QFile>
#include <QXmlStreamReader>

namespace Dopamine::Shared::Colors {

ColorManager &ColorManager::instance() {
    static ColorManager s_instance;
    return s_instance;
}

ColorManager::ColorManager(QObject *parent)
    : QObject(parent) {}

QStringList ColorManager::themeIds() {
    return {QStringLiteral("roseglow"), QStringLiteral("exoblur"),
            QStringLiteral("thegrand"), QStringLiteral("redline"),
            QStringLiteral("flat")};
}

bool ColorManager::initialize(const QString &customPath) {
    if (!customPath.isEmpty()) {
        return loadPalette(customPath);
    }
    return setTheme(QStringLiteral("roseglow"));
}

bool ColorManager::setTheme(const QString &themeId) {
    const QString id = themeId.trimmed().toLower();
    if (!themeIds().contains(id)) {
        return false;
    }
    if (id == m_theme && !m_colors.isEmpty()) {
        return true;
    }
    if (!m_cache.contains(id)) {
        QHash<QString, QColor> palette;
        if (!loadInto(themePath(id), palette)) {
            return false;
        }
        m_cache.insert(id, palette);
    }
    m_colors = m_cache.value(id);
    m_theme = id;
    emit paletteChanged();
    return true;
}

QColor ColorManager::paletteColor(const QString &themeId,
                                  const QString &name) {
    const QString id = themeId.trimmed().toLower();
    if (!themeIds().contains(id)) {
        return {};
    }
    if (id == m_theme && !m_colors.isEmpty()) {
        return m_colors.value(name);
    }
    auto it = m_cache.constFind(id);
    if (it == m_cache.constEnd()) {
        QHash<QString, QColor> palette;
        if (!loadInto(themePath(id), palette)) {
            return {};
        }
        m_cache.insert(id, palette);
        it = m_cache.constFind(id);
    }
    return it.value().value(name);
}

bool ColorManager::loadPalette(const QString &resourcePath) {
    QHash<QString, QColor> palette;
    if (!loadInto(resourcePath, palette)) {
        return false;
    }
    m_colors = palette;
    emit paletteChanged();
    return true;
}

bool ColorManager::loadInto(const QString &resourcePath,
                            QHash<QString, QColor> &target) const {
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QHash<QString, QColor> palette;
    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() &&
            reader.name() == QStringLiteral("color")) {
            const QString name =
                reader.attributes().value("name").toString();
            const QString hex = reader.readElementText().trimmed();
            if (!name.isEmpty() && !hex.isEmpty()) {
                palette.insert(name, QColor(hex));
            }
        }
    }
    if (reader.hasError() || palette.isEmpty()) {
        return false;
    }
    target = palette;
    return true;
}

QString ColorManager::themePath(const QString &themeId) const {
    const QString file = QString("theme_%1.xml").arg(themeId);
    QString path = QString(
        ":/Dopamine/Core/src/shared/colors/%1").arg(file);
    if (!QFile::exists(path)) {
        path = QString("src/shared/colors/%1").arg(file);
    }
    return path;
}

QColor ColorManager::get(const QString &name,
                         const QColor &fallback) const {
    return m_colors.value(name, fallback);
}

bool ColorManager::hasColor(const QString &name) const {
    return m_colors.contains(name);
}

} // namespace Dopamine::Shared::Colors
