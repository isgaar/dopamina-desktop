/**
 * Location: src/features/library/infra/RecursiveDirectoryScanner.h
 * Function: Background thread recursive filesystem scanner discovering audio
 * files across nested directories and streaming batches in real time.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_LIBRARY_INFRA_RECURSIVEDIRECTORYSCANNER_H
#define DOPAMINE_FEATURES_LIBRARY_INFRA_RECURSIVEDIRECTORYSCANNER_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <atomic>

namespace Dopamine::Features::Library::Infra {

class RecursiveDirectoryScanner : public QObject {
    Q_OBJECT
public:
    explicit RecursiveDirectoryScanner(QObject *parent = nullptr);
    ~RecursiveDirectoryScanner() override;

    void startScan(const QString &rootPath);
    void stopScan();
    bool isScanning() const { return m_isScanning.load(); }
    static QString resolveRootPath();

signals:
    void batchDiscovered(const QVariantList &tracks);
    void scanFinished(int totalTracks);
    void scanProgress(int count, const QString &status);

private:
    std::atomic<bool> m_isScanning{false};
    std::atomic<bool> m_stopRequested{false};
};

} // namespace Dopamine::Features::Library::Infra

#endif // DOPAMINE_FEATURES_LIBRARY_INFRA_RECURSIVEDIRECTORYSCANNER_H
