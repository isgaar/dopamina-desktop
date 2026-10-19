/**
 * Location: src/core/credentials/MemoryCredentialStore.cpp
 * Function: Thread-safe in-memory credential storage implementation for API
 * bearer tokens and ephemeral authentication secrets across client sessions.
 * All Rights Reserved isgaar
 */
#include "MemoryCredentialStore.h"

namespace Dopamine::Core::Credentials {

bool MemoryCredentialStore::storeToken(const QString &service,
                                       const QString &token) {
    if (service.isEmpty()) {
        return false;
    }
    m_tokens.insert(service, token);
    return true;
}

QString MemoryCredentialStore::retrieveToken(const QString &service) const {
    return m_tokens.value(service);
}

bool MemoryCredentialStore::deleteToken(const QString &service) {
    return m_tokens.remove(service) > 0;
}

} // namespace Dopamine::Core::Credentials
