#include "SessionManager.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>

SessionManager::SessionManager(const QString &logsDir, QObject *parent)
    : QObject(parent), m_logsDir(logsDir)
{
    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, &SessionManager::checkForNewSession);
    m_pollTimer->start(1000);

    // Defer the initial check so that signal connections in main() are established
    // before we emit. A singleShot(0) fires on the first event-loop iteration.
    QTimer::singleShot(0, this, &SessionManager::checkForNewSession);
}

QString SessionManager::findLatestSessionFolder() const
{
    QDir dir(m_logsDir);
    if (!dir.exists()) return {};

    // Folders are named Hearthstone_YYYY_MM_DD_HH_MM_SS — sorts correctly as strings
    QStringList folders = dir.entryList(
        QStringList() << "Hearthstone_*",
        QDir::Dirs | QDir::NoDotAndDotDot,
        QDir::Name
    );
    if (folders.isEmpty()) return {};
    return dir.filePath(folders.last());
}

void SessionManager::checkForNewSession()
{
    QString latest = findLatestSessionFolder();

    if (!latest.isEmpty() && latest != m_currentSessionFolder) {
        QString powerLog = QDir(latest).filePath("Power.log");
        if (QFileInfo::exists(powerLog)) {
            m_currentSessionFolder = latest;
            m_currentPowerLog = powerLog;
            m_decksLogEmitted = false;
            qDebug() << "[SessionManager] New session:" << powerLog;
            emit newSessionDetected(powerLog);
        }
    }

    // Decks.log may appear later (after the lobby screen loads), so keep
    // checking on every tick until we have emitted it for this session.
    if (!m_currentSessionFolder.isEmpty() && !m_decksLogEmitted) {
        QString decksLog = QDir(m_currentSessionFolder).filePath("Decks.log");
        if (QFileInfo::exists(decksLog)) {
            m_decksLogEmitted = true;
            emit decksLogDetected(decksLog);
        }
    }
}
