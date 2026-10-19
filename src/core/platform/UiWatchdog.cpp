/**
 * Location: src/core/platform/UiWatchdog.cpp
 * Function: Implementation of the UI loop watchdog logging ERR_UI_STALL
 * when main thread blocking delays exceed the responsiveness threshold.
 * All Rights Reserved isgaar
 */
#include "UiWatchdog.h"
#include "core/logging/JsonLogger.h"

namespace Core::Platform {

UiWatchdog::UiWatchdog(QObject* parent) : QObject(parent) {
    connect(&m_timer, &QTimer::timeout, this, &UiWatchdog::onTick);
}

void UiWatchdog::start(int tickIntervalMs, int thresholdMs) {
    m_thresholdMs = thresholdMs;
    m_elapsed.start();
    m_timer.start(tickIntervalMs);
}

void UiWatchdog::stop() {
    m_timer.stop();
}

void UiWatchdog::setLastOperation(const QString& op) {
    m_lastOp = op;
}

void UiWatchdog::onTick() {
    qint64 actual = m_elapsed.restart();
    if (actual > m_thresholdMs) {
        emit stallDetected(actual, m_lastOp);
        Logging::JsonLogger::instance().log(
            Logging::LogLevel::Warn,
            Errors::ErrorCode::UiStall,
            QStringLiteral("UiWatchdog"),
            QStringLiteral("ui_stall"),
            actual,
            QString(),
            QStringLiteral("UI blocked for %1 ms during: %2")
                .arg(actual).arg(m_lastOp)
        );
    }
}

} // namespace Core::Platform
