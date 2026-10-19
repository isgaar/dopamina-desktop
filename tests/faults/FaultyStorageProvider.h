/**
 * Location: tests/faults/FaultyStorageProvider.h
 * Function: Fault-injecting storage provider decorator simulating network
 * latencies, severed connections, truncated reads, and timeout scenarios.
 * All Rights Reserved isgaar
 */
#ifndef TESTS_FAULTS_FAULTYSTORAGEPROVIDER_H
#define TESTS_FAULTS_FAULTYSTORAGEPROVIDER_H

#include "core/storage/IStorageProvider.h"
#include <QThread>

namespace Tests::Faults {

class FaultyStorageProvider : public Dopamine::Core::Storage::IStorageProvider {
public:
    explicit FaultyStorageProvider(
        Dopamine::Core::Storage::IStorageProvider* inner = nullptr
    ) : m_inner(inner) {}

    void setSimulatedLatencyMs(int ms) { m_latencyMs = ms; }
    void setSimulateTimeout(bool b) { m_timeout = b; }
    void setSimulateCutAfterBytes(int n) { m_cutBytes = n; }
    void setSimulateFailConnect(bool b) { m_failConnect = b; }

    bool connect() override {
        applyLatency();
        if (m_failConnect || m_timeout) return false;
        return m_inner ? m_inner->connect() : true;
    }

    void disconnect() override {
        if (m_inner) m_inner->disconnect();
    }

    bool isConnected() const override {
        if (m_failConnect || m_timeout) return false;
        return m_inner ? m_inner->isConnected() : true;
    }

    bool probe() override {
        applyLatency();
        if (m_failConnect || m_timeout) return false;
        return m_inner ? m_inner->probe() : true;
    }

    QString rootPath() const override {
        return m_inner ? m_inner->rootPath() : QStringLiteral("/simulated");
    }

    bool writeRootFile(const QString& name, const QByteArray& data) override {
        applyLatency();
        if (m_timeout) return false;
        return m_inner ? m_inner->writeRootFile(name, data) : true;
    }

    QByteArray readRootFile(const QString& name) const override {
        const_cast<FaultyStorageProvider*>(this)->applyLatency();
        if (m_timeout) return QByteArray();
        QByteArray data = m_inner ? m_inner->readRootFile(name)
                                  : QByteArray("simulated_content");
        if (m_cutBytes > 0 && data.size() > m_cutBytes) {
            return data.left(m_cutBytes);
        }
        return data;
    }

    bool existsRootFile(const QString& name) const override {
        const_cast<FaultyStorageProvider*>(this)->applyLatency();
        if (m_timeout) return false;
        return m_inner ? m_inner->existsRootFile(name) : true;
    }

    QString lastError() const override {
        if (m_timeout) return QStringLiteral("ERR_CONN_TIMEOUT: Storage hung");
        if (m_failConnect)
            return QStringLiteral("ERR_CONN_REFUSED: Connect failed");
        return m_inner ? m_inner->lastError() : QString();
    }


private:
    void applyLatency() {
        if (m_latencyMs > 0) {
            QThread::msleep(static_cast<unsigned long>(m_latencyMs));
        }
    }

    Dopamine::Core::Storage::IStorageProvider* m_inner{nullptr};
    int m_latencyMs{0};
    int m_cutBytes{0};
    bool m_timeout{false};
    bool m_failConnect{false};
};

} // namespace Tests::Faults

#endif // TESTS_FAULTS_FAULTYSTORAGEPROVIDER_H
