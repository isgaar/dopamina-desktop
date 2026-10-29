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

class QVariantAnimation;
class QNetworkAccessManager;

namespace Dopamine::Shared::Colors {

class ColorManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentTheme READ currentTheme NOTIFY paletteChanged)
    Q_PROPERTY(bool dynamicAccentEnabled READ isDynamicAccentEnabled WRITE setDynamicAccentEnabled NOTIFY dynamicAccentChanged)
    Q_PROPERTY(QColor activeAccent READ activeAccent NOTIFY paletteChanged)
    Q_PROPERTY(bool isDynamicActive READ isDynamicActive NOTIFY paletteChanged)
public:
    static ColorManager &instance();
    static QStringList themeIds();
    explicit ColorManager(QObject *parent = nullptr);
    ~ColorManager() override;

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

    bool isDynamicAccentEnabled() const { return m_dynamicAccentEnabled; }
    Q_INVOKABLE void setDynamicAccentEnabled(bool enabled);
    bool isDynamicActive() const { return m_dynamicAccentEnabled && m_hasDynamicAccent; }
    QColor activeAccent() const;

    Q_INVOKABLE void updateTrackCover(const QString &coverUrl, int trackId = 0);
    Q_INVOKABLE void clearDynamicAccent();

signals:
    void paletteChanged();
    void dynamicAccentChanged();

private:
    bool loadInto(const QString &resourcePath,
                  QHash<QString, QColor> &target) const;
    QString themePath(const QString &themeId) const;

    QColor extractAccentFromImage(const QImage &image, const QColor &fallback);
    QColor ensureContrastOnScrim(const QColor &color, const QColor &bg = QColor(0x14, 0x14, 0x14), double minRatio = 4.5);
    void applyTargetAccent(const QColor &target, bool immediate = false);

    QHash<QString, QColor> m_colors;
    QHash<QString, QHash<QString, QColor>> m_cache;
    QString m_theme = QStringLiteral("roseglow");

    bool m_dynamicAccentEnabled = true;
    bool m_hasDynamicAccent = false;
    QColor m_currentAccent;
    QColor m_targetAccent;
    QVariantAnimation *m_accentAnim = nullptr;
    QHash<QString, QColor> m_accentCache;
    QNetworkAccessManager *m_nam = nullptr;
};

} // namespace Dopamine::Shared::Colors

#endif // DOPAMINE_SHARED_COLORS_COLORMANAGER_H
