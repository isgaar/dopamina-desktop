/**
 * Location: src/core/platform/UiWatchdog.h
 * Function: UI thread responsiveness monitor checking tick delays every 50ms
 * and emitting ERR_UI_STALL warnings if event loop stalls exceed 250ms.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_PLATFORM_UIWATCHDOG_H
#define DOPAMINE_CORE_PLATFORM_UIWATCHDOG_H

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

namespace Core::Platform {

class UiWatchdog : public QObject {
    Q_OBJECT
public:
    explicit UiWatchdog(QObject* parent = nullptr);

    void start(int tickIntervalMs = 50, int thresholdMs = 250);
    void stop();
    void setLastOperation(const QString& op);

signals:
    void stallDetected(qint64 delayMs, const QString& lastOp);

private slots:
    void onTick();

private:
    QTimer m_timer;
    QElapsedTimer m_elapsed;
    int m_thresholdMs{250};
    QString m_lastOp{QStringLiteral("idle")};
};

} // namespace Core::Platform

#endif // DOPAMINE_CORE_PLATFORM_UIWATCHDOG_H
