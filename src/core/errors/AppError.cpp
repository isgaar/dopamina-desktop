/**
 * Location: src/core/errors/AppError.cpp
 * Function: Logic determining retry strategies (Automatic, Manual, None)
 * based on the DopamineFM architectural error catalog guidelines.
 * All Rights Reserved isgaar
 */
#include "AppError.h"

namespace Core::Errors {

RetryMode AppError::defaultRetryMode(ErrorCode code) {
    switch (code) {
    case ErrorCode::ConnNetworkDown:
    case ErrorCode::ConnDns:
    case ErrorCode::ConnTimeout:
    case ErrorCode::ConnRefused:
    case ErrorCode::ConnLost:
    case ErrorCode::AudUnderrun:
    case ErrorCode::ApiRateLimit:
    case ErrorCode::ApiBusy:
    case ErrorCode::MixPoolEmpty:
    case ErrorCode::MixRefill:
    case ErrorCode::PlConflict:
    case ErrorCode::PlPending:
    case ErrorCode::DmnRestart:
        return RetryMode::Automatic;

    case ErrorCode::ConnTls:
    case ErrorCode::AuthBadCredentials:
    case ErrorCode::AuthConflict:
    case ErrorCode::AuthToken:
    case ErrorCode::SrcShareNotFound:
    case ErrorCode::SrcPathNotFound:
    case ErrorCode::SrcIndexStale:
    case ErrorCode::AudDeviceLost:
    case ErrorCode::PlDuplicate:
    case ErrorCode::PlUnavailable:
        return RetryMode::Manual;

    default:
        return RetryMode::None;
    }
}

} // namespace Core::Errors
