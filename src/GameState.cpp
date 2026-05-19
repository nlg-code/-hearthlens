#include "GameState.h"
#include "SecretsDatabase.h"
#include <QDebug>

GameState::GameState(CardDatabase *cards, QObject *parent)
    : QObject(parent), m_cards(cards) {}

void GameState::onNewSession()
{
    m_playerDeck.clear();
    m_deckFormat = 2;
    qDebug() << "[GameState] New session — deck cleared.";
    emit stateChanged();
}

void GameState::onGameEnded()
{
    m_localPlayerId = -1;
    m_countedEntities.clear();
    m_entityZone.clear();
    m_entityPlayer.clear();
    m_playerCardsSeen.clear();
    m_opponentCardsSeen.clear();
    m_currentTurn = 0;
    m_opponentClass.clear();
    m_opponentSecretEntities.clear();
    m_possibleSecrets.clear();
    m_entityHandTurn.clear();
    m_playerOneCostPlayed.clear();
    m_playerDeadMinions.clear();
    m_playerPlayedFromHand.clear();
    m_deckOriginEntities.clear();
    m_pendingDrawEntities.clear();
    m_animalCompanionUpgrades = 0;
    m_currentCompanions.clear();
    m_collectingCompanions = false;
    m_pendingCompanions.clear();
    m_countedUpgradeEntities.clear();
    m_drawnFromDeckEntities.clear();

    m_catchingUp = false;
    qDebug() << "[GameState] Game ended — per-game state reset.";
    emit stateChanged();
}

void GameState::onGameCreated()
{
    m_localPlayerId = -1;
    m_countedEntities.clear();
    m_entityZone.clear();
    m_entityPlayer.clear();
    m_playerCardsSeen.clear();
    m_opponentCardsSeen.clear();
    m_currentTurn = 0;
    m_opponentClass.clear();
    m_opponentSecretEntities.clear();
    m_possibleSecrets.clear();
    m_entityHandTurn.clear();
    m_playerOneCostPlayed.clear();
    m_playerDeadMinions.clear();
    m_playerPlayedFromHand.clear();
    m_deckOriginEntities.clear();
    m_pendingDrawEntities.clear();
    m_animalCompanionUpgrades = 0;
    m_currentCompanions.clear();
    m_collectingCompanions = false;
    m_pendingCompanions.clear();
    m_countedUpgradeEntities.clear();
    m_drawnFromDeckEntities.clear();

    m_catchingUp = false;

    // NOTE: m_playerDeck and m_deckFormat are intentionally NOT cleared here.
    // The deck definition comes from Decks.log which is a separate file parsed
    // by a separate LogWatcher.  If we clear here, and Decks.log was already
    // fully parsed before Power.log reaches CREATE_GAME, the deck is lost.
    // The deck is only replaced when onDeckLoaded fires with a new deck.

    qDebug() << "[GameState] New game started, state reset."
             << "playerDeck has" << m_playerDeck.size() << "unique cards";
    emit stateChanged();
}

void GameState::onTurnChanged(int turn)
{
    m_currentTurn = turn;
}

void GameState::onDeckLoaded(const QHash<QString, int> &deck, int format)
{
    m_playerDeck = deck;
    m_deckFormat = format;
    qDebug() << "[GameState] Player deck loaded:" << deck.size()
             << "unique cards, format=" << format;

    // If a game is already in progress and the deck arrived late (e.g. Decks.log
    // was detected after Power.log had already established the local player),
    // re-run retroactive draw counting now that we know what's in the deck.
    if (m_localPlayerId != -1)
        retroactiveDrawCount();

    emit stateChanged();
}

QHash<QString, int> GameState::playerHand() const
{
    if (m_localPlayerId == -1) return {};
    QHash<QString, int> result;
    for (auto it = m_entityZone.constBegin(); it != m_entityZone.constEnd(); ++it) {
        int eid = it.key();
        if (it.value() != "HAND") continue;
        if (m_entityPlayer.value(eid, -1) != m_localPlayerId) continue;
        const QString &cid = m_countedEntities.value(eid);
        if (!cid.isEmpty())
            result[cid]++;
    }
    return result;
}

