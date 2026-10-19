/**
 * Location: src/shared/i18n/StringManager.h
 * Function: Android Studio-style internationalization manager parsing XML
 * resources and providing dynamic language resolution for declarative QML.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_SHARED_I18N_STRINGMANAGER_H
#define DOPAMINE_SHARED_I18N_STRINGMANAGER_H

#include <QHash>
#include <QObject>
#include <QString>

namespace Dopamine::Shared::I18n {

class StringManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentLocale READ currentLocale NOTIFY localeChanged)

public:
    static StringManager &instance();
    explicit StringManager(QObject *parent = nullptr);

    void initialize(const QString &forcedLocale = QString());
    bool loadCatalog(const QString &resourcePath);

    Q_INVOKABLE QString get(const QString &key) const;
    Q_INVOKABLE void setLocale(const QString &locale);
    QString currentLocale() const { return m_locale; }

signals:
    void localeChanged();

private:
    QString m_locale;
    QHash<QString, QString> m_strings;
};

} // namespace Dopamine::Shared::I18n

#endif // DOPAMINE_SHARED_I18N_STRINGMANAGER_H
