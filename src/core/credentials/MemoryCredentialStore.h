/**
 * Location: src/core/credentials/MemoryCredentialStore.h
 * Function: In-memory credential store fallback ensuring authorization tokens
 * are never written to disk, QSettings, or unencrypted local files.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_CREDENTIALS_MEMORYCREDENTIALSTORE_H
#define DOPAMINE_CORE_CREDENTIALS_MEMORYCREDENTIALSTORE_H

#include "ICredentialStore.h"
#include <QHash>

namespace Dopamine::Core::Credentials {

class MemoryCredentialStore : public ICredentialStore {
public:
    MemoryCredentialStore() = default;
    ~MemoryCredentialStore() override = default;

    bool storeToken(const QString &service,
                    const QString &token) override;
    QString retrieveToken(const QString &service) const override;
    bool deleteToken(const QString &service) override;

private:
    mutable QHash<QString, QString> m_tokens;
};

} // namespace Dopamine::Core::Credentials

#endif // DOPAMINE_CORE_CREDENTIALS_MEMORYCREDENTIALSTORE_H