QHash<QString, int> GameState::playerDeckRemaining() const
{
    QHash<QString, int> remaining = m_playerDeck;
    for (auto it = m_playerCardsSeen.constBegin(); it != m_playerCardsSeen.constEnd(); ++it) {
        int left = remaining.value(it.key(), 0) - it.value();
        if (left > 0)
            remaining[it.key()] = left;
        else
            remaining.remove(it.key());
    }
    return remaining;
}

QHash<QString, int> GameState::opponentHandAges() const
{
    if (m_localPlayerId == -1) return {};
    QHash<QString, int> result;
    for (auto it = m_entityHandTurn.constBegin(); it != m_entityHandTurn.constEnd(); ++it) {
        int eid = it.key();
        if (m_entityPlayer.value(eid, -1) == m_localPlayerId) continue;
        const QString &cid = m_countedEntities.value(eid);
        if (cid.isEmpty()) continue;
        if (!result.contains(cid) || result[cid] > it.value())
            result[cid] = it.value();
    }
    return result;
}

void GameState::countReveal(int playerId, const QString &cardId)
{
    if (cardId.isEmpty() || playerId == -1) return;
    if (m_localPlayerId == -1) return;

    if (playerId == m_localPlayerId) {
        m_playerCardsSeen[cardId] += 1;
        qDebug() << "[GameState] countReveal LOCAL:" << cardId
                 << "seen=" << m_playerCardsSeen[cardId];
    } else {
        if (!m_catchingUp)
            m_opponentCardsSeen[cardId] += 1;
    }
}

void GameState::flushPendingDraw(int entityId, const QString &cardId)
{
    if (!m_pendingDrawEntities.remove(entityId)) return;
    if (cardId.isEmpty()) return;
    if (!m_drawnFromDeckEntities.contains(entityId)) {
        m_drawnFromDeckEntities.insert(entityId);
        qDebug() << "[GameState] flushPendingDraw: entity" << entityId << "→" << cardId;
        countReveal(m_localPlayerId, cardId);
    }
}

void GameState::retroactiveDrawCount()
{
    qDebug() << "[GameState] retroactiveDrawCount: localPlayer=" << m_localPlayerId
             << "entityZone.size=" << m_entityZone.size()
             << "playerDeck.size=" << m_playerDeck.size();

    int counted = 0, skippedAlready = 0, skippedPlayer = 0, skippedDeck = 0;
    int skippedNoCid = 0, skippedNotInDeck = 0, deferred = 0;

    for (auto it = m_entityZone.constBegin(); it != m_entityZone.constEnd(); ++it) {
        int eid = it.key();

        if (m_drawnFromDeckEntities.contains(eid)) {
            skippedAlready++;
            continue;
        }

        int ePlayer = m_entityPlayer.value(eid, -1);
        if (ePlayer != m_localPlayerId) {
            skippedPlayer++;
            continue;
        }

        if (it.value() == "DECK") {
            skippedDeck++;
            continue;
        }

        const QString &cid = m_countedEntities.value(eid);
        if (cid.isEmpty()) {
            deferred++;
            m_pendingDrawEntities.insert(eid);
            qDebug() << "  [retro] entity" << eid << "zone=" << it.value()
                     << "player=" << ePlayer << "NO CARDID → deferred";
            continue;
        }

        if (!m_playerDeck.contains(cid)) {
            skippedNotInDeck++;
            qDebug() << "  [retro] entity" << eid << "cardId=" << cid
                     << "NOT IN DECK → skip";
            continue;
        }

        m_drawnFromDeckEntities.insert(eid);
        countReveal(m_localPlayerId, cid);
        counted++;
        qDebug() << "  [retro] entity" << eid << "cardId=" << cid
                 << "zone=" << it.value() << "→ COUNTED";
    }

    qDebug() << "[GameState] retroactiveDrawCount done:"
             << "counted=" << counted
             << "skippedAlready=" << skippedAlready
             << "skippedPlayer=" << skippedPlayer
             << "skippedDeck=" << skippedDeck
             << "skippedNoCid=" << skippedNoCid
             << "skippedNotInDeck=" << skippedNotInDeck
             << "deferred=" << deferred
             << "playerCardsSeen=" << m_playerCardsSeen;
}

