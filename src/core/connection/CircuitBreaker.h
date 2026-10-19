/**
 * Location: src/core/connection/CircuitBreaker.h
 * Function: Resilient exponential backoff with jitter and circuit breaker state
 * machine guarding against network storms and credential account lockouts.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_CONNECTION_CIRCUITBREAKER_H
#define DOPAMINE_CORE_CONNECTION_CIRCUITBREAKER_H

#include <QDateTime>
#include <QRandomGenerator>

namespace Core::Connection {

class CircuitBreaker {
public:
    enum class State {
        Closed,   // Normal
        Open,     // Tripped, blocking calls
        HalfOpen  // Testing recovery
    };

    CircuitBreaker() = default;

    bool allowRequest() {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (m_state == State::Open) {
            if (now >= m_resetTimeoutMs) {
                m_state = State::HalfOpen;
                return true;
            }
            return false;
        }
        return true;
    }

    void recordSuccess() {
        m_consecutiveFailures = 0;
        m_backoffIndex = 0;
        m_state = State::Closed;
    }

    void recordFailure(bool isAuthFailure = false) {
        if (isAuthFailure) {
            // Lock out automatic retries immediately
            m_state = State::Open;
            m_resetTimeoutMs = std::numeric_limits<qint64>::max();
            return;
        }

        m_consecutiveFailures++;
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (m_consecutiveFailures >= 5) {
            m_state = State::Open;
            m_resetTimeoutMs = now + 15000; // 15 s half-open window
        }
    }

    void setRetryAfterSeconds(double seconds) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        m_state = State::Open;
        m_resetTimeoutMs = now + static_cast<qint64>(seconds * 1000.0);
    }

    int nextBackoffMs() {
        static const int delaysSec[] = {1, 2, 4, 8, 16, 30};
        int baseSec = delaysSec[m_backoffIndex];
        if (m_backoffIndex < 5) m_backoffIndex++;

        // Add ±20% jitter
        double rnd = QRandomGenerator::global()->generateDouble();
        double jitter = 0.8 + (rnd * 0.4);
        return static_cast<int>(baseSec * 1000 * jitter);

    }

    void reset() {
        m_consecutiveFailures = 0;
        m_backoffIndex = 0;
        m_state = State::Closed;
        m_resetTimeoutMs = 0;
    }

    State state() const { return m_state; }

private:
    State m_state{State::Closed};
    int m_consecutiveFailures{0};
    int m_backoffIndex{0};
    qint64 m_resetTimeoutMs{0};
};

} // namespace Core::Connection

#endif // DOPAMINE_CORE_CONNECTION_CIRCUITBREAKER_H
