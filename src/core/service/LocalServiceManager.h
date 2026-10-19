/**
 * Location: src/core/service/LocalServiceManager.h
 * Function: Process manager controlling local Python background music engine,
 * verifying environment dependencies, and launching child server processes.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_SERVICE_LOCALSERVICEMANAGER_H
#define DOPAMINE_CORE_SERVICE_LOCALSERVICEMANAGER_H

#include <QObject>
#include <QProcess>
#include <QString>

namespace Dopamine::Core::Service {

class LocalServiceManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY statusChanged)
    Q_PROPERTY(bool hasPython READ hasPython CONSTANT)
    Q_PROPERTY(bool hasFfmpeg READ hasFfmpeg CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY statusChanged)

public:
    explicit LocalServiceManager(QObject *parent = nullptr);
    ~LocalServiceManager() override;

    bool hasPython() const;
    bool hasFfmpeg() const;
    QString pythonPath() const;
    QString ffmpegPath() const;
    bool isRunning() const;
    QString lastError() const { return m_lastError; }

    bool start(const QString &musicDir, int port = 8000);
    void stop();

signals:
    void statusChanged();
    void errorOccurred(const QString &err);

private:
    void resolveBinaries();

    QString m_pythonPath;
    QString m_ffmpegPath;
    QString m_lastError;
    QProcess *m_process = nullptr;
};

} // namespace Dopamine::Core::Service

#endif // DOPAMINE_CORE_SERVICE_LOCALSERVICEMANAGER_H
