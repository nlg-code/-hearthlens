#pragma once

#include <QObject>
#include <QString>
#include <QFile>
#include <QTimer>

// Tails a single Power.log file. Opens it shared (read-only, no lock) so
// Hearthstone can keep writing. Seeks to end on start, polls for new bytes.
class LogWatcher : public QObject {
    Q_OBJECT
public:
    explicit LogWatcher(QObject *parent = nullptr);
    ~LogWatcher();

    // Switch to a new file (e.g. when SessionManager detects a new session).
    // Closes the old file and starts tailing the new one.
    // seekToEnd=false reads from position 0 (needed for Decks.log, where the
    // file may already contain the deckstring by the time we start watching).
    void watchFile(const QString &path, bool seekToEnd = true);

signals:
    void lineRead(const QString &line);

private slots:
    void poll();

private:
    void closeCurrent();
    QFile *m_file = nullptr;
    QTimer *m_timer;
    QByteArray m_buffer; // accumulates partial lines across reads
};
