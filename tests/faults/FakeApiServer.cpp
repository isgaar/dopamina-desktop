/**
 * Location: tests/faults/FakeApiServer.cpp
 * Function: Implementation of the mock HTTP API server parsing HTTP requests
 * and replying with customizable status codes, headers, and payload bodies.
 * All Rights Reserved isgaar
 */
#include "FakeApiServer.h"

namespace Tests::Faults {

FakeApiServer::FakeApiServer(QObject* parent) : QObject(parent) {
    connect(&m_server, &QTcpServer::newConnection,
            this, &FakeApiServer::onNewConnection);
}

FakeApiServer::~FakeApiServer() {
    stop();
}

bool FakeApiServer::start() {
    return m_server.listen(QHostAddress::LocalHost, 0);
}

void FakeApiServer::stop() {
    if (m_server.isListening()) {
        m_server.close();
    }
}

quint16 FakeApiServer::port() const {
    return m_server.serverPort();
}

QString FakeApiServer::url() const {
    return QStringLiteral("http://127.0.0.1:%1").arg(port());
}

void FakeApiServer::setNextResponse(
    int statusCode,
    const QByteArray& body,
    const QString& extraHeader
) {
    m_statusCode = statusCode;
    m_responseBody = body;
    m_extraHeader = extraHeader;
}

void FakeApiServer::onNewConnection() {
    QTcpSocket* sock = m_server.nextPendingConnection();
    if (!sock) return;

    if (m_closeImmediately) {
        sock->abort();
        sock->deleteLater();
        return;
    }

    connect(sock, &QTcpSocket::readyRead, this, [this, sock]() {
        const QByteArray request = sock->readAll();
        if (m_lastRequest.isEmpty()) m_lastRequest = request;

        QByteArray resp;
        resp += QStringLiteral("HTTP/1.1 %1 OK\r\n").arg(m_statusCode).toUtf8();
        resp += "Content-Type: application/json\r\n";
        resp += QStringLiteral("Content-Length: %1\r\n")
            .arg(m_responseBody.size()).toUtf8();
        if (!m_extraHeader.isEmpty()) {
            resp += m_extraHeader.toUtf8() + "\r\n";
        }
        resp += "Connection: close\r\n\r\n";
        resp += m_responseBody;

        sock->write(resp);
        sock->flush();
        sock->disconnectFromHost();
    });

    connect(sock, &QTcpSocket::disconnected, sock, &QObject::deleteLater);
}

} // namespace Tests::Faults
