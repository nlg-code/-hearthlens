#include "LogWatcher.h"
#include <QDebug>

LogWatcher::LogWatcher(QObject *parent) : QObject(parent)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &LogWatcher::poll);
    m_timer->start(100); // poll every 100ms
}

LogWatcher::~LogWatcher()
{
    closeCurrent();
}

void LogWatcher::closeCurrent()
{
    if (m_file) {
        m_file->close();
        delete m_file;
        m_file = nullptr;
    }
    m_buffer.clear();
}

void LogWatcher::watchFile(const QString &path, bool seekToEnd)
{
    closeCurrent();
    m_file = new QFile(path);
    // ReadOnly is non-exclusive on Windows — Hearthstone keeps writing fine.
    if (!m_file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[LogWatcher] Failed to open" << path;
        delete m_file;
        m_file = nullptr;
        return;
    }
    if (seekToEnd)
        m_file->seek(m_file->size());
    qDebug() << "[LogWatcher] Tailing" << path
             << (seekToEnd ? "from end" : "from start")
             << "(file size:" << m_file->size() << ")";
}

void LogWatcher::poll()
{
    if (!m_file) return;

    QByteArray chunk = m_file->readAll();
    if (!chunk.isEmpty())
        m_buffer.append(chunk);

    // Cap lines emitted per tick so the event loop stays responsive when
    // catching up on a large existing file (e.g. tracker launched mid-game).
    // Remaining lines are processed on the next 100 ms tick.
    constexpr int kMaxLinesPerTick = 300;
    int processed = 0;
    int newlineIdx;
    while (processed < kMaxLinesPerTick &&
           (newlineIdx = m_buffer.indexOf('\n')) != -1) {
        QByteArray lineBytes = m_buffer.left(newlineIdx);
        m_buffer.remove(0, newlineIdx + 1);
        if (lineBytes.endsWith('\r')) lineBytes.chop(1);
        if (!lineBytes.isEmpty()) {
            emit lineRead(QString::fromUtf8(lineBytes));
            ++processed;
        }
    }
}
