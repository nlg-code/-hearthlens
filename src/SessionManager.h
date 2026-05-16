#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QFileSystemWatcher>

// Watches the Hearthstone Logs/ directory and emits when a new
// session folder (Hearthstone_YYYY_MM_DD_HH_MM_SS) appears.
// Returns the path to the active Power.log to tail.
class SessionManager : public QObject {
    Q_OBJECT
public:
    explicit SessionManager(const QString &logsDir, QObject *parent = nullptr);
    QString currentPowerLogPath() const { return m_currentPowerLog; }

signals:
    void newSessionDetected(const QString &powerLogPath);
    void decksLogDetected(const QString &decksLogPath);

private slots:
    void checkForNewSession();

private:
    QString findLatestSessionFolder() const;
    QString m_logsDir;
    QString m_currentSessionFolder;
    QString m_currentPowerLog;
    bool    m_decksLogEmitted = false;
    QTimer *m_pollTimer;
};
