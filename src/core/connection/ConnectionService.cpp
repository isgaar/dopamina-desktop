/**
 * Location: src/core/connection/ConnectionService.cpp
 * Function: Implementation of the connection coordinator orchestrating
 * live probe checks, retry backoffs, and service degradation transitions.
 * All Rights Reserved isgaar
 */
#include "ConnectionService.h"
#include "core/logging/JsonLogger.h"

namespace Core::Connection {

ConnectionService::ConnectionService(QObject* parent) : QObject(parent) {
    m_retryTimer.setSingleShot(true);
    connect(&m_retryTimer, &QTimer::timeout,
            this, &ConnectionService::onRetryTimer);
}

QString ConnectionService::stateString() const {
    switch (m_state) {
    case ConnectionState::Unconfigured:  return QStringLiteral("SinConfigurar");
    case ConnectionState::Checking:      return QStringLiteral("Verificando");
    case ConnectionState::Connected:     return QStringLiteral("Conectado");
    case ConnectionState::Degraded:      return QStringLiteral("Degradado");
    case ConnectionState::Reconnecting:  return QStringLiteral("Reconectando");
    case ConnectionState::Disconnected:  return QStringLiteral("SinConexion");
    case ConnectionState::AuthFailed:    return QStringLiteral("AuthFallida");
    }
    return QStringLiteral("Desconocido");
}

QString ConnectionService::levelString() const {
    switch (m_report.level) {
    case ServiceLevel::Full:      return QStringLiteral("Completo");
    case ServiceLevel::SmbOnly:   return QStringLiteral("Solo SMB");
    case ServiceLevel::CacheOnly: return QStringLiteral("Sin conexión");
    }
    return QStringLiteral("Sin conexión");
}

bool ConnectionService::isConnected() const {
    return m_state == ConnectionState::Connected
        || m_state == ConnectionState::Degraded;
}

QString ConnectionService::statusMessage() const {
    if (m_state == ConnectionState::AuthFailed)
        return QStringLiteral("Credenciales incorrectas");
    if (m_state == ConnectionState::Degraded)
        return QStringLiteral("Modo degradado (Solo SMB)");
    if (m_state == ConnectionState::Disconnected)
        return QStringLiteral("Sin conexión con el servidor");
    return levelString();
}


void ConnectionService::setState(ConnectionState newState) {
    if (m_state == newState) return;
    ConnectionState oldState = m_state;
    m_state = newState;
    emit stateChanged();
    emit connectionChanged(oldState, newState, m_report.code, m_report);

    Logging::JsonLogger::instance().log(
        Logging::LogLevel::Info,
        m_report.code,
        QStringLiteral("ConnectionService"),
        QStringLiteral("state_transition"),
        0, QString(),
        QStringLiteral("%1 -> %2")
            .arg(static_cast<int>(oldState))
            .arg(static_cast<int>(newState))
    );

}

void ConnectionService::setUnconfigured() {
    m_report = ConnectionReport();
    m_retryTimer.stop();
    setState(ConnectionState::Unconfigured);
}

void ConnectionService::updateReport(const ConnectionReport& report) {
    m_report = report;
    if (report.code == Core::Errors::ErrorCode::AuthBadCredentials) {
        m_circuitBreaker.recordFailure(true);
        m_retryTimer.stop(); // Zero automatic retries
        setState(ConnectionState::AuthFailed);
        return;
    }
    if (report.level == ServiceLevel::Full) {
        m_circuitBreaker.recordSuccess();
        m_retryTimer.stop();
        setState(ConnectionState::Connected);
    } else if (report.level == ServiceLevel::SmbOnly) {
        m_circuitBreaker.recordSuccess();
        m_retryTimer.stop();
        setState(ConnectionState::Degraded);
    } else {
        m_circuitBreaker.recordFailure(false);
        setState(ConnectionState::Disconnected);
        int delay = m_circuitBreaker.nextBackoffMs();
        m_retryTimer.start(delay);
    }
}

void ConnectionService::checkConnection() {
    setState(ConnectionState::Checking);
}

void ConnectionService::retry() {
    m_circuitBreaker.reset();
    checkConnection();
}

void ConnectionService::onRetryTimer() {
    if (m_state == ConnectionState::AuthFailed) return;
    setState(ConnectionState::Reconnecting);
}

} // namespace Core::Connection
