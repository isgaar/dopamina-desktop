/**
 * Location: src/core/discovery/DiscoveryService.h
 * Function: Zero-configuration mDNS client resolving Dopamine companion
 * servers on the local area network and configuring connection settings safely.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_DISCOVERY_DISCOVERYSERVICE_H
#define DOPAMINE_CORE_DISCOVERY_DISCOVERYSERVICE_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QTimer>

namespace Dopamine::Core::Discovery {

class DiscoveryService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isDiscovering READ isDiscovering
               NOTIFY discoveryStateChanged)
    Q_PROPERTY(bool isDiscovered READ isDiscovered
               NOTIFY discoveryStateChanged)
    Q_PROPERTY(QString discoveredHost READ discoveredHost
               NOTIFY discoveryStateChanged)
    Q_PROPERTY(int discoveredPort READ discoveredPort
               NOTIFY discoveryStateChanged)

public:
    explicit DiscoveryService(QObject *parent = nullptr);
    ~DiscoveryService() override;

    bool isDiscovering() const { return m_isDiscovering; }
    bool isDiscovered() const { return m_isDiscovered; }
    QString discoveredHost() const { return m_discoveredHost; }
    int discoveredPort() const { return m_discoveredPort; }

    Q_INVOKABLE void startDiscovery();
    Q_INVOKABLE void stopDiscovery();

signals:
    void discoveryStateChanged();
    void serverDiscovered(const QString &host, int port);
    void discoveryFinished();

private slots:
    void handleProcessOutput();
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleTimeout();

private:
    void parseAvahiLine(const QString &line);

    QProcess *m_process = nullptr;
    QTimer *m_timeoutTimer = nullptr;
    QString m_discoveredHost;
    int m_discoveredPort = 8000;
    bool m_isDiscovering = false;
    bool m_isDiscovered = false;
};

} // namespace Dopamine::Core::Discovery

#endif // DOPAMINE_CORE_DISCOVERY_DISCOVERYSERVICE_H
