/**
 * Location: src/core/ApiClient.cpp
 * Function: Implementation of the core HTTP network client sending POST
 * requests with application/json headers and handling reply lifecycle safely.
 * All Rights Reserved isgaar
 */
#include "ApiClient.h"
#include "ProjectConfig.h"
#include <QNetworkRequest>
#include <QUuid>
#include <QUrl>
#include <QDebug>

namespace Dopamine::Core {

ApiClient::ApiClient(QObject *parent)
    : QObject(parent), m_networkManager(this) {}

QNetworkRequest ApiClient::createRequest(const QString &url) const {
    if (!ProjectConfig::instance().isProduction()) {
        QUrl parsed(url);
        qInfo().noquote() << "[ApiClient] Consulta Dopamine Engine:"
                          << parsed.host() << ":" << parsed.port(8000)
                          << parsed.path();
    }
    QNetworkRequest request{QUrl(url)};
    request.setTransferTimeout(5000);
    request.setRawHeader("Accept", "application/json");
    if (!m_bearerToken.isEmpty()) {
        request.setRawHeader("Authorization",
                             "Bearer " + m_bearerToken.toUtf8());
    }
    QString reqId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    request.setRawHeader("X-Request-Id", reqId.toUtf8());
    return request;
}

void ApiClient::get(const QString &url, ResponseCallback callback) {
    QNetworkRequest request = createRequest(url);
    QNetworkReply *reply = m_networkManager.get(request);

    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            callback(true, reply->readAll());
        } else {
            callback(false, reply->readAll());
        }
    });
}

void ApiClient::post(const QString &url,
                     const QByteArray &jsonData,
                     ResponseCallback callback) {
    QNetworkRequest request = createRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/json; charset=utf-8");

    QNetworkReply *reply = m_networkManager.post(request, jsonData);

    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            callback(true, reply->readAll());
        } else {
            callback(false, reply->readAll());
        }
    });
}

void ApiClient::put(const QString &url,
                    const QByteArray &jsonData,
                    ResponseCallback callback) {
    QNetworkRequest request = createRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/json; charset=utf-8");
    QNetworkReply *reply = m_networkManager.put(request, jsonData);
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        reply->deleteLater();
        callback(reply->error() == QNetworkReply::NoError, reply->readAll());
    });
}

void ApiClient::deleteResource(const QString &url, ResponseCallback callback) {
    QNetworkRequest request = createRequest(url);
    QNetworkReply *reply = m_networkManager.deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        reply->deleteLater();
        callback(reply->error() == QNetworkReply::NoError, reply->readAll());
    });
}

} // namespace Dopamine::Core
