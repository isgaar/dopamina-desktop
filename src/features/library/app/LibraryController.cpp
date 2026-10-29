#include <QSettings>
#include <QFile>
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

static int computeFidelityScore(const QVariantMap &m) {
    int score = 0;
    double dur = m.value(QStringLiteral("duracion")).toDouble();
    if (dur <= 0.0) dur = m.value(QStringLiteral("duration")).toDouble();
    if (dur > 1.0) score += 1000;

    QString fmt = m.value(QStringLiteral("formato")).toString().toLower();
    if (fmt.isEmpty()) fmt = m.value(QStringLiteral("format")).toString().toLower();
    if (fmt.contains(QLatin1String("flac")) || fmt.contains(QLatin1String("wav")) ||
        fmt.contains(QLatin1String("alac")) || fmt.contains(QLatin1String("aiff"))) {
        score += 500;
    } else if (fmt.contains(QLatin1String("aac")) || fmt.contains(QLatin1String("m4a")) ||
               fmt.contains(QLatin1String("opus"))) {
        score += 300;
    } else if (fmt.contains(QLatin1String("mp3"))) {
        score += 200;
    } else {
        score += 100;
    }

    int br = m.value(QStringLiteral("bitrate")).toInt();
    if (br >= 1411) score += 400;
    else if (br >= 320) score += 300;
    else if (br >= 256) score += 200;
    else if (br >= 192) score += 100;
    else if (br > 0) score += 50;

    int sr = m.value(QStringLiteral("sample_rate")).toInt();
    if (sr <= 0) sr = m.value(QStringLiteral("sampleRate")).toInt();
    if (sr >= 96000) score += 100;
    else if (sr >= 48000) score += 50;
    else if (sr >= 44100) score += 25;

    int bd = m.value(QStringLiteral("bit_depth")).toInt();
    if (bd <= 0) bd = m.value(QStringLiteral("bitDepth")).toInt();
    if (bd >= 24) score += 100;
    else if (bd >= 16) score += 50;

    return score;
}

static std::pair<QString, QString> extractBaseAndVersion(const QString &title) {
    static const QRegularExpression verRegex(
        QStringLiteral("\\b(remix|edit|live|acoustic|instrumental|dub|club\\s*mix|extended|radio\\s*edit|vip|vocal|acapella|cover|version|session|demo|orchestral)\\b"),
        QRegularExpression::CaseInsensitiveOption
    );
    QString t = title.trimmed();
    QRegularExpressionMatch match = verRegex.match(t);
    QString ver = match.hasMatch() ? match.captured(0).toLower() : QString();

    QString clean = t;
    clean.remove(QRegularExpression(QStringLiteral("[\\(\\[\\{][^\\)\\]\\}]*[\\)\\]\\}]")));
    clean.remove(QRegularExpression(QStringLiteral("[-–—]\\s*.*$")));
    clean.remove(QRegularExpression(QStringLiteral("[^\\w\\s]")));
    clean = clean.trimmed().toLower();
    return {clean.isEmpty() ? t.toLower() : clean, ver};
}

