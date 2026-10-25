/**
 * Location: src/core/credentials/FileCredentialStore.h
 * Function: Owner-only file credential store allowing desktop sessions to
 * survive restarts without writing tokens into QSettings or XML files.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_CREDENTIALS_FILECREDENTIALSTORE_H
#define DOPAMINE_CORE_CREDENTIALS_FILECREDENTIALSTORE_H

#include "ICredentialStore.h"
#include <QString>

namespace Dopamine::Core::Credentials {

class FileCredentialStore : public ICredentialStore {
public:
    explicit FileCredentialStore(QString directory = QString());
    ~FileCredentialStore() override = default;

    bool storeToken(const QString &service,
                    const QString &token) override;
    QString retrieveToken(const QString &service) const override;
    bool deleteToken(const QString &service) override;

private:
    QString pathFor(const QString &service) const;
    QString m_directory;
};

} // namespace Dopamine::Core::Credentials

#endif // DOPAMINE_CORE_CREDENTIALS_FILECREDENTIALSTORE_H
