/**
 * Location: src/features/library/app/LibraryController.cpp
 * Function: Implementation of library controller binding catalog API responses
 * and real-time recursive directory scanner batches to reactive QML models,
 * with alphabetical A-Z section grouping and Hi-Res audio badge metadata.
 * All Rights Reserved isgaar
 */
#include "LibraryController.h"
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QStandardPaths>
#include <algorithm>
#include <cmath>

namespace Dopamine::Features::Library::App {

namespace {
QString computeSectionLetter(const QString &rawTitle) {
    QString trimmed = rawTitle.trimmed();
    if (trimmed.isEmpty()) return QStringLiteral("#");
    QChar ch = trimmed.at(0);
    if (ch.decompositionTag() != QChar::NoDecomposition &&
        !ch.decomposition().isEmpty()) {
        ch = ch.decomposition().at(0);
    }
    QChar up = ch.toUpper();
    if (up >= QLatin1Char('A') && up <= QLatin1Char('Z')) {
        return QString(up);
    }
    return QStringLiteral("#");
}

void enrichTrackQuality(QVariantMap &m) {
    QString fmt = m.value(QStringLiteral("formato")).toString().trimmed();
    if (fmt.isEmpty())
        fmt = m.value(QStringLiteral("format")).toString().trimmed();
    if (fmt.isEmpty())
        fmt = m.value(QStringLiteral("codec")).toString().trimmed();
    if (fmt.isEmpty()) {
        const QString path = m.value(QStringLiteral("path")).toString().toLower();
        if (path.endsWith(QLatin1String(".mp3"))) fmt = QStringLiteral("mp3");
        else if (path.endsWith(QLatin1String(".m4a"))) fmt = QStringLiteral("m4a");
        else if (path.endsWith(QLatin1String(".aac"))) fmt = QStringLiteral("aac");
        else if (path.endsWith(QLatin1String(".wav"))) fmt = QStringLiteral("wav");
        else if (path.endsWith(QLatin1String(".ogg"))) fmt = QStringLiteral("ogg");
        else if (path.endsWith(QLatin1String(".opus"))) fmt = QStringLiteral("opus");
        else if (path.endsWith(QLatin1String(".alac"))) fmt = QStringLiteral("alac");
        else if (path.endsWith(QLatin1String(".aiff"))) fmt = QStringLiteral("aiff");
        else fmt = QStringLiteral("flac");
    }
    const QString fmtLower = fmt.toLower();
    const QString fmtUpper = fmt.toUpper();

    int bits = m.value(QStringLiteral("bit_depth")).toInt();
    if (bits <= 0) bits = m.value(QStringLiteral("bitDepth")).toInt();
    if (bits <= 0) bits = (fmtLower == QLatin1String("flac")) ? 24 : 16;

    int sr = m.value(QStringLiteral("sample_rate")).toInt();
    if (sr <= 0) sr = m.value(QStringLiteral("sampleRate")).toInt();
    if (sr <= 0) sr = 44100;

    const int bitrate = m.value(QStringLiteral("bitrate")).toInt();

    const bool isLossless = (fmtLower == QLatin1String("flac") ||
                             fmtLower == QLatin1String("wav") ||
                             fmtLower == QLatin1String("alac") ||
                             fmtLower == QLatin1String("aiff") ||
                             fmtLower == QLatin1String("dsd"));
    const bool isLossy = (fmtLower == QLatin1String("mp3") ||
                          fmtLower == QLatin1String("aac") ||
                          fmtLower == QLatin1String("ogg") ||
                          fmtLower == QLatin1String("opus") ||
                          fmtLower == QLatin1String("m4a") ||
                          fmtLower == QLatin1String("wma"));

    QString tier;
    if (fmtLower.contains(QLatin1String("adpcm")) || bits == 4) {
        tier = QStringLiteral("LOW");
    } else if (isLossless && (bits >= 24 || sr > 48000)) {
        tier = QStringLiteral("MAX");
    } else if (isLossless) {
        tier = QStringLiteral("HIGH");
    } else if (isLossy && bitrate >= 1 && bitrate <= 219) {
        tier = QStringLiteral("LOW");
    } else {
        tier = QStringLiteral("OK");
    }

    QString details = fmtUpper;
    if (bits > 0) details += QStringLiteral(" %1-bit").arg(bits);
    if (sr > 0) details += QStringLiteral(" / %1 kHz").arg(sr / 1000);
    if (bitrate > 0) details += QStringLiteral(" / %1 kbps").arg(bitrate);

    QStringList metaParts;
    QString genre = m.value(QStringLiteral("genre")).toString().trimmed();
    if (genre.isEmpty())
        genre = m.value(QStringLiteral("genero")).toString().trimmed();
    if (!genre.isEmpty()) metaParts.append(genre);

    int year = m.value(QStringLiteral("year")).toInt();
    if (year <= 0) year = m.value(QStringLiteral("anio")).toInt();
    if (year > 0) metaParts.append(QString::number(year));

    const double bpm = m.value(QStringLiteral("bpm")).toDouble();
    if (bpm > 0.0 && !std::isnan(bpm))
        metaParts.append(QStringLiteral("%1 BPM").arg(static_cast<int>(bpm)));

    QString key = m.value(QStringLiteral("musical_key")).toString().trimmed();
    if (key.isEmpty())
        key = m.value(QStringLiteral("tonalidad")).toString().trimmed();
    if (key.isEmpty())
        key = m.value(QStringLiteral("camelot")).toString().trimmed();
    if (!key.isEmpty()) metaParts.append(key);

    m[QStringLiteral("format")] = fmtUpper;
    m[QStringLiteral("bit_depth")] = bits;
    m[QStringLiteral("sample_rate")] = sr;
    m[QStringLiteral("fidelityTier")] = tier;
    m[QStringLiteral("qualityDetails")] = details;
    m[QStringLiteral("extraMeta")] = metaParts.join(QStringLiteral(" \u2022 "));
    m[QStringLiteral("formatBadge")] = tier;
}
} // namespace

LibraryController::LibraryController(Infra::LibraryApiClient *apiClient,
                                     QObject *parent)
    : QObject(parent), m_api(apiClient),
      m_scanner(new Infra::RecursiveDirectoryScanner(this)) {
    connect(m_scanner, &Infra::RecursiveDirectoryScanner::batchDiscovered,
            this, &LibraryController::handleDiscoveredBatch);
    connect(m_scanner, &Infra::RecursiveDirectoryScanner::scanFinished,
            this, [this](int total) {
                m_isLoading = false;
                m_statusMessage = QStringLiteral("%1 canciones").arg(total);
                emit loadingChanged(); emit statusMessageChanged();
            });
}

void LibraryController::enrichAndSortTracks(QVariantList &list) {
    for (int i = 0; i < list.size(); ++i) {
        QVariantMap m = list[i].toMap();
        QString title = m.value(QStringLiteral("titulo")).toString().trimmed();
        if (title.isEmpty())
            title = m.value(QStringLiteral("title")).toString().trimmed();
        m[QStringLiteral("section")] = computeSectionLetter(title);
        enrichTrackQuality(m);
        list[i] = m;
    }

    std::stable_sort(list.begin(), list.end(),
                     [](const QVariant &a, const QVariant &b) {
        const QVariantMap ma = a.toMap();
        const QVariantMap mb = b.toMap();
        const QString sa = ma.value(QStringLiteral("section")).toString();
        const QString sb = mb.value(QStringLiteral("section")).toString();
        if (sa != sb) {
            if (sa == QLatin1String("#")) return true;
            if (sb == QLatin1String("#")) return false;
            return sa < sb;
        }
        QString ta = ma.value(QStringLiteral("titulo")).toString();
        if (ta.isEmpty()) ta = ma.value(QStringLiteral("title")).toString();
        QString tb = mb.value(QStringLiteral("titulo")).toString();
        if (tb.isEmpty()) tb = mb.value(QStringLiteral("title")).toString();
        return QString::compare(ta, tb, Qt::CaseInsensitive) < 0;
    });

    QString prevSection;
    for (int i = 0; i < list.size(); ++i) {
        QVariantMap m = list[i].toMap();
        QString sec = m.value(QStringLiteral("section")).toString();
        m[QStringLiteral("showSectionHeader")] = (i == 0 || sec != prevSection);
        prevSection = sec;
        list[i] = m;
    }
}

int LibraryController::indexForSection(const QString &letter) const {
    if (m_tracks.isEmpty()) return -1;
    const QString target = letter.trimmed().toUpper();
    for (int i = 0; i < m_tracks.size(); ++i) {
        const QString sec = m_tracks[i].toMap()
                                .value(QStringLiteral("section")).toString();
        if (sec == target) return i;
        if (target != QLatin1String("#") && sec != QLatin1String("#") && sec > target) {
            return i;
        }
    }
    return m_tracks.size() - 1;
}

void LibraryController::handleDiscoveredBatch(const QVariantList &batch) {
    for (const auto &item : batch) {
        m_allTracks.append(item);
        auto m = item.toMap();
        QString alb = m.value(QStringLiteral("album")).toString();
        if (!alb.isEmpty() && !m_knownAlbums.contains(alb)) {
            m_knownAlbums.insert(alb);
            m_albums.append(QVariantMap{{QStringLiteral("name"), alb}});
        }
        QString art = m.value(QStringLiteral("artist")).toString();
        if (!art.isEmpty() && !m_knownArtists.contains(art)) {
            m_knownArtists.insert(art);
            m_artists.append(QVariantMap{{QStringLiteral("name"), art}});
        }
    }
    applySearchFilter();
    m_statusMessage = QStringLiteral("%1 canciones").arg(m_allTracks.size());
    emit statusMessageChanged(); emit albumsChanged(); emit artistsChanged();
}

void LibraryController::applySearchFilter() {
    if (m_searchQuery.isEmpty()) {
        m_tracks = m_allTracks;
    } else {
        m_tracks.clear();
        QString q = m_searchQuery.toLower();
        for (const auto &v : m_allTracks) {
            auto m = v.toMap();
            bool t = m.value(QStringLiteral("titulo"))
                         .toString().toLower().contains(q) ||
                     m.value(QStringLiteral("title"))
                         .toString().toLower().contains(q);
            bool a = m.value(QStringLiteral("artista"))
                         .toString().toLower().contains(q) ||
                     m.value(QStringLiteral("artist"))
                         .toString().toLower().contains(q);
            bool b = m.value(QStringLiteral("album"))
                         .toString().toLower().contains(q);
            if (t || a || b) m_tracks.append(v);
        }
    }
    enrichAndSortTracks(m_tracks);
    emit tracksChanged();
}

void LibraryController::setSearchQuery(const QString &q) {
    if (m_searchQuery == q) return;
    m_searchQuery = q; emit searchQueryChanged();
    if (m_localSource) applySearchFilter();
    else loadTracks(1);
}

double LibraryController::totalDurationHours() const {
    double s = 0.0;
    for (const auto &v : m_tracks)
        s += v.toMap().value(QStringLiteral("duracion")).toDouble();
    return s / 3600.0;
}

void LibraryController::search(const QString &q) { setSearchQuery(q); }

void LibraryController::loadTracks(int page) {
    page = qMax(1, page);
    QString dir = Infra::RecursiveDirectoryScanner::resolveRootPath();
    if (!dir.isEmpty()) {
        if (page > 1) return;
        m_localSource = true; m_hasMoreTracks = false;
        m_trackPage = 0;
        m_isLoading = true; m_statusMessage = QStringLiteral("Buscando...");
        emit loadingChanged(); emit statusMessageChanged();
        m_allTracks.clear(); m_tracks.clear(); m_albums.clear();
        m_artists.clear(); m_knownAlbums.clear(); m_knownArtists.clear();
        emit tracksChanged(); m_scanner->startScan(dir); return;
    }
    m_localSource = false;

    if (!m_api) return;
    if (page > 1 && (m_isLoading || !m_hasMoreTracks)) return;
    const bool reset = page <= 1;
    if (reset) ++m_requestGeneration;
    const quint64 generation = m_requestGeneration;
    m_isLoading = true; emit loadingChanged();
    m_api->fetchTracks(page, 100, m_searchQuery,
                       [this, page, reset, generation](bool ok, auto arr) {
        if (generation != m_requestGeneration) return;
        m_isLoading = false; emit loadingChanged();
        if (!ok) return;
        if (reset) m_tracks.clear();
        for (const auto &val : arr)
            m_tracks.append(val.toObject().toVariantMap());
        enrichAndSortTracks(m_tracks);
        m_trackPage = page;
        m_hasMoreTracks = arr.size() >= 100;
        emit tracksChanged();
    });
}

void LibraryController::loadMoreTracks() {
    if (!m_isLoading && m_hasMoreTracks) loadTracks(m_trackPage + 1);
}

void LibraryController::loadAlbums(int page) {
    if (!m_albums.isEmpty() || !m_api) return;
    m_isLoading = true; emit loadingChanged();
    m_api->fetchAlbums(page, 50, [this](bool ok, auto arr) {
        m_isLoading = false; emit loadingChanged();
        if (!ok) return;
        m_albums.clear();
        for (const auto &val : arr)
            m_albums.append(val.toObject().toVariantMap());
        emit albumsChanged();
    });
}

void LibraryController::loadArtists(int page) {
    if (!m_artists.isEmpty() || !m_api) return;
    m_isLoading = true; emit loadingChanged();
    m_api->fetchArtists(page, 50, [this](bool ok, auto arr) {
        m_isLoading = false; emit loadingChanged();
        if (!ok) return;
        m_artists.clear();
        for (const auto &val : arr)
            m_artists.append(val.toObject().toVariantMap());
        emit artistsChanged();
    });
}

QVariantMap LibraryController::findTrackById(int trackId) const {
    if (trackId == 0) return {};
    for (const auto &v : m_tracks) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("id")).toInt() == trackId) return m;
    }
    return {};
}

