/**
 * Location: src/core/errors/ErrorCode.cpp
 * Function: Implementation of error code stringifier mapping error enum
 * values to canonical ERR_* diagnostic tokens matching localized catalogs.
 * All Rights Reserved isgaar
 */
#include "ErrorCode.h"

namespace Core::Errors {

QString errorCodeToString(ErrorCode c) {
    switch (c) {
    case ErrorCode::ConnUnconfigured:
        return QStringLiteral("ERR_CONN_UNCONFIGURED");
    case ErrorCode::ConnNetworkDown:
        return QStringLiteral("ERR_CONN_NETWORK_DOWN");
    case ErrorCode::ConnDns:
        return QStringLiteral("ERR_CONN_DNS");
    case ErrorCode::ConnTimeout:
        return QStringLiteral("ERR_CONN_TIMEOUT");
    case ErrorCode::ConnRefused:
        return QStringLiteral("ERR_CONN_REFUSED");
    case ErrorCode::ConnLost:
        return QStringLiteral("ERR_CONN_LOST");
    case ErrorCode::ConnSmbDialect:
        return QStringLiteral("ERR_CONN_SMB_DIALECT");
    case ErrorCode::ConnTls:
        return QStringLiteral("ERR_CONN_TLS");
    case ErrorCode::AuthBadCredentials:
        return QStringLiteral("ERR_AUTH_BAD_CREDENTIALS");
    case ErrorCode::AuthAccountState:
        return QStringLiteral("ERR_AUTH_ACCOUNT_STATE");
    case ErrorCode::AuthConflict:
        return QStringLiteral("ERR_AUTH_CONFLICT");
    case ErrorCode::AuthToken:
        return QStringLiteral("ERR_AUTH_TOKEN");
    case ErrorCode::AuthKeyring:
        return QStringLiteral("ERR_AUTH_KEYRING");
    case ErrorCode::SrcShareNotFound:
        return QStringLiteral("ERR_SRC_SHARE_NOT_FOUND");
    case ErrorCode::SrcPathNotFound:
        return QStringLiteral("ERR_SRC_PATH_NOT_FOUND");
    case ErrorCode::SrcAccessDenied:
        return QStringLiteral("ERR_SRC_ACCESS_DENIED");
    case ErrorCode::SrcFileMissing:
        return QStringLiteral("ERR_SRC_FILE_MISSING");
    case ErrorCode::SrcIndexStale:
        return QStringLiteral("ERR_SRC_INDEX_STALE");
    case ErrorCode::SrcTraversal:
        return QStringLiteral("ERR_SRC_TRAVERSAL");
    case ErrorCode::AudDecode:
        return QStringLiteral("ERR_AUD_DECODE");
    case ErrorCode::AudFormat:
        return QStringLiteral("ERR_AUD_FORMAT");
    case ErrorCode::AudDeviceLost:
        return QStringLiteral("ERR_AUD_DEVICE_LOST");
    case ErrorCode::AudUnderrun:
        return QStringLiteral("ERR_AUD_UNDERRUN");
    case ErrorCode::AudSeek:
        return QStringLiteral("ERR_AUD_SEEK");
    case ErrorCode::AudEqInvalid:
        return QStringLiteral("ERR_AUD_EQ_INVALID");
    case ErrorCode::ApiVersion:
        return QStringLiteral("ERR_API_VERSION");
    case ErrorCode::ApiRateLimit:
        return QStringLiteral("ERR_API_RATE_LIMIT");
    case ErrorCode::ApiBusy:
        return QStringLiteral("ERR_API_BUSY");
    case ErrorCode::ApiBadResponse:
        return QStringLiteral("ERR_API_BAD_RESPONSE");
    case ErrorCode::MixSeedUnknown:
        return QStringLiteral("ERR_MIX_SEED_UNKNOWN");
    case ErrorCode::MixPoolEmpty:
        return QStringLiteral("ERR_MIX_POOL_EMPTY");
    case ErrorCode::MixRefill:
        return QStringLiteral("ERR_MIX_REFILL");
    case ErrorCode::PlDuplicate:
        return QStringLiteral("ERR_PL_DUPLICATE");
    case ErrorCode::PlUnavailable:
        return QStringLiteral("ERR_PL_UNAVAILABLE");
    case ErrorCode::PlConflict:
        return QStringLiteral("ERR_PL_CONFLICT");
    case ErrorCode::PlPending:
        return QStringLiteral("ERR_PL_PENDING");
    case ErrorCode::DmnRestart:
        return QStringLiteral("ERR_DMN_RESTART");
    case ErrorCode::DmnBacklog:
        return QStringLiteral("ERR_DMN_BACKLOG");
    case ErrorCode::CfgInvalid:
        return QStringLiteral("ERR_CFG_INVALID");
    case ErrorCode::UiStall:
        return QStringLiteral("ERR_UI_STALL");
    default:
        return QStringLiteral("ERR_NONE");
    }
}

} // namespace Core::Errors
