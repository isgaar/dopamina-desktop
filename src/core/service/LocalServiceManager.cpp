/**
 * Location: src/core/service/LocalServiceManager.cpp
 * Function: Implementation of the local service manager spawning child Python
 * uvicorn processes, supervising lifetime, and validating runtime tools.
 * All Rights Reserved isgaar
 */
#include "LocalServiceManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace Dopamine::Core::Service {

LocalServiceManager::LocalServiceManager(QObject *parent)
    : QObject(parent), m_process(new QProcess(this)) {
    resolveBinaries();
    connect(m_process, &QProcess::stateChanged,
            this, &LocalServiceManager::statusChanged);
    connect(m_process, &QProcess::errorOccurred, this, [this](auto) {
        m_lastError = QStringLiteral("process_failed");
        emit errorOccurred(m_lastError);
        emit statusChanged();
    });
}

LocalServiceManager::~LocalServiceManager() {
    stop();
}

void LocalServiceManager::resolveBinaries() {
    m_pythonPath = QStandardPaths::findExecutable(QStringLiteral("python3"));
    if (m_pythonPath.isEmpty()) {
        m_pythonPath = QStandardPaths::findExecutable(QStringLiteral("python"));
    }
    m_ffmpegPath = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
}

bool LocalServiceManager::hasPython() const {
    return !m_pythonPath.isEmpty();
}

bool LocalServiceManager::hasFfmpeg() const {
    return !m_ffmpegPath.isEmpty();
}

QString LocalServiceManager::pythonPath() const {
    return m_pythonPath;
}

QString LocalServiceManager::ffmpegPath() const {
    return m_ffmpegPath;
}

bool LocalServiceManager::isRunning() const {
    return m_process && m_process->state() != QProcess::NotRunning;
}

bool LocalServiceManager::start(const QString &musicDir, int port) {
    if (isRunning()) stop();
    resolveBinaries();
    if (!hasPython()) {
        m_lastError = QStringLiteral("no_python");
        emit errorOccurred(m_lastError);
        return false;
    }
    if (!hasFfmpeg()) {
        m_lastError = QStringLiteral("no_ffmpeg");
        emit errorOccurred(m_lastError);
        return false;
    }
    if (!QDir(musicDir).exists()) {
        m_lastError = QStringLiteral("folder_not_found");
        emit errorOccurred(m_lastError);
        return false;
    }

    auto env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("MUSIC_DIR"), musicDir);
    env.insert(QStringLiteral("PORT"), QString::number(port));
    env.insert(QStringLiteral("HOST"), QStringLiteral("127.0.0.1"));
    env.insert(QStringLiteral("PYTHONPATH"), QStringLiteral("src"));
    m_process->setProcessEnvironment(env);

    QString appDir = QCoreApplication::applicationDirPath();
    QDir d(appDir);
    if (d.exists(QStringLiteral("src/dopaminaserver"))) {
        m_process->setWorkingDirectory(appDir);
    } else if (d.cdUp() && d.exists(QStringLiteral("src/dopaminaserver"))) {
        m_process->setWorkingDirectory(d.absolutePath());
    }

    QStringList args = {QStringLiteral("-m"),
                        QStringLiteral("dopaminaserver.cli"),
                        QStringLiteral("server")};
    m_process->start(m_pythonPath, args);
    m_lastError.clear();
    return true;
}

void LocalServiceManager::stop() {
    if (!isRunning()) return;
    m_process->terminate();
    if (!m_process->waitForFinished(1000)) {
        m_process->kill();
        m_process->waitForFinished(500);
    }
}

} // namespace Dopamine::Core::Service
