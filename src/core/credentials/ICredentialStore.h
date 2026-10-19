/**
 * Location: src/core/credentials/ICredentialStore.h
 * Function: Abstract interface for secure credential storage preventing token
 * leaks in configuration files, plaintext logs, and serialization formats.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_CREDENTIALS_ICREDENTIALSTORE_H
#define DOPAMINE_CORE_CREDENTIALS_ICREDENTIALSTORE_H

#include <QString>

namespace Dopamine::Core::Credentials {

class ICredentialStore {
public:
    virtual ~ICredentialStore() = default;

    virtual bool storeToken(const QString &service,
                            const QString &token) = 0;
    virtual QString retrieveToken(const QString &service) const = 0;
    virtual bool deleteToken(const QString &service) = 0;
};

} // namespace Dopamine::Core::Credentials

#endif // DOPAMINE_CORE_CREDENTIALS_ICREDENTIALSTORE_H
