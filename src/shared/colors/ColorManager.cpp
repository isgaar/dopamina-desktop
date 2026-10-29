/**
 * Location: src/shared/colors/ColorManager.cpp
 * Function: Implementation of the XML palette catalog supporting the five
 * Android themes, cached previews, and live palette notifications.
 * All Rights Reserved isgaar
 */
#include "ColorManager.h"
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QVariantAnimation>
#include <QEasingCurve>
#include <QXmlStreamReader>
#include <algorithm>
#include <cmath>

namespace Dopamine::Shared::Colors {

namespace {
double chan(double c) {
    return (c <= 0.03928) ? (c / 12.92) : std::pow((c + 0.055) / 1.055, 2.4);
}

double relativeLuminance(const QColor &color) {
    double r = chan(color.redF());
    double g = chan(color.greenF());
    double b = chan(color.blueF());
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

double calculateWcagContrast(const QColor &fg, const QColor &bg) {
    double l1 = relativeLuminance(fg);
    double l2 = relativeLuminance(bg);
    return (std::max(l1, l2) + 0.05) / (std::min(l1, l2) + 0.05);
}

struct SwatchBucket {
    int count = 0;
    double sumH = 0.0;
    double sumS = 0.0;
    double sumL = 0.0;
};
} // namespace

ColorManager &ColorManager::instance() {
    static ColorManager s_instance;
    return s_instance;
}

ColorManager::ColorManager(QObject *parent)
    : QObject(parent) {}

ColorManager::~ColorManager() {
    if (m_accentAnim) {
        m_accentAnim->stop();
    }
}

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
    if (!m_hasDynamicAccent) {
        m_currentAccent = m_colors.value(QStringLiteral("primary"), QColor(0xFA, 0x2D, 0x48));
    }
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

QColor ColorManager::ensureContrastOnScrim(const QColor &color, const QColor &bg, double minRatio) {
    if (calculateWcagContrast(color, bg) >= minRatio) return color;
    float h, s, l;
    color.getHslF(&h, &s, &l);
    if (h < 0.0f) h = 0.0f;
    int step = 0;
    QColor candidate = color;
    while (calculateWcagContrast(candidate, bg) < minRatio && step < 24) {
        l = std::min(0.78f, l + 0.025f);
        candidate = QColor::fromHslF(h, s, l);
        step++;
    }
    return candidate;
}

QColor ColorManager::extractAccentFromImage(const QImage &origImage, const QColor &fallback) {
    if (origImage.isNull()) return fallback;
    const QImage img = origImage.scaled(128, 128, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                                .convertToFormat(QImage::Format_RGB32);
    if (img.isNull()) return fallback;

    SwatchBucket buckets[24 * 4 * 4];

    for (int y = 0; y < img.height(); y += 2) {
        const QRgb *scanLine = reinterpret_cast<const QRgb*>(img.constScanLine(y));
        for (int x = 0; x < img.width(); x += 2) {
            const QRgb rgb = scanLine[x];
            const QColor c(rgb);
            float h, s, l;
            c.getHslF(&h, &s, &l);
            if (h < 0.0f || s < 0.12f || l < 0.08f || l > 0.92f) continue;

            const int hBin = qBound(0, static_cast<int>(h * 24.0f), 23);
            const int sBin = qBound(0, static_cast<int>(s * 4.0f), 3);
            const int lBin = qBound(0, static_cast<int>(l * 4.0f), 3);
            const int idx = (hBin << 4) | (sBin << 2) | lBin;

            buckets[idx].count++;
            buckets[idx].sumH += h;
            buckets[idx].sumS += s;
            buckets[idx].sumL += l;
        }
    }

    double bestScore = -1.0;
    double bestH = 0.0, bestS = 0.0, bestL = 0.0;
    int bestCount = 0;

    for (int i = 0; i < 24 * 4 * 4; ++i) {
        if (buckets[i].count <= 0) continue;
        const double avgH = buckets[i].sumH / buckets[i].count;
        const double avgS = buckets[i].sumS / buckets[i].count;
        const double avgL = buckets[i].sumL / buckets[i].count;

        const double sW = qBound(0.0, avgS, 1.0);
        const double lW = 1.0 - qBound(0.0, 2.0 * std::abs(avgL - 0.5), 1.0);
        const bool isVib = (avgS >= 0.45 && avgL >= 0.35 && avgL <= 0.75);
        const double bst = isVib ? 1.25 : 1.0;
        const double score = std::log(1.0 + static_cast<double>(buckets[i].count)) *
                             (0.35 + 0.65 * sW) *
                             (0.40 + 0.60 * lW) * bst;

        if (score > bestScore) {
            bestScore = score;
            bestH = avgH;
            bestS = avgS;
            bestL = avgL;
            bestCount = buckets[i].count;
        }
    }

    if (bestCount <= 0 || bestS < 0.10) {
        return fallback;
    }

    const double finalS = qBound(0.35, bestS, 0.95);
    const double finalL = qBound(0.48, bestL, 0.70);
    const QColor candidate = QColor::fromHslF(bestH, finalS, finalL);
    return ensureContrastOnScrim(candidate);
}

void ColorManager::applyTargetAccent(const QColor &target, bool immediate) {
    if (!target.isValid()) return;
    m_targetAccent = target;
    m_hasDynamicAccent = true;

    if (immediate || !m_currentAccent.isValid()) {
        m_currentAccent = target;
        emit paletteChanged();
        return;
    }

    if (m_currentAccent == target) return;

    if (!m_accentAnim) {
        m_accentAnim = new QVariantAnimation(this);
        m_accentAnim->setDuration(350);
        m_accentAnim->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_accentAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
            m_currentAccent = val.value<QColor>();
            emit paletteChanged();
        });
    } else {
        m_accentAnim->stop();
    }
    m_accentAnim->setStartValue(m_currentAccent);
    m_accentAnim->setEndValue(target);
    m_accentAnim->start();
}

void ColorManager::updateTrackCover(const QString &coverUrl, int trackId) {
    if (!m_dynamicAccentEnabled) return;
    const QString trimmed = coverUrl.trimmed();
    if (trimmed.isEmpty()) {
        clearDynamicAccent();
        return;
    }

    const QString cacheKey = (trackId > 0) ? QStringLiteral("id_%1").arg(trackId) : trimmed;
    if (m_accentCache.contains(cacheKey)) {
        applyTargetAccent(m_accentCache.value(cacheKey), false);
        return;
    }

    const QColor fallback = m_colors.value(QStringLiteral("primary"), QColor(0xFA, 0x2D, 0x48));

    QString localPath = trimmed;
    if (localPath.startsWith(QLatin1String("file://"))) {
        localPath = QUrl(localPath).toLocalFile();
    }
    if (QFile::exists(localPath)) {
        QImage img(localPath);
        if (!img.isNull()) {
            const QColor extracted = extractAccentFromImage(img, fallback);
            m_accentCache.insert(cacheKey, extracted);
            applyTargetAccent(extracted, false);
            return;
        }
    }

    if (trimmed.startsWith(QLatin1String("http://")) || trimmed.startsWith(QLatin1String("https://"))) {
        if (!m_nam) m_nam = new QNetworkAccessManager(this);
        QNetworkRequest req{QUrl(trimmed)};
        QNetworkReply *reply = m_nam->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, cacheKey, fallback]() {
            reply->deleteLater();
            if (reply->error() == QNetworkReply::NoError) {
                QImage img;
                if (img.loadFromData(reply->readAll())) {
                    const QColor extracted = extractAccentFromImage(img, fallback);
                    m_accentCache.insert(cacheKey, extracted);
                    applyTargetAccent(extracted, false);
                }
            }
        });
    }
}

