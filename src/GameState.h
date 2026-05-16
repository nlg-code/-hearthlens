#pragma once

#include <QObject>
#include <QString>
#include <QHash>
#include <QSet>
#include <QStringList>
#include "CardDatabase.h"

class GameState : public QObject {
    Q_OBJECT
public:
    explicit GameState(CardDatabase *cards, QObject *parent = nullptr);

    int  localPlayerId() const { return m_localPlayerId; }
    bool isWild() const { return m_deckFormat != 2; }

    QHash<QString, int> playerDeck() const { return m_playerDeck; }
    QHash<QString, int> playerDeckRemaining() const;
    QHash<QString, int> playerHand() const;
    QHash<QString, int> playerCardsSeen() const { return m_playerCardsSeen; }
    QHash<QString, int> opponentCardsSeen() const { return m_opponentCardsSeen; }

    int            currentTurn()           const { return m_currentTurn; }
    QString        opponentClass()         const { return m_opponentClass; }
    int            opponentSecretsInPlay() const { return (int)m_opponentSecretEntities.size(); }
    QStringList    possibleOpponentSecrets() const { return m_possibleSecrets; }
    QHash<QString, int> opponentHandAges() const;
    QStringList playerOneCostPlayed() const { return m_playerOneCostPlayed; }
    QHash<QString, int> playerDeadMinions() const { return m_playerDeadMinions; }
    int animalCompanionUpgrades() const { return m_animalCompanionUpgrades; }
    QStringList currentAnimalCompanions() const;

    void setCatchingUp(bool catching) { m_catchingUp = catching; }
    bool isCatchingUp() const { return m_catchingUp; }

public slots:
    void onNewSession();
    void onGameCreated();
    void onGameEnded();
    void onDeckLoaded(const QHash<QString, int> &deck, int format);
    void onTurnChanged(int turn);
    void onCardRevealed(int entityId, int playerId, const QString &cardId,
                        const QString &fromZone, const QString &toZone);
    void onZoneChanged(int entityId, int playerId, const QString &cardId,
                       const QString &fromZone, const QString &toZone);

signals:
    void stateChanged();
    void localPlayerIdDetected(int playerId);

private:
    void countReveal(int playerId, const QString &cardId);
    void flushPendingDraw(int entityId, const QString &cardId);
    void retroactiveDrawCount();

    CardDatabase *m_cards;
    int m_localPlayerId = -1;
    int m_deckFormat    = 2;
    bool m_catchingUp   = false;

    QHash<int, QString> m_countedEntities;
    QHash<int, QString> m_entityZone;
    QHash<int, int>     m_entityPlayer;

    QHash<QString, int> m_playerDeck;
    QHash<QString, int> m_playerCardsSeen;
    QHash<QString, int> m_opponentCardsSeen;

    int         m_currentTurn = 0;
    QString     m_opponentClass;
    QSet<int>   m_opponentSecretEntities;
    QStringList m_possibleSecrets;
    QHash<int, int> m_entityHandTurn;
    QStringList         m_playerOneCostPlayed;
    QHash<QString, int> m_playerDeadMinions;
    int             m_animalCompanionUpgrades = 0;
    QStringList     m_currentCompanions;
    bool            m_collectingCompanions = false;
    QStringList     m_pendingCompanions;
    QSet<int>       m_countedUpgradeEntities;
    QSet<int>       m_drawnFromDeckEntities;
    QSet<int>       m_pendingDrawEntities;
    QSet<int>       m_deckOriginEntities;
    QSet<int>       m_playerPlayedFromHand;
};