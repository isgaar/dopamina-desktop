/**
 * Location: src/core/storage/LocalStorageProvider.h
 * Function: Local filesystem storage provider implementation managing root
 * audio library paths, server descriptor sync, and atomic manifests.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_STORAGE_LOCALSTORAGEPROVIDER_H
#define DOPAMINE_CORE_STORAGE_LOCALSTORAGEPROVIDER_H

#include "IStorageProvider.h"
#include <QString>

namespace Dopamine::Core::Storage {

class LocalStorageProvider : public IStorageProvider {
public:
    explicit LocalStorageProvider(const QString &folderPath = {});
    ~LocalStorageProvider() override = default;

    bool connect() override;
    void disconnect() override;
    bool isConnected() const override { return m_connected; }
    bool probe() override;
    QString rootPath() const override { return m_folderPath; }
    bool writeRootFile(const QString &fileName,
                       const QByteArray &data) override;
    QByteArray readRootFile(const QString &fileName) const override;
    bool existsRootFile(const QString &fileName) const override;
    QString lastError() const override { return m_lastError; }

    bool ensureServerDescriptor(const QString &serverId,
                                const QString &libraryName,
                                const QString &apiEndpoint =
                                    QStringLiteral("/api/v1"));

private:
    QString m_folderPath;
    bool m_connected = false;
    mutable QString m_lastError;
};

} // namespace Dopamine::Core::Storage

#endif // DOPAMINE_CORE_STORAGE_LOCALSTORAGEPROVIDER_H
