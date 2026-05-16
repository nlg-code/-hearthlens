#include <memory>
#include <QApplication>
#include <QGuiApplication>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

#include "SessionManager.h"
#include "LogWatcher.h"
#include "PowerLogParser.h"
#include "DecksLogParser.h"
#include "CardDatabase.h"
#include "ImageCache.h"
#include "GameState.h"
#include "OverlayWindow.h"

// Ensures Hearthstone's log.config has the sections needed for Power.log and
// Decks.log to be written. Returns true if the file was created or modified
// (meaning Hearthstone must be restarted for the change to take effect).
static bool ensureLogConfig(const QString &logsDir)
{
    // log.config lives in the Hearthstone installation directory — the parent
    // of the Logs folder (or one of the known install paths as a fallback).
    QStringList candidates;
    candidates << QFileInfo(logsDir).dir().filePath("log.config");
    candidates << "C:/Program Files (x86)/Hearthstone/log.config";
    candidates << "C:/Program Files/Hearthstone/log.config";

    // Use the first candidate whose parent directory exists.
    QString configPath;
    for (const auto &c : candidates) {
        if (QFileInfo(c).dir().exists()) { configPath = c; break; }
    }
    if (configPath.isEmpty()) return false;

    QString existing;
    {
        QFile f(configPath);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text))
            existing = QString::fromUtf8(f.readAll());
    }

    QString toAppend;
    // Hearthstone maps section names directly to log file names:
    // [Power] → Power.log,  [Decks] → Decks.log.
    // Older trackers used [PowerTaskList] but that section is no longer
    // recognised by Hearthstone and produces no output.
    if (!existing.contains("[Power]"))
        toAppend += "\n[Power]\nLogLevel=1\nFilePrinting=True\n"
                    "ConsolePrinting=False\nScreenPrinting=False\nVerbose=True\n";
    if (!existing.contains("[Decks]"))
        toAppend += "\n[Decks]\nLogLevel=1\nFilePrinting=True\n"
                    "ConsolePrinting=False\nScreenPrinting=False\nVerbose=True\n";

    if (toAppend.isEmpty()) return false;

    QFile f(configPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "[LogConfig] Cannot write" << configPath
                   << "— try running as administrator.";
        QMessageBox::warning(nullptr, "HearthLens — Permission denied",
            QString("Could not write Hearthstone log.config.\n\n"
                    "Please run HearthLens as administrator once so it can\n"
                    "enable game logging, then relaunch normally.\n\n"
                    "Expected location:\n%1").arg(configPath));
        return false;
    }
    f.write(toAppend.toUtf8());
    qInfo() << "[LogConfig] Updated" << configPath;
    return true;
}

static QString resolveLogsDir()
{
    QSettings settings("HearthLens", "HearthLens");
    QString saved = settings.value("logsDir").toString();
    if (!saved.isEmpty() && QDir(saved).exists()) return saved;

    const QStringList candidates = {
        "C:/Program Files (x86)/Hearthstone/Logs",
        "C:/Program Files/Hearthstone/Logs",
        QDir::homePath() + "/AppData/Local/Blizzard/Hearthstone/Logs",
    };
    for (const auto &p : candidates) {
        if (QDir(p).exists()) {
            settings.setValue("logsDir", p);
            return p;
        }
    }

    QString picked = QFileDialog::getExistingDirectory(
        nullptr, "Select Hearthstone Logs folder", "C:/");
    if (!picked.isEmpty()) settings.setValue("logsDir", picked);
    return picked;
}

int main(int argc, char **argv)
{
    // Disable per-monitor DPI scaling so the overlay panels stay a fixed
    // pixel size when dragged across monitors with different scale factors.
    // Windows will bitmap-scale on high-DPI screens (slight blur, but stable size).
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");

    QApplication app(argc, argv);
    app.setApplicationName("HearthLens");
    app.setOrganizationName("HearthLens");

    QString logsDir = resolveLogsDir();
    if (logsDir.isEmpty()) {
        QMessageBox::critical(nullptr, "HearthLens",
            "Could not find Hearthstone Logs folder.");
        return 1;
    }
    qInfo() << "Logs dir:" << logsDir;

    if (ensureLogConfig(logsDir)) {
        QMessageBox::information(nullptr, "HearthLens — Logging enabled",
            "Hearthstone logging has been configured for the first time.\n\n"
            "Please start (or restart) Hearthstone now — the tracker will\n"
            "detect your game automatically once it launches.");
    }

    auto *cards        = new CardDatabase();
    auto *images       = new ImageCache();
    auto *state        = new GameState(cards);
    auto *parser       = new PowerLogParser();
    auto *watcher      = new LogWatcher();
    auto *decksParser  = new DecksLogParser(cards);
    auto *decksWatcher = new LogWatcher();
    auto *session      = new SessionManager(logsDir);
    auto *overlay      = new OverlayWindow(state, cards, images, &app);

    // On the very first detection the session folder already existed when the
    // tracker launched (user is in lobby from a prior play session).  Seek to
    // the end so we don't replay completed games.  For every subsequent new
    // session folder (Hearthstone restarted while tracker is running) we read
    // from the beginning to catch the game that is starting.
    auto firstPowerLog = std::make_shared<bool>(true);
    QObject::connect(session, &SessionManager::newSessionDetected,
                     watcher, [watcher, firstPowerLog](const QString &path) {
                         bool seek = *firstPowerLog;
                         *firstPowerLog = false;
                         watcher->watchFile(path, /*seekToEnd=*/seek);
                     });
    QObject::connect(watcher, &LogWatcher::lineRead,
                     parser,  &PowerLogParser::onLine);

    // Reset the decks parser and clear the loaded deck whenever a new session
    // starts so that the previous session's deck doesn't persist in the overlay.
    QObject::connect(session, &SessionManager::newSessionDetected,
                     decksParser, &DecksLogParser::reset);
    QObject::connect(session, &SessionManager::newSessionDetected,
                     state, [state](const QString &) { state->onNewSession(); });

    QObject::connect(session, &SessionManager::decksLogDetected,
                     decksWatcher, [decksWatcher](const QString &path) {
                         decksWatcher->watchFile(path, /*seekToEnd=*/false);
                     });
    QObject::connect(decksWatcher, &LogWatcher::lineRead,
                     decksParser,  &DecksLogParser::onLine);
    QObject::connect(decksParser, &DecksLogParser::deckLoaded,
                     state,        &GameState::onDeckLoaded); // (cards, format)

    QObject::connect(parser, &PowerLogParser::gameCreated,
                     state,  &GameState::onGameCreated);
    QObject::connect(parser, &PowerLogParser::gameEnded,
                     state,  &GameState::onGameEnded);
    QObject::connect(parser, &PowerLogParser::turnChanged,
                     state,  &GameState::onTurnChanged);
    QObject::connect(parser, &PowerLogParser::cardRevealed,
                     state,  &GameState::onCardRevealed);
    QObject::connect(parser, &PowerLogParser::zoneChanged,
                     state,  &GameState::onZoneChanged);

    cards->load();
    overlay->show();
    return app.exec();
}
