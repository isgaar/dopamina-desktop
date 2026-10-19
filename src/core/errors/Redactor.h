/**
 * Location: src/core/errors/Redactor.h
 * Function: Security utility sanitizing credentials, passwords, Bearer tokens,
 * and SMB URI credentials from logs, reports, and debug outputs.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_ERRORS_REDACTOR_H
#define DOPAMINE_CORE_ERRORS_REDACTOR_H

#include <QString>

namespace Core::Errors {

class Redactor {
public:
    static QString redact(const QString& input);
    static QString redactUri(const QString& uri);
};

} // namespace Core::Errors

#endif // DOPAMINE_CORE_ERRORS_REDACTOR_H
