/**
 * Location: src/core/storage/SmbStorageProvider.h
 * Function: SMB remote share storage provider orchestrating TCP reachability,
 * root descriptor sync, and credentials isolation for audio libraries.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_STORAGE_SMBSTORAGEPROVIDER_H
#define DOPAMINE_CORE_STORAGE_SMBSTORAGEPROVIDER_H

#include "IStorageProvider.h"
#include <QString>

namespace Dopamine::Core::Storage {

class SmbStorageProvider : public IStorageProvider {
public:
    SmbStorageProvider(const QString &host = {},
                       const QString &share = {},
                       const QString &folder = {},
                       const QString &user = {},
                       const QString &mountOverride = {});
    ~SmbStorageProvider() override = default;

    bool connect() override;
    void disconnect() override;
    bool isConnected() const override { return m_connected; }
    bool probe() override;
    QString rootPath() const override;
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
    QString m_host;
    QString m_share;
    QString m_folder;
    QString m_user;
    QString m_mountOverride;
    bool m_connected = false;
    mutable QString m_lastError;
};

} // namespace Dopamine::Core::Storage

#endif // DOPAMINE_CORE_STORAGE_SMBSTORAGEPROVIDER_H