static bool areTracksDuplicates(const QVariantMap &a, const QVariantMap &b) {
    QString artA = a.value(QStringLiteral("artista")).toString().trimmed();
    if (artA.isEmpty()) artA = a.value(QStringLiteral("artist")).toString().trimmed();
    QString artB = b.value(QStringLiteral("artista")).toString().trimmed();
    if (artB.isEmpty()) artB = b.value(QStringLiteral("artist")).toString().trimmed();

    artA.remove(QRegularExpression(QStringLiteral("[^\\w\\s]")));
    artB.remove(QRegularExpression(QStringLiteral("[^\\w\\s]")));
    if (artA.isEmpty() || artB.isEmpty() || artA.compare(artB, Qt::CaseInsensitive) != 0)
        return false;

    QString titA = a.value(QStringLiteral("titulo")).toString().trimmed();
    if (titA.isEmpty()) titA = a.value(QStringLiteral("title")).toString().trimmed();
    QString titB = b.value(QStringLiteral("titulo")).toString().trimmed();
    if (titB.isEmpty()) titB = b.value(QStringLiteral("title")).toString().trimmed();

    auto [baseA, verA] = extractBaseAndVersion(titA);
    auto [baseB, verB] = extractBaseAndVersion(titB);

    if (baseA != baseB) return false;
    if (verA != verB) return false;

    double durA = a.value(QStringLiteral("duracion")).toDouble();
    if (durA <= 0.0) durA = a.value(QStringLiteral("duration")).toDouble();
    double durB = b.value(QStringLiteral("duracion")).toDouble();
    if (durB <= 0.0) durB = b.value(QStringLiteral("duration")).toDouble();

    // Corrupt / 00:00 duration detection
    if ((durA <= 1.0 && durB > 1.0) || (durB <= 1.0 && durA > 1.0))
        return true;
    if (durA <= 1.0 && durB <= 1.0)
        return true;

    double durDiff = std::abs(durA - durB);
    double maxDur = std::max(durA, durB);
    if (durDiff > 3.0 && (durDiff / maxDur) > 0.015)
        return false;

    double bpmA = a.value(QStringLiteral("bpm")).toDouble();
    double bpmB = b.value(QStringLiteral("bpm")).toDouble();
    if (bpmA > 10.0 && bpmB > 10.0) {
        double bpmDiff = std::abs(bpmA - bpmB);
        double oct1 = std::abs(bpmA * 2 - bpmB);
        double oct2 = std::abs(bpmA - bpmB * 2);
        if (bpmDiff > 1.5 && oct1 > 2.0 && oct2 > 2.0)
            return false;
    }

    return true;
}

