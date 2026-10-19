/**
 * Location: src/core/storage/WatchdogRunner.h
 * Function: Timeout guardian executing storage calls on worker threads and
 * abandoning poisoned blocked threads to prevent UI freezes on remote I/O.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_STORAGE_WATCHDOGRUNNER_H
#define DOPAMINE_CORE_STORAGE_WATCHDOGRUNNER_H

#include <QFutureWatcher>
#include <QObject>
#include <QTimer>
#include <QtConcurrent>
#include <functional>
#include "core/errors/ErrorCode.h"

namespace Core::Storage {

class WatchdogRunner : public QObject {
public:
    explicit WatchdogRunner(QObject* parent = nullptr) : QObject(parent) {}

    template <typename Func, typename Callback>
    void executeWithDeadline(Func task, int timeoutMs, Callback onFinished) {
        auto* watcher = new QFutureWatcher<bool>(this);
        auto* timer = new QTimer(this);
        timer->setSingleShot(true);

        auto cleanup = [watcher, timer]() {
            timer->stop();
            timer->deleteLater();
            watcher->deleteLater();
        };

        connect(watcher, &QFutureWatcher<bool>::finished, this,
                [watcher, onFinished, cleanup]() {
            bool success = watcher->result();
            cleanup();
            onFinished(success, Core::Errors::ErrorCode::NoError);
        });

        connect(timer, &QTimer::timeout, this,
                [watcher, onFinished, cleanup]() {
            disconnect(watcher, nullptr, nullptr, nullptr);
            cleanup();
            // Abandon poisoned worker thread and notify timeout error
            onFinished(false, Core::Errors::ErrorCode::ConnTimeout);
        });

        timer->start(timeoutMs);
        QFuture<bool> future = QtConcurrent::run([task]() -> bool {
            return task();
        });
        watcher->setFuture(future);
    }
};

} // namespace Core::Storage

#endif // DOPAMINE_CORE_STORAGE_WATCHDOGRUNNER_H
