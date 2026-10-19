/**
 * Location: src/features/library/infra/RecursiveDirectoryScanner.cpp
 * Function: Implementation of asynchronous directory scanner yielding batches
 * of discovered tracks from subdirectories to keep UI fluid and responsive.
 * All Rights Reserved isgaar
 */
#include "RecursiveDirectoryScanner.h"
#include "core/ProjectConfig.h"
#include "core/storage/SmbStorageProvider.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QMetaObject>
#include <QThread>

namespace Dopamine::Features::Library::Infra {

RecursiveDirectoryScanner::RecursiveDirectoryScanner(QObject *parent)
    : QObject(parent) {}

RecursiveDirectoryScanner::~RecursiveDirectoryScanner() { stopScan(); }

void RecursiveDirectoryScanner::stopScan() { m_stopRequested.store(true); }

QString RecursiveDirectoryScanner::resolveRootPath() {
    auto &cfg = Core::ProjectConfig::instance();
    if (cfg.sourceType() == QLatin1String("smb")) {
        Core::Storage::SmbStorageProvider prov(
            cfg.smbHost(), cfg.smbShare(), cfg.smbFolder(), cfg.smbUser());
        return prov.rootPath();
    }
    return cfg.sourceType() == QLatin1String("local") ?
        cfg.localPath() : QString{};
}

void RecursiveDirectoryScanner::startScan(const QString &rootPath) {
    if (m_isScanning.load()) stopScan();
    m_stopRequested.store(false);
    m_isScanning.store(true);

    QThread *thread = QThread::create([this, rootPath]() {
        QStringList filters = {
            QStringLiteral("*.flac"), QStringLiteral("*.mp3"),
            QStringLiteral("*.m4a"), QStringLiteral("*.wav"),
            QStringLiteral("*.ogg"), QStringLiteral("*.opus"),
            QStringLiteral("*.aac"), QStringLiteral("*.wma")
        };
        QDirIterator it(rootPath, filters, QDir::Files,
                        QDirIterator::Subdirectories);
        QVariantList batch; int total = 0;
        while (it.hasNext() && !m_stopRequested.load()) {
            QString filePath = it.next();
            QFileInfo fi(filePath);
            QString title = fi.completeBaseName();
            QString relPath = QDir(rootPath).relativeFilePath(filePath);
            QStringList parts = relPath.split(QLatin1Char('/'),
                                              Qt::SkipEmptyParts);
            QString artist = QStringLiteral("Varios Artistas");
            QString album = QStringLiteral("Álbum Desconocido");
            if (parts.size() >= 3) {
                artist = parts[0]; album = parts[1];
            } else if (parts.size() == 2) {
                artist = parts[0]; album = parts[0];
            }
            QVariantMap track;
            track[QStringLiteral("id")] = static_cast<int>(qHash(filePath));
            track[QStringLiteral("titulo")] = title;
            track[QStringLiteral("title")] = title;
            track[QStringLiteral("artista")] = artist;
            track[QStringLiteral("artist")] = artist;
            track[QStringLiteral("album")] = album;
            track[QStringLiteral("path")] = filePath;
            track[QStringLiteral("duracion")] = 210;
            batch.append(track); total++;

            if (batch.size() >= 20) {
                QVariantList copy = batch; batch.clear();
                QMetaObject::invokeMethod(this, [this, copy, total]() {
                    emit batchDiscovered(copy);
                    emit scanProgress(total, QStringLiteral("scanning"));
                }, Qt::QueuedConnection);
            }
        }
        if (!batch.isEmpty() && !m_stopRequested.load()) {
            QVariantList copy = batch;
            QMetaObject::invokeMethod(this, [this, copy, total]() {
                emit batchDiscovered(copy);
            }, Qt::QueuedConnection);
        }
        m_isScanning.store(false);
        QMetaObject::invokeMethod(this, [this, total]() {
            emit scanFinished(total);
        }, Qt::QueuedConnection);
    });
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

} // namespace Dopamine::Features::Library::Infra
