/**
 * Location: src/core/storage/IStorageProvider.h
 * Function: Abstract storage provider interface governing SMB and local
 * filesystem storage lifecycle, reachability probes, and file sync.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_STORAGE_ISTORAGEPROVIDER_H
#define DOPAMINE_CORE_STORAGE_ISTORAGEPROVIDER_H

#include <QByteArray>
#include <QString>

namespace Dopamine::Core::Storage {

class IStorageProvider {
public:
    virtual ~IStorageProvider() = default;

    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual bool probe() = 0;
    virtual QString rootPath() const = 0;
    virtual bool writeRootFile(const QString &fileName,
                               const QByteArray &data) = 0;
    virtual QByteArray readRootFile(const QString &fileName) const = 0;
    virtual bool existsRootFile(const QString &fileName) const = 0;
    virtual QString lastError() const = 0;
};

} // namespace Dopamine::Core::Storage

#endif // DOPAMINE_CORE_STORAGE_ISTORAGEPROVIDER_H
