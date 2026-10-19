/**
 * Location: src/shared/colors/ColorManager.cpp
 * Function: Implementation of the XML color catalog manager parsing color
 * resource tokens using QXmlStreamReader and resolving semantic lookups.
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

bool ColorManager::initialize(const QString &customPath) {
    if (!customPath.isEmpty()) {
        return loadPalette(customPath);
    }
    QString resPath =
        QStringLiteral(":/Dopamine/Core/src/shared/colors/colors.xml");
    if (!QFile::exists(resPath)) {
        resPath = QStringLiteral("src/shared/colors/colors.xml");
    }
    return loadPalette(resPath);
}

bool ColorManager::loadPalette(const QString &resourcePath) {
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    m_colors.clear();
    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() &&
            reader.name() == QStringLiteral("color")) {
            const QString name =
                reader.attributes().value("name").toString();
            const QString hex = reader.readElementText().trimmed();
            if (!name.isEmpty() && !hex.isEmpty()) {
                m_colors.insert(name, QColor(hex));
            }
        }
    }
    return !reader.hasError();
}

QColor ColorManager::get(const QString &name,
                         const QColor &fallback) const {
    return m_colors.value(name, fallback);
}

bool ColorManager::hasColor(const QString &name) const {
    return m_colors.contains(name);
}

} // namespace Dopamine::Shared::Colors
