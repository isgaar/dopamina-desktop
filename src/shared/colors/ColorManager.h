/**
 * Location: src/shared/colors/ColorManager.h
 * Function: XML color palette loader parsing colors.xml into QColor tokens
 * and providing global lookups for QML views and C++ styling classes.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_SHARED_COLORS_COLORMANAGER_H
#define DOPAMINE_SHARED_COLORS_COLORMANAGER_H

#include <QColor>
#include <QHash>
#include <QObject>
#include <QString>

namespace Dopamine::Shared::Colors {

class ColorManager : public QObject {
    Q_OBJECT
public:
    static ColorManager &instance();
    explicit ColorManager(QObject *parent = nullptr);

    bool initialize(const QString &customPath = QString());
    bool loadPalette(const QString &resourcePath);

    Q_INVOKABLE QColor get(const QString &name,
                          const QColor &fallback = QColor()) const;
    Q_INVOKABLE bool hasColor(const QString &name) const;

private:
    QHash<QString, QColor> m_colors;
};

} // namespace Dopamine::Shared::Colors

#endif // DOPAMINE_SHARED_COLORS_COLORMANAGER_H