void GameState::onCardRevealed(int entityId, int playerId, const QString &cardId,
                               const QString &fromZone, const QString &toZone)
{
    if (cardId.isEmpty() || entityId == -1) return;

    if (playerId != -1) m_entityPlayer[entityId] = playerId;
    if (!toZone.isEmpty()) m_entityZone[entityId] = toZone;

    if (m_collectingCompanions) {
        CardInfo ci = m_cards->lookup(cardId);
        static const QSet<QString> kBaseCompanions = {"NEW1_032", "NEW1_033", "NEW1_034"};
        const bool isLocalOrGame = (playerId == -1 || playerId == m_localPlayerId);
        if (isLocalOrGame && ci.type == "MINION" && !kBaseCompanions.contains(cardId)
            && !m_pendingCompanions.contains(cardId)) {
            m_pendingCompanions.append(cardId);
            if (m_pendingCompanions.size() == 3) {
                m_currentCompanions    = m_pendingCompanions;
                m_collectingCompanions = false;
                m_pendingCompanions.clear();
                qDebug() << "[GameState] Detected companions:" << m_currentCompanions;
                emit stateChanged();
            }
        }
    }

    if (playerId == -1) { emit stateChanged(); return; }

    if (m_localPlayerId != -1 && playerId != m_localPlayerId && m_opponentClass.isEmpty()) {
        CardInfo c = m_cards->lookup(cardId);
        if (c.type == "HERO" && !c.cardClass.isEmpty())
            m_opponentClass = c.cardClass;
    }

    if (m_localPlayerId == -1 && fromZone == "DECK") {
        m_localPlayerId = playerId;
        qDebug() << "[GameState] Detected local player =" << playerId
                 << "(first DECK reveal: entity" << entityId << "→" << cardId << ")";
        emit localPlayerIdDetected(playerId);
        retroactiveDrawCount();
    }

    if (fromZone == "DECK" && toZone == "DECK")
        m_deckOriginEntities.insert(entityId);

    if (fromZone == "HAND" &&
        !m_countedUpgradeEntities.contains(entityId) &&
        m_countedEntities.contains(entityId) &&
        (cardId == "MEND_300" || cardId == "MEND_303" || cardId == "MEND_307")) {
        // fromZone == "HAND" ensures we only trigger on play, not on draw.
        // onZoneChanged sets m_countedEntities before onCardRevealed fires for
        // a deck draw, which would otherwise cause a spurious trigger on draw.
        // Seal immediately so a second reveal of the same entity cannot retrigger.
        m_countedUpgradeEntities.insert(entityId);
        m_animalCompanionUpgrades += (cardId == "MEND_307") ? 2 : 1;
        m_pendingCompanions.clear();
        m_collectingCompanions = true;
    }

    auto it = m_countedEntities.find(entityId);
    if (it != m_countedEntities.end() && it.value() == cardId) {
        if (m_localPlayerId != -1 &&
            (playerId == m_localPlayerId || m_entityPlayer.value(entityId, -1) == m_localPlayerId))
            flushPendingDraw(entityId, cardId);
        emit stateChanged();
        return;
    }
    m_countedEntities[entityId] = cardId;

    if (m_localPlayerId != -1 &&
        (playerId == m_localPlayerId || m_entityPlayer.value(entityId, -1) == m_localPlayerId))
        flushPendingDraw(entityId, cardId);

    if (m_localPlayerId != -1 && playerId != m_localPlayerId) {
        countReveal(playerId, cardId);
    } else if (m_localPlayerId != -1 && playerId == m_localPlayerId
               && fromZone == "DECK" && !toZone.isEmpty() && toZone != "DECK"
               && !m_drawnFromDeckEntities.contains(entityId)) {
        m_drawnFromDeckEntities.insert(entityId);
        countReveal(playerId, cardId);
    }
    emit stateChanged();
}

