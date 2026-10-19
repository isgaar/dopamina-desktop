/**
 * Location: src/core/connection/ConnectionService.h
 * Function: Central state machine managing connectivity status, degradation
 * levels, automatic health probes, backoff retries, and QML view properties.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_CONNECTION_CONNECTIONSERVICE_H
#define DOPAMINE_CORE_CONNECTION_CONNECTIONSERVICE_H

#include <QObject>
#include <QTimer>
#include "ConnectionReport.h"
#include "CircuitBreaker.h"
#include "ProbeRunner.h"

namespace Core::Connection {

enum class ConnectionState {
    Unconfigured = 0,
    Checking,
    Connected,
    Degraded,
    Reconnecting,
    Disconnected,
    AuthFailed
};

class ConnectionService : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString stateString READ stateString NOTIFY stateChanged)
    Q_PROPERTY(QString levelString READ levelString NOTIFY stateChanged)
    Q_PROPERTY(int level READ level NOTIFY stateChanged)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY stateChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY stateChanged)

public:
    explicit ConnectionService(QObject* parent = nullptr);

    ConnectionState state() const { return m_state; }
    QString stateString() const;
    QString levelString() const;
    int level() const { return static_cast<int>(m_report.level); }
    bool isConnected() const;
    QString statusMessage() const;
    const ConnectionReport& report() const { return m_report; }

    Q_INVOKABLE void checkConnection();
    Q_INVOKABLE void retry();
    void setUnconfigured();
    void updateReport(const ConnectionReport& report);

signals:
    void stateChanged();
    void connectionChanged(
        ConnectionState from,
        ConnectionState to,
        Core::Errors::ErrorCode code,
        const ConnectionReport& report
    );

private slots:
    void onRetryTimer();

private:
    void setState(ConnectionState newState);

    ConnectionState m_state{ConnectionState::Unconfigured};
    ConnectionReport m_report;
    CircuitBreaker m_circuitBreaker;
    ProbeRunner m_probes;
    QTimer m_retryTimer;
};

} // namespace Core::Connection

#endif // DOPAMINE_CORE_CONNECTION_CONNECTIONSERVICE_H