void ColorManager::setDynamicAccentEnabled(bool enabled) {
    if (m_dynamicAccentEnabled == enabled) return;
    m_dynamicAccentEnabled = enabled;
    emit dynamicAccentChanged();
    if (!enabled) {
        clearDynamicAccent();
    }
    emit paletteChanged();
}

void ColorManager::clearDynamicAccent() {
    if (!m_hasDynamicAccent) return;
    const QColor fallback = m_colors.value(QStringLiteral("primary"), QColor(0xFA, 0x2D, 0x48));
    if (m_currentAccent.isValid()) {
        applyTargetAccent(fallback, false);
    }
    m_hasDynamicAccent = false;
}

QColor ColorManager::activeAccent() const {
    if (m_dynamicAccentEnabled && m_hasDynamicAccent && m_currentAccent.isValid()) {
        return m_currentAccent;
    }
    return m_colors.value(QStringLiteral("primary"), QColor(0xFA, 0x2D, 0x48));
}

QColor ColorManager::get(const QString &name,
                         const QColor &fallback) const {
    if (m_dynamicAccentEnabled && m_hasDynamicAccent && m_currentAccent.isValid()) {
        if (name == QLatin1String("primary") ||
            name == QLatin1String("accent") ||
            name == QLatin1String("border_highlight") ||
            name == QLatin1String("border_accent") ||
            name == QLatin1String("eq_coral") ||
            name == QLatin1String("eq_pro_badge")) {
            return m_currentAccent;
        }
        if (name == QLatin1String("primary_hover") ||
            name == QLatin1String("accent_pink")) {
            return m_currentAccent.lighter(112);
        }
        if (name == QLatin1String("primary_dim") ||
            name == QLatin1String("accent_light") ||
            name == QLatin1String("eq_coral_dim")) {
            QColor c = m_currentAccent;
            c.setAlpha(static_cast<int>(255 * 0.22));
            return c;
        }
    }
    return m_colors.value(name, fallback);
}

bool ColorManager::hasColor(const QString &name) const {
    return m_colors.contains(name);
}

} // namespace Dopamine::Shared::Colors
