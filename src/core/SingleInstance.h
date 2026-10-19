/**
 * Location: src/core/SingleInstance.h
 * Function: Single-instance process lock and IPC coordination ensuring only one
 * desktop application instance executes while forwarding launch parameters.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_SINGLE_INSTANCE_H
#define DOPAMINE_CORE_SINGLE_INSTANCE_H

#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <QString>

namespace Dopamine::Core {

class SingleInstance : public QObject {
    Q_OBJECT
public:
    explicit SingleInstance(const QString &serverName,
                            QObject *parent = nullptr);
    ~SingleInstance() override = default;

    bool checkAndForward(const QString &message = QString());
    bool startListening();

signals:
    void messageReceived(const QString &message);

private slots:
    void onNewConnection();

private:
    QString m_serverName;
    QLocalServer m_server;
};

} // namespace Dopamine::Core

#endif // DOPAMINE_CORE_SINGLE_INSTANCE_H
