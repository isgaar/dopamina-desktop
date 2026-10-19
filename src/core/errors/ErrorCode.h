/**
 * Location: src/core/errors/ErrorCode.h
 * Function: Strongly typed error code enumeration for the DopamineFM error
 * catalog spanning connection, auth, source, audio, and API failure domains.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_ERRORS_ERRORCODE_H
#define DOPAMINE_CORE_ERRORS_ERRORCODE_H

#include <QString>

namespace Core::Errors {

enum class ErrorCode {
    NoError = 0,
    ConnUnconfigured,
    ConnNetworkDown,
    ConnDns,
    ConnTimeout,
    ConnRefused,
    ConnLost,
    ConnSmbDialect,
    ConnTls,
    AuthBadCredentials,
    AuthAccountState,
    AuthConflict,
    AuthToken,
    AuthKeyring,
    SrcShareNotFound,
    SrcPathNotFound,
    SrcAccessDenied,
    SrcFileMissing,
    SrcIndexStale,
    SrcTraversal,
    AudDecode,
    AudFormat,
    AudDeviceLost,
    AudUnderrun,
    AudSeek,
    AudEqInvalid,
    ApiVersion,
    ApiRateLimit,
    ApiBusy,
    ApiBadResponse,
    MixSeedUnknown,
    MixPoolEmpty,
    MixRefill,
    PlDuplicate,
    PlUnavailable,
    PlConflict,
    PlPending,
    DmnRestart,
    DmnBacklog,
    CfgInvalid,
    UiStall
};

QString errorCodeToString(ErrorCode c);

} // namespace Core::Errors

#endif // DOPAMINE_CORE_ERRORS_ERRORCODE_H
