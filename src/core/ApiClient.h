/**
 * Location: src/core/ApiClient.h
 * Function: Core network access adapter wrapping Qt QNetworkAccessManager
 * to perform asynchronous HTTP API requests with JSON payloads and callbacks.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_API_CLIENT_H
#define DOPAMINE_CORE_API_CLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <functional>

namespace Dopamine::Core {

class ApiClient : public QObject {
    Q_OBJECT
public:
    explicit ApiClient(QObject *parent = nullptr);
    ~ApiClient() override = default;

    using ResponseCallback =
        std::function<void(bool ok, const QByteArray &data)>;

    void get(const QString &url, ResponseCallback callback);

    void post(const QString &url,
              const QByteArray &jsonData,
              ResponseCallback callback);

    void put(const QString &url,
             const QByteArray &jsonData,
             ResponseCallback callback);

    void deleteResource(const QString &url,
                        ResponseCallback callback);

    void setBearerToken(const QString &token) { m_bearerToken = token; }
    QString bearerToken() const { return m_bearerToken; }

private:
    QNetworkRequest createRequest(const QString &url) const;
    QNetworkAccessManager m_networkManager;
    QString m_bearerToken;
};

} // namespace Dopamine::Core

#endif // DOPAMINE_CORE_API_CLIENT_H
