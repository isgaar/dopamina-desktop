/**
 * Location: src/features/settings/infra/SourceProbe.cpp
 * Function: Implementation of the diagnostic probe validating HTTP health
 * APIs and verifying local Python and FFmpeg binary presence on the system.
 * All Rights Reserved isgaar
 */
#include "SourceProbe.h"
#include "core/ApiClient.h"
#include "core/service/LocalServiceManager.h"
#include <QDir>
#include <QTcpSocket>

namespace Dopamine::Features::Settings::Infra {

void SourceProbe::probeApi(
    Core::ApiClient *client,
    const QString &url,
    std::function<void(bool ok)> callback) {
    if (!client) { callback(false); return; }
    QString b = url.trimmed();
    if (b.endsWith(QLatin1Char('/'))) b.chop(1);
    client->get(b + QStringLiteral("/api/v1/health"),
                [callback](bool ok, const QByteArray &) {
                    callback(ok);
                });
}

QString SourceProbe::probeLocal(
    const Core::Service::LocalServiceManager *svc,
    const QString &folder) {
    if (!svc) return QStringLiteral("error");
    if (!svc->hasPython()) return QStringLiteral("no_python");
    if (!svc->hasFfmpeg()) return QStringLiteral("no_ffmpeg");
    if (folder.isEmpty() || !QDir(folder).exists()) {
        return QStringLiteral("folder_not_found");
    }
    return QStringLiteral("ok");
}

QString SourceProbe::probeSmb(const QString &host, int port) {
    QString h = host.trimmed();
    if (h.startsWith(QLatin1String("smb://"), Qt::CaseInsensitive)) {
        h = h.mid(6);
    }
    int atIdx = h.indexOf(QLatin1Char('@'));
    if (atIdx != -1) h = h.mid(atIdx + 1);
    int slashIdx = h.indexOf(QLatin1Char('/'));
    if (slashIdx != -1) h = h.left(slashIdx);
    if (h.isEmpty()) return QStringLiteral("empty_host");
    QTcpSocket sock;
    sock.connectToHost(h, port);
    if (!sock.waitForConnected(1500)) return QStringLiteral("unreachable");
    sock.disconnectFromHost();
    return QStringLiteral("ok");
}

} // namespace Dopamine::Features::Settings::Infra
