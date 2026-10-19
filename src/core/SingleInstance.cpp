/**
 * Location: src/core/SingleInstance.cpp
 * Function: Implementation of single-instance IPC communicating across
 * desktop processes to pass command-line arguments and deep-links in real time.
 * All Rights Reserved isgaar
 */
#include "SingleInstance.h"

namespace Dopamine::Core {

SingleInstance::SingleInstance(const QString &serverName, QObject *parent)
    : QObject(parent), m_serverName(serverName), m_server(this) {
    connect(&m_server, &QLocalServer::newConnection,
            this, &SingleInstance::onNewConnection);
}

bool SingleInstance::checkAndForward(const QString &message) {
    QLocalSocket socket;
    socket.connectToServer(m_serverName);
    if (socket.waitForConnected(500)) {
        if (!message.isEmpty()) {
            socket.write(message.toUtf8());
            socket.waitForBytesWritten(500);
        }
        socket.disconnectFromServer();
        return true;
    }
    return false;
}

bool SingleInstance::startListening() {
    QLocalServer::removeServer(m_serverName);
    return m_server.listen(m_serverName);
}

void SingleInstance::onNewConnection() {
    QLocalSocket *clientSocket = m_server.nextPendingConnection();
    if (!clientSocket) return;

    connect(clientSocket, &QLocalSocket::readyRead,
            this, [this, clientSocket]() {
        QByteArray data = clientSocket->readAll();
        QString message = QString::fromUtf8(data).trimmed();
        if (!message.isEmpty()) {
            emit messageReceived(message);
        }
        clientSocket->disconnectFromServer();
        clientSocket->deleteLater();
    });
}

} // namespace Dopamine::Core
