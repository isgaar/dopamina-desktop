/**
 * Location: src/shared/Theme.h
 * Function: Master design token singleton exposing colors, radiuses, spacing,
 * and typography constants for consistent native desktop styling in Qt Quick.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_SHARED_THEME_H
#define DOPAMINE_SHARED_THEME_H

#include <QColor>
#include <QLocale>
#include <QObject>
#include <QString>

namespace Dopamine::Shared {

class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString appName READ appName CONSTANT)
    Q_PROPERTY(QString fontFamily READ fontFamily CONSTANT)
    Q_PROPERTY(bool isSpanish READ isSpanish CONSTANT)

    Q_PROPERTY(QColor primary READ primary CONSTANT)
    Q_PROPERTY(QColor primaryHover READ primaryHover CONSTANT)
    Q_PROPERTY(QColor primaryDim READ primaryDim CONSTANT)
    Q_PROPERTY(QColor accent READ accent CONSTANT)
    Q_PROPERTY(QColor accentLight READ accentLight CONSTANT)
    Q_PROPERTY(QColor accentOrange READ accentOrange CONSTANT)

    Q_PROPERTY(QColor bgDarkest READ bgDarkest CONSTANT)
    Q_PROPERTY(QColor bgDark READ bgDark CONSTANT)
    Q_PROPERTY(QColor bgSurface READ bgSurface CONSTANT)
    Q_PROPERTY(QColor bgCard READ bgCard CONSTANT)
    Q_PROPERTY(QColor bgSelected READ bgSelected CONSTANT)
    Q_PROPERTY(QColor borderDark READ borderDark CONSTANT)
    Q_PROPERTY(QColor borderHighlight READ borderHighlight CONSTANT)

    Q_PROPERTY(QColor textLight READ textLight CONSTANT)
    Q_PROPERTY(QColor textMuted READ textMuted CONSTANT)
    Q_PROPERTY(QColor textDim READ textDim CONSTANT)

    Q_PROPERTY(QColor success READ success CONSTANT)
    Q_PROPERTY(QColor danger READ danger CONSTANT)
    Q_PROPERTY(QColor warning READ warning CONSTANT)

    Q_PROPERTY(int spaceXs READ spaceXs CONSTANT)
    Q_PROPERTY(int spaceSm READ spaceSm CONSTANT)
    Q_PROPERTY(int spaceMd READ spaceMd CONSTANT)
    Q_PROPERTY(int spaceLg READ spaceLg CONSTANT)
    Q_PROPERTY(int spaceXl READ spaceXl CONSTANT)

    Q_PROPERTY(int radiusSm READ radiusSm CONSTANT)
    Q_PROPERTY(int radiusMd READ radiusMd CONSTANT)
    Q_PROPERTY(int radiusLg READ radiusLg CONSTANT)
    Q_PROPERTY(int radiusFull READ radiusFull CONSTANT)

    Q_PROPERTY(int fontSm READ fontSm CONSTANT)
    Q_PROPERTY(int fontBase READ fontBase CONSTANT)
    Q_PROPERTY(int fontMd READ fontMd CONSTANT)
    Q_PROPERTY(int fontLg READ fontLg CONSTANT)

public:
    static Theme &instance();
    explicit Theme(QObject *parent = nullptr);

    QString appName() const { return QStringLiteral("Dopamine"); }
    QString fontFamily() const { return QStringLiteral("LumpySans"); }
    bool isSpanish() const {
        return QLocale::system().language() == QLocale::Spanish;
    }

    Q_INVOKABLE QString string(const QString &key) const;
    Q_INVOKABLE QColor color(const QString &key) const;
    Q_INVOKABLE QString formatDuration(double seconds) const;
    Q_INVOKABLE QColor withAlpha(const QColor &c, qreal a) const {
        QColor r = c; r.setAlphaF(a); return r;
    }

    QColor primary() const;
    QColor primaryHover() const;
    QColor primaryDim() const;
    QColor accent() const;
    QColor accentLight() const;
    QColor accentOrange() const;

    QColor bgDarkest() const;
    QColor bgDark() const;
    QColor bgSurface() const;
    QColor bgCard() const;
    QColor bgSelected() const;
    QColor borderDark() const;
    QColor borderHighlight() const;

    QColor textLight() const;
    QColor textMuted() const;
    QColor textDim() const;

    QColor success() const;
    QColor danger() const;
    QColor warning() const;

    int spaceXs() const { return 4; } int spaceSm() const { return 8; }
    int spaceMd() const { return 16; } int spaceLg() const { return 24; }
    int spaceXl() const { return 32; }
    int radiusSm() const { return 6; } int radiusMd() const { return 10; }
    int radiusLg() const { return 14; } int radiusFull() const { return 9999; }
    int fontSm() const { return 11; } int fontBase() const { return 13; }
    int fontMd() const { return 15; } int fontLg() const { return 18; }
};

} // namespace Dopamine::Shared

#endif // DOPAMINE_SHARED_THEME_H