static void deduplicateTracksInPlace(QVariantList &list) {
    QVariantList deduped;
    for (const QVariant &item : list) {
        QVariantMap m = item.toMap();
        int score = computeFidelityScore(m);
        m[QStringLiteral("fidelityScore")] = score;

        int matchedIdx = -1;
        for (int i = 0; i < deduped.size(); ++i) {
            if (areTracksDuplicates(m, deduped[i].toMap())) {
                matchedIdx = i;
                break;
            }
        }
        if (matchedIdx == -1) {
            deduped.append(m);
        } else {
            if (score > deduped[matchedIdx].toMap().value(QStringLiteral("fidelityScore")).toInt()) {
                deduped[matchedIdx] = m;
            }
        }
    }
    list = deduped;
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

    deduplicateTracksInPlace(list);

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
    if (trackId <= 0) return;

    if (isTrackDownloaded(trackId)) {
        m_statusMessage = QStringLiteral("La pista ya se encuentra descargada");
        emit statusMessageChanged();
        emit trackDownloaded(QStringLiteral("already_downloaded"), true);
        return;
    }

    if (m_activeDownloads.contains(trackId)) {
        m_statusMessage = QStringLiteral("Descarga ya en curso");
        emit statusMessageChanged();
        return;
    }

    QString dlDir = QStandardPaths::writableLocation(
        QStandardPaths::DownloadLocation);
    if (dlDir.isEmpty()) dlDir = QDir::homePath();
    QDir().mkpath(dlDir);

    QString safeTitle = title.trimmed();
    if (safeTitle.isEmpty()) safeTitle = QStringLiteral("pista_%1").arg(trackId);
    static const QRegularExpression illegalChars(QStringLiteral("[\\\\/:*?\"<>|]"));
    safeTitle.replace(illegalChars, QStringLiteral("_"));

    QString ext = format.trimmed().toLower();
    if (ext.contains(QLatin1String("flac"))) ext = QStringLiteral("flac");
    else if (ext.contains(QLatin1String("mp3"))) ext = QStringLiteral("mp3");
    else if (ext.contains(QLatin1String("aac")) || ext.contains(QLatin1String("m4a"))) ext = QStringLiteral("m4a");
    else if (ext.contains(QLatin1String("opus"))) ext = QStringLiteral("opus");
    else if (ext.contains(QLatin1String("wav"))) ext = QStringLiteral("wav");
    else if (ext.isEmpty()) ext = QStringLiteral("flac");

    const QString destPath = dlDir + QLatin1Char('/') + safeTitle +
                             QLatin1Char('.') + ext;

    m_activeDownloads.insert(trackId);

    if (!streamPath.isEmpty() && !streamPath.startsWith(QLatin1String("http"))) {
        QString localSrc = streamPath;
        if (localSrc.startsWith(QLatin1String("file://")))
            localSrc = localSrc.mid(7);
        QFile::remove(destPath);
        const bool ok = QFile::copy(localSrc, destPath);
        if (ok) {
            QSettings settings(QStringLiteral("DopamineFM"), QStringLiteral("Downloads"));
            settings.setValue(QStringLiteral("track_%1").arg(trackId), destPath);
            m_downloadedTrackIds.insert(trackId);
        }
        m_activeDownloads.remove(trackId);
        m_statusMessage = ok ? QStringLiteral("Guardado en %1").arg(destPath)
                             : QStringLiteral("Error al guardar pista");
        emit statusMessageChanged();
        emit trackDownloaded(destPath, ok);
        return;
    }

    if (!m_api) {
        m_activeDownloads.remove(trackId);
        return;
    }
    m_statusMessage = QStringLiteral("Descargando %1...").arg(safeTitle);
    emit statusMessageChanged();
    m_api->downloadTrackStream(trackId,
        [this, trackId, destPath, safeTitle](bool ok, const QByteArray &data) {
            m_activeDownloads.remove(trackId);
            bool saved = false;
            if (ok && !data.isEmpty()) {
                QFile f(destPath);
                if (f.open(QIODevice::WriteOnly)) {
                    saved = (f.write(data) == data.size());
                    f.close();
                }
            }
            if (saved) {
                QSettings settings(QStringLiteral("DopamineFM"), QStringLiteral("Downloads"));
                settings.setValue(QStringLiteral("track_%1").arg(trackId), destPath);
                m_downloadedTrackIds.insert(trackId);
            }
            m_statusMessage = saved
                ? QStringLiteral("Descarga completada: %1").arg(safeTitle)
                : QStringLiteral("Error al descargar %1").arg(safeTitle);
            emit statusMessageChanged();
            emit trackDownloaded(destPath, saved);
        });
}

void LibraryController::loadRecentTracks() {
    if (!m_api) {
        m_recentTracks.clear();
        for (int i = 0; i < qMin(20, m_tracks.size()); ++i)
            m_recentTracks.append(m_tracks[i]);
        emit recentTracksChanged();
        return;
    }
    m_api->fetchRecentTracks(20, [this](bool ok, const QJsonArray &arr) {
        m_recentTracks.clear();
        if (ok && !arr.isEmpty()) {
            for (const auto &val : arr)
                m_recentTracks.append(val.toObject().toVariantMap());
        } else {
            for (int i = 0; i < qMin(20, m_tracks.size()); ++i)
                m_recentTracks.append(m_tracks[i]);
        }
        emit recentTracksChanged();
    });
}

bool LibraryController::isTrackDownloaded(int trackId) const {
    if (trackId <= 0) return false;
    if (m_downloadedTrackIds.contains(trackId)) return true;
    QSettings settings(QStringLiteral("DopamineFM"), QStringLiteral("Downloads"));
    QString key = QStringLiteral("track_%1").arg(trackId);
    if (settings.contains(key)) {
        QString path = settings.value(key).toString();
        if (!path.isEmpty() && QFile::exists(path)) {
            m_downloadedTrackIds.insert(trackId);
            return true;
        }
    }
    return false;
}

} // namespace Dopamine::Features::Library::App
