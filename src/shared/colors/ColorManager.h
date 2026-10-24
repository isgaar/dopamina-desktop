/**
 * Location: src/shared/colors/ColorManager.h
 * Function: XML color palette loader parsing Android-style theme resources
 * into semantic QColor tokens with live palette switching and previews.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_SHARED_COLORS_COLORMANAGER_H
#define DOPAMINE_SHARED_COLORS_COLORMANAGER_H

#include <QColor>
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

namespace Dopamine::Shared::Colors {

class ColorManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentTheme READ currentTheme NOTIFY paletteChanged)
public:
    static ColorManager &instance();
    static QStringList themeIds();
    explicit ColorManager(QObject *parent = nullptr);

    bool initialize(const QString &customPath = QString());
    bool loadPalette(const QString &resourcePath);

    Q_INVOKABLE bool setTheme(const QString &themeId);
    Q_INVOKABLE QColor paletteColor(const QString &themeId,
                                    const QString &name);
    Q_INVOKABLE QStringList availableThemes() const {
        return themeIds();
    }
    QString currentTheme() const { return m_theme; }

    Q_INVOKABLE QColor get(const QString &name,
                          const QColor &fallback = QColor()) const;
    Q_INVOKABLE bool hasColor(const QString &name) const;

signals:
    void paletteChanged();

private:
    bool loadInto(const QString &resourcePath,
                  QHash<QString, QColor> &target) const;
    QString themePath(const QString &themeId) const;

    QHash<QString, QColor> m_colors;
    QHash<QString, QHash<QString, QColor>> m_cache;
    QString m_theme = QStringLiteral("roseglow");
};

} // namespace Dopamine::Shared::Colors

#endif // DOPAMINE_SHARED_COLORS_COLORMANAGER_H
