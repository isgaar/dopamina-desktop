/**
 * Location: src/core/errors/AppError.h
 * Function: Rich error value object encapsulating catalog ErrorCode, retry
 * policies, user-facing error messages, and telemetry request correlation IDs.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_ERRORS_APPERROR_H
#define DOPAMINE_CORE_ERRORS_APPERROR_H

#include "ErrorCode.h"
#include <QString>

namespace Core::Errors {

enum class RetryMode {
    None = 0,
    Automatic,
    Manual
};

struct AppError {
    ErrorCode code{ErrorCode::NoError};
    QString message;
    QString requestId;
    RetryMode retryMode{RetryMode::None};

    bool isError() const { return code != ErrorCode::NoError; }
    QString codeString() const { return errorCodeToString(code); }
    static RetryMode defaultRetryMode(ErrorCode code);
};

} // namespace Core::Errors

#endif // DOPAMINE_CORE_ERRORS_APPERROR_H
