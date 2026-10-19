/**
 * Location: tests/faults/FakeApiServer.h
 * Function: Lightweight mock HTTP API server based on QTcpServer for injecting
 * fault scenarios S01-S14, rate limits, invalid JSON, and server dropouts.
 * All Rights Reserved isgaar
 */
#ifndef TESTS_FAULTS_FAKEAPISERVER_H
#define TESTS_FAULTS_FAKEAPISERVER_H

#include <QByteArray>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

namespace Tests::Faults {

class FakeApiServer : public QObject {
    Q_OBJECT
public:
    explicit FakeApiServer(QObject* parent = nullptr);
    ~FakeApiServer() override;

    bool start();
    void stop();
    quint16 port() const;
    QString url() const;

    void setNextResponse(
        int statusCode,
        const QByteArray& body,
        const QString& extraHeader = QString()
    );
    void setCloseImmediately(bool b) { m_closeImmediately = b; }

private slots:
    void onNewConnection();

private:
    QTcpServer m_server;
    int m_statusCode{200};
    QByteArray m_responseBody{R"({"status":"ok","api_version":"1.0.0"})"};
    QString m_extraHeader;
    bool m_closeImmediately{false};
};

} // namespace Tests::Faults

#endif // TESTS_FAULTS_FAKEAPISERVER_H
