#pragma once

#include <QObject>
#include <QString>
#include <QRegularExpression>
#include <QHash>

// A snapshot of an entity's known facts at a moment in time.
struct EntityInfo {
    int id = -1;
    int playerId = -1;       // owner (1 or 2)
    QString zone;            // DECK / HAND / PLAY / GRAVEYARD / SECRET / SETASIDE
    QString cardId;          // empty until revealed
};

class PowerLogParser : public QObject {
    Q_OBJECT
public:
    explicit PowerLogParser(QObject *parent = nullptr);

public slots:
    void onLine(const QString &line);

signals:
    void gameCreated();
    void gameEnded();
    void turnChanged(int turn); // raw TAG_CHANGE Entity=GameEntity tag=TURN value=N

    // Fired whenever we learn a card's identity (SHOW_ENTITY or revealed FULL_ENTITY).
    void cardRevealed(int entityId, int playerId, const QString &cardId,
                      const QString &fromZone, const QString &toZone);

    // Fired on any zone change. cardId may be empty if not yet revealed.
    void zoneChanged(int entityId, int playerId, const QString &cardId,
                     const QString &fromZone, const QString &toZone);

private:
    // Parse the inside of a bracketed entity description:
    //   entityName=Foo id=N zone=Z zonePos=P cardId=X player=N
    static EntityInfo parseEntityBlock(const QString &block);

    void rememberEntity(const EntityInfo &e);
    EntityInfo entity(int id) const { return m_entities.value(id); }

    // We only process lines from PowerTaskList stream (has bracketed entities).
    // The GameState stream is a near-duplicate with bare entity IDs.
    bool isPowerTaskListLine(const QString &line) const;

    // Strip "D HH:MM:SS.fffffff PowerTaskList.DebugPrintPower() - " prefix.
    QString stripPrefix(const QString &line) const;

    QRegularExpression m_reCreateGame;
    QRegularExpression m_reGameOver;              // "TAG_CHANGE Entity=GameEntity tag=STATE value=COMPLETE"
    QRegularExpression m_reFullEntityBracketed;   // "FULL_ENTITY - Updating [..] CardID=X"
    QRegularExpression m_reShowEntityBracketed;   // "SHOW_ENTITY - Updating Entity=[..] CardID=X"
    QRegularExpression m_reTagChangeBracketed;    // "TAG_CHANGE Entity=[..] tag=ZONE value=X"
    QRegularExpression m_reTurnChange;            // "TAG_CHANGE Entity=GameEntity tag=TURN value=N"
    QRegularExpression m_rePrefix;
    QRegularExpression m_reBodyTag;

    // Persistent map of entity_id -> latest known info.
    QHash<int, EntityInfo> m_entities;

    // Track SHOW_ENTITY / FULL_ENTITY body blocks so we can detect
    // inline zone changes (e.g. tag=ZONE value=HAND after a draw).
    int     m_pendingEntityId  = -1;
    int     m_pendingPlayerId  = -1;
    QString m_pendingCardId;
    QString m_pendingZone;
};
