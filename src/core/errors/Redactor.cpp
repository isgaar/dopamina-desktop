/**
 * Location: src/core/errors/Redactor.cpp
 * Function: Implementation of data sanitation replacing sensitive tokens,
 * passwords, and embedded network credentials with secure placeholders.
 * All Rights Reserved isgaar
 */
#include "Redactor.h"
#include <QRegularExpression>

namespace Core::Errors {

QString Redactor::redact(const QString& input) {
    if (input.isEmpty()) return input;
    QString out = input;

    // Redact password=... or pwd=...
    static const QRegularExpression passRe(
        QStringLiteral(
            R"((?i)(password|passwd|pwd|secret)\s*[:=]\s*([^\s,;&]+))"
        )
    );
    out.replace(passRe, QStringLiteral(R"(\1=[REDACTED])"));

    // Redact Authorization: Bearer ...
    static const QRegularExpression authRe(
        QStringLiteral(
            R"((?i)(bearer|token|auth)\s+([a-zA-Z0-9_\-\.]{8,}))"
        )
    );
    out.replace(authRe, QStringLiteral(R"(\1 [REDACTED])"));

    return redactUri(out);
}

QString Redactor::redactUri(const QString& uri) {
    if (uri.isEmpty()) return uri;
    QString out = uri;

    // smb://user:pass@host/share or smb://user@host/share
    static const QRegularExpression smbCredsRe(
        QStringLiteral(
            R"((smb|http|https)://([^:@/\s]+)(?::([^@/\s]+))?@)"
        )
    );
    out.replace(smbCredsRe, QStringLiteral(R"(\1://[USER]@)"));

    return out;
}

} // namespace Core::Errors
