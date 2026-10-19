/**
 * Location: src/shared/i18n/StringManager.cpp
 * Function: Implementation of the internationalization manager parsing XML
 * resource files using QXmlStreamReader and resolving key translations.
 * All Rights Reserved isgaar
 */
#include "StringManager.h"
#include "core/platform/PlatformHelper.h"
#include <QFile>
#include <QXmlStreamReader>

namespace Dopamine::Shared::I18n {

StringManager &StringManager::instance() {
    static StringManager s_instance;
    return s_instance;
}

StringManager::StringManager(QObject *parent)
    : QObject(parent), m_locale("es") {}

void StringManager::initialize(const QString &forcedLocale) {
    if (!forcedLocale.isEmpty()) {
        m_locale = forcedLocale;
    } else {
        m_locale = Dopamine::Core::Platform::PlatformHelper::instance()
            .detectHostLanguage();
    }

    const QString fileName = QString("strings_%1.xml").arg(m_locale);
    QString resPath =
        QString(":/Dopamine/Core/src/shared/i18n/%1").arg(fileName);
    if (!QFile::exists(resPath)) {
        resPath = QString("src/shared/i18n/%1").arg(fileName);
    }
    loadCatalog(resPath);
}

void StringManager::setLocale(const QString &locale) {
    if (m_locale == locale) return;
    initialize(locale);
    emit localeChanged();
}

bool StringManager::loadCatalog(const QString &resourcePath) {
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    m_strings.clear();
    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() &&
            reader.name() == QStringLiteral("string")) {
            const QString key = reader.attributes().value("name").toString();
            const QString val = reader.readElementText().trimmed();
            if (!key.isEmpty()) {
                m_strings.insert(key, val);
            }
        }
    }
    return !reader.hasError();
}

QString StringManager::get(const QString &key) const {
    auto it = m_strings.constFind(key);
    if (it != m_strings.constEnd()) {
        return it.value();
    }
    return key;
}

} // namespace Dopamine::Shared::I18n