void LibraryController::fetchTrackMetadata(int trackId) {
    if (!m_api || trackId <= 0) return;
    m_api->fetchTrackMetadata(trackId,
        [this, trackId](bool ok, const QJsonObject &obj) {
            if (!ok) return;
            QVariantMap meta = obj.toVariantMap();
            enrichTrackQuality(meta);
            emit trackMetadataLoaded(trackId, meta);
        });
}

void LibraryController::downloadTrack(int trackId, const QString &title,
                                      const QString &format,
                                      const QString &streamPath) {
    QString dlDir = QStandardPaths::writableLocation(
        QStandardPaths::DownloadLocation);
    if (dlDir.isEmpty()) dlDir = QDir::homePath();
    QDir().mkpath(dlDir);

    QString safeTitle = title.trimmed();
    if (safeTitle.isEmpty()) safeTitle = QStringLiteral("pista_%1").arg(trackId);
    safeTitle.replace(QLatin1Char('/'), QLatin1Char('_'));
    safeTitle.replace(QLatin1Char('\\'), QLatin1Char('_'));
    QString ext = format.trimmed().toLower();
    if (ext.isEmpty()) ext = QStringLiteral("flac");
    const QString destPath = dlDir + QLatin1Char('/') + safeTitle +
                             QLatin1Char('.') + ext;

    if (!streamPath.isEmpty() && !streamPath.startsWith(QLatin1String("http"))) {
        QString localSrc = streamPath;
        if (localSrc.startsWith(QLatin1String("file://")))
            localSrc = localSrc.mid(7);
        QFile::remove(destPath);
        const bool ok = QFile::copy(localSrc, destPath);
        m_statusMessage = ok ? QStringLiteral("Guardado en %1").arg(destPath)
                             : QStringLiteral("Error al guardar pista");
        emit statusMessageChanged();
        emit trackDownloaded(destPath, ok);
        return;
    }

    if (!m_api || trackId <= 0) return;
    m_statusMessage = QStringLiteral("Descargando %1...").arg(safeTitle);
    emit statusMessageChanged();
    m_api->downloadTrackStream(trackId,
        [this, destPath, safeTitle](bool ok, const QByteArray &data) {
            bool saved = false;
            if (ok && !data.isEmpty()) {
                QFile f(destPath);
                if (f.open(QIODevice::WriteOnly)) {
                    saved = (f.write(data) == data.size());
                    f.close();
                }
            }
            m_statusMessage = saved
                ? QStringLiteral("Descarga completada: %1").arg(safeTitle)
                : QStringLiteral("Error al descargar %1").arg(safeTitle);
            emit statusMessageChanged();
            emit trackDownloaded(destPath, saved);
        });
}

} // namespace Dopamine::Features::Library::App