void GameState::onZoneChanged(int entityId, int playerId, const QString &cardId,
                              const QString &fromZone, const QString &toZone)
{
    if (entityId == -1) return;

    if (toZone == "DECK" && !m_entityZone.contains(entityId))
        m_deckOriginEntities.insert(entityId);

    if (!toZone.isEmpty()) m_entityZone[entityId] = toZone;
    if (playerId != -1)    m_entityPlayer[entityId] = playerId;

    int resolvedPlayer = (playerId != -1) ? playerId : m_entityPlayer.value(entityId, -1);

    // Don't remove from m_entityHandTurn on hand exit — keep it so opponentHandAges()
    // can still correlate the draw turn with the card ID after it's been revealed.

    if (m_localPlayerId != -1 && resolvedPlayer == m_localPlayerId) {
        const QString cid = !cardId.isEmpty() ? cardId : m_countedEntities.value(entityId);
        if (!cid.isEmpty()) {
            if (toZone == "PLAY" && fromZone == "HAND"
                    && !m_playerPlayedFromHand.contains(entityId)) {
                m_playerPlayedFromHand.insert(entityId);
                if (m_cards->lookup(cid).cost == 1)
                    m_playerOneCostPlayed.append(cid);
            }
            if (!m_catchingUp
                && fromZone == "PLAY" && toZone == "GRAVEYARD"
                && m_cards->lookup(cid).type == "MINION")
                m_playerDeadMinions[cid]++;
        }
    }

    if (m_localPlayerId != -1 && resolvedPlayer == m_localPlayerId
        && (toZone == "PLAY" || toZone == "GRAVEYARD")) {
        const QString cid = !cardId.isEmpty() ? cardId : m_countedEntities.value(entityId);
        if (cid == "MEND_300" || cid == "MEND_303" || cid == "MEND_307") {
            if (!m_countedUpgradeEntities.contains(entityId)) {
                m_countedUpgradeEntities.insert(entityId);
                m_animalCompanionUpgrades += (cid == "MEND_307") ? 2 : 1;
                // Trigger companion re-detection for this upgrade.
                // This path fires for both normal hand-plays (via TAG_CHANGE only)
                // and replayed copies (e.g. Confront the Tol'vir replaying Tame Pet),
                // which never come from HAND so onCardRevealed won't catch them.
                m_pendingCompanions.clear();
                m_collectingCompanions = true;
            }
        }
    }

    if (m_localPlayerId != -1 && resolvedPlayer != -1 && resolvedPlayer != m_localPlayerId) {
        if (!m_catchingUp && toZone == "HAND")
            m_entityHandTurn[entityId] = m_currentTurn;
        if (toZone == "SECRET") {
            m_opponentSecretEntities.insert(entityId);
            if (m_possibleSecrets.isEmpty() && !m_opponentClass.isEmpty()) {
                m_possibleSecrets = SecretsDatabase::cardIdsForClass(m_opponentClass);
                m_possibleSecrets.removeIf([this](const QString &id) {
                    return m_cards->lookup(id).rarity == "LEGENDARY";
                });
            }
        }
    }
    if (fromZone == "SECRET") {
        m_opponentSecretEntities.remove(entityId);
        if (!cardId.isEmpty())
            m_possibleSecrets.removeAll(cardId);
    }

    if (m_localPlayerId != -1 && resolvedPlayer == m_localPlayerId
        && fromZone == "DECK" && !toZone.isEmpty() && toZone != "DECK") {
        const QString cid = !cardId.isEmpty() ? cardId : m_countedEntities.value(entityId);
        if (!cid.isEmpty() && !m_drawnFromDeckEntities.contains(entityId)) {
            m_drawnFromDeckEntities.insert(entityId);
            countReveal(m_localPlayerId, cid);
        } else if (cid.isEmpty() && !m_drawnFromDeckEntities.contains(entityId)) {
            m_pendingDrawEntities.insert(entityId);
        }
    }

    if (cardId.isEmpty() || playerId == -1) {
        emit stateChanged();
        return;
    }

    if (m_localPlayerId == -1 && fromZone == "DECK") {
        m_localPlayerId = playerId;
        qDebug() << "[GameState] Detected local player =" << playerId
                 << "(via zoneChanged from DECK: entity" << entityId << "→" << cardId << ")";
        emit localPlayerIdDetected(playerId);
        retroactiveDrawCount();
    }

    auto it = m_countedEntities.find(entityId);
    if (it != m_countedEntities.end() && it.value() == cardId) {
        emit stateChanged();
        return;
    }
    m_countedEntities[entityId] = cardId;

    if (playerId != m_localPlayerId)
        countReveal(playerId, cardId);
    emit stateChanged();
}

QStringList GameState::currentAnimalCompanions() const
{
    if (m_animalCompanionUpgrades == 0)
        return { "NEW1_032", "NEW1_033", "NEW1_034" };
    return m_currentCompanions;
}