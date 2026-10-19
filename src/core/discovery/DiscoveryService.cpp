/**
 * Location: src/core/discovery/DiscoveryService.cpp
 * Function: Implementation of LAN service discovery using Avahi zero-conf
 * utility to resolve server host and port dynamically without manual setup.
 * All Rights Reserved isgaar
 */
#include "DiscoveryService.h"
#include "../ProjectConfig.h"
#include <QDebug>
#include <QStandardPaths>

namespace Dopamine::Core::Discovery {

DiscoveryService::DiscoveryService(QObject *parent)
    : QObject(parent),
      m_process(new QProcess(this)),
      m_timeoutTimer(new QTimer(this)) {
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout,
            this, &DiscoveryService::handleTimeout);
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &DiscoveryService::handleProcessOutput);
    connect(m_process, &QProcess::finished,
            this, &DiscoveryService::handleProcessFinished);
}

DiscoveryService::~DiscoveryService() {
    stopDiscovery();
}

void DiscoveryService::startDiscovery() {
    if (m_isDiscovering) return;
    stopDiscovery();

    QString bin = QStandardPaths::findExecutable("avahi-browse");
    if (bin.isEmpty()) {
        bin = "/usr/bin/avahi-browse";
    }
    QStringList args;
    args << "-rtp" << "_dopaminefm._tcp";

    m_isDiscovering = true;
    m_isDiscovered = false;
    emit discoveryStateChanged();

    m_timeoutTimer->start(3500);
    m_process->start(bin, args);
}

void DiscoveryService::stopDiscovery() {
    m_timeoutTimer->stop();
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(500)) {
            m_process->kill();
        }
    }
    if (m_isDiscovering) {
        m_isDiscovering = false;
        emit discoveryStateChanged();
        emit discoveryFinished();
    }
}

void DiscoveryService::handleProcessOutput() {
    while (m_process->canReadLine()) {
        QString line = QString::fromUtf8(m_process->readLine()).trimmed();
        parseAvahiLine(line);
    }
}

void DiscoveryService::parseAvahiLine(const QString &line) {
    if (!line.startsWith("=") && !line.startsWith("+")) return;
    QStringList parts = line.split(';');
    if (parts.size() >= 9) {
        QString host = parts[6].trimmed();
        int port = parts[8].trimmed().toInt();
        if (!host.isEmpty() && port > 0) {
            m_discoveredHost = host;
            m_discoveredPort = port;
            m_isDiscovered = true;
            QString apiUrl = QString("http://%1:%2").arg(host).arg(port);
            ProjectConfig::instance().setApiUrl(apiUrl);
            qInfo() << "[DiscoveryService] Servidor detectado:" << apiUrl;
            emit serverDiscovered(host, port);
            emit discoveryStateChanged();
            stopDiscovery();
        }
    }
}

void DiscoveryService::handleProcessFinished(int exitCode,
                                            QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
    if (m_isDiscovering) {
        m_isDiscovering = false;
        emit discoveryStateChanged();
        emit discoveryFinished();
    }
}

void DiscoveryService::handleTimeout() {
    if (m_isDiscovering) {
        qInfo() << "[DiscoveryService] Timeout sin detección, usando fallback.";
        stopDiscovery();
    }
}

} // namespace Dopamine::Core::Discovery
