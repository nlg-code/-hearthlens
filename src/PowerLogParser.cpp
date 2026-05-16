#include "PowerLogParser.h"
#include <QDebug>

PowerLogParser::PowerLogParser(QObject *parent) : QObject(parent)
{
    m_rePrefix              = QRegularExpression(R"(^D \d\d:\d\d:\d\d\.\d+ (?:PowerTaskList|GameState)\.DebugPrintPower\(\) - )");
    m_reCreateGame          = QRegularExpression(R"(^\s*CREATE_GAME\s*$)");
    m_reGameOver            = QRegularExpression(R"(TAG_CHANGE\s+Entity=GameEntity\s+tag=STATE\s+value=COMPLETE)");
    m_reFullEntityBracketed = QRegularExpression(R"(FULL_ENTITY\s+-\s+(?:Updating|Creating)\s+\[((?:[^\[\]]*|\[[^\[\]]*\])*)\]\s+CardID=([A-Za-z0-9_]*))");
    m_reShowEntityBracketed = QRegularExpression(R"(SHOW_ENTITY\s+-\s+Updating\s+Entity=\[((?:[^\[\]]*|\[[^\[\]]*\])*)\]\s+CardID=([A-Za-z0-9_]+))");
    m_reTagChangeBracketed  = QRegularExpression(R"(TAG_CHANGE\s+Entity=\[((?:[^\[\]]*|\[[^\[\]]*\])*)\]\s+tag=ZONE\s+value=(\w+))");
    m_reTurnChange          = QRegularExpression(R"(TAG_CHANGE\s+Entity=GameEntity\s+tag=TURN\s+value=(\d+))");
    m_reBodyTag             = QRegularExpression(R"(^\s+tag=(\w+)\s+value=(.+)$)");
}

bool PowerLogParser::isPowerTaskListLine(const QString &line) const
{
    return m_rePrefix.match(line).hasMatch();
}

QString PowerLogParser::stripPrefix(const QString &line) const
{
    auto m = m_rePrefix.match(line);
    if (!m.hasMatch()) return line;
    return line.mid(m.capturedEnd());
}

EntityInfo PowerLogParser::parseEntityBlock(const QString &block)
{
    EntityInfo e;
    QString cleaned = block;
    cleaned.remove(QRegularExpression(R"(\[[^\]]*\])"));
    QRegularExpression re(R"((\w+)=([^\s]+))");
    auto it = re.globalMatch(cleaned);
    while (it.hasNext()) {
        auto m = it.next();
        const QString k = m.captured(1);
        const QString v = m.captured(2);
        if      (k == "id")     e.id = v.toInt();
        else if (k == "player") e.playerId = v.toInt();
        else if (k == "zone")   e.zone = v;
        else if (k == "cardId") e.cardId = v;
    }
    return e;
}

void PowerLogParser::rememberEntity(const EntityInfo &e)
{
    if (e.id == -1) return;
    EntityInfo &cur = m_entities[e.id];
    cur.id = e.id;
    if (e.playerId != -1) cur.playerId = e.playerId;
    if (!e.zone.isEmpty()) cur.zone = e.zone;
    if (!e.cardId.isEmpty()) cur.cardId = e.cardId;
}

void PowerLogParser::onLine(const QString &line)
{
    if (!isPowerTaskListLine(line)) return;

    QString payload = stripPrefix(line);

    // While inside a SHOW_ENTITY / FULL_ENTITY body block, look for
    // inline tag updates.  The zone change for draws (DECK→HAND) is
    // embedded here — there is no separate TAG_CHANGE line for it.
    if (m_pendingEntityId != -1) {
        auto mBody = m_reBodyTag.match(payload);
        if (mBody.hasMatch()) {
            QString tag   = mBody.captured(1).trimmed();
            QString value = mBody.captured(2).trimmed();
            if (tag == "ZONE" && !m_pendingZone.isEmpty() && value != m_pendingZone) {
                EntityInfo &cur = m_entities[m_pendingEntityId];
                cur.zone = value;
                emit zoneChanged(m_pendingEntityId, m_pendingPlayerId,
                                 m_pendingCardId, m_pendingZone, value);
                m_pendingZone = value;
            }
            return;
        }
        m_pendingEntityId = -1;
    }

    // CREATE_GAME: new game starting.
    if (m_reCreateGame.match(payload).hasMatch()) {
        m_entities.clear();
        m_pendingEntityId = -1;
        emit gameCreated();
        return;
    }

    // TAG_CHANGE Entity=GameEntity tag=STATE value=COMPLETE: game over.
    if (m_reGameOver.match(payload).hasMatch()) {
        emit gameEnded();
        return;
    }

    // FULL_ENTITY: an entity declared with its (possibly empty) cardId.
    auto mFull = m_reFullEntityBracketed.match(payload);
    if (mFull.hasMatch()) {
        EntityInfo e = parseEntityBlock(mFull.captured(1));
        if (!mFull.captured(2).isEmpty()) e.cardId = mFull.captured(2);
        rememberEntity(e);
        m_pendingEntityId  = e.id;
        m_pendingPlayerId  = e.playerId;
        m_pendingCardId    = e.cardId;
        m_pendingZone      = e.zone;
        if (!e.cardId.isEmpty() && e.id != -1) {
            emit cardRevealed(e.id, e.playerId, e.cardId, e.zone, e.zone);
        }
        return;
    }

    auto mShow = m_reShowEntityBracketed.match(payload);
    if (mShow.hasMatch()) {
        EntityInfo e = parseEntityBlock(mShow.captured(1));
        e.cardId = mShow.captured(2);

        EntityInfo prev = entity(e.id);
        QString fromZone = prev.zone.isEmpty() ? e.zone : prev.zone;
        int resolvedPlayer = e.playerId != -1 ? e.playerId : prev.playerId;

        rememberEntity(e);

        m_pendingEntityId  = e.id;
        m_pendingPlayerId  = resolvedPlayer;
        m_pendingCardId    = e.cardId;
        m_pendingZone      = e.zone;

        emit cardRevealed(e.id, resolvedPlayer, e.cardId, fromZone, e.zone);
        return;
    }

    // TAG_CHANGE Entity=GameEntity tag=TURN value=N — turn counter incremented.
    auto mTurn = m_reTurnChange.match(payload);
    if (mTurn.hasMatch()) {
        emit turnChanged(mTurn.captured(1).toInt());
        return;
    }

    // TAG_CHANGE Entity=[..] tag=ZONE value=NEW_ZONE — a card moved zones.
    auto mTag = m_reTagChangeBracketed.match(payload);
    if (mTag.hasMatch()) {
        EntityInfo e = parseEntityBlock(mTag.captured(1));
        QString toZone = mTag.captured(2);
        if (e.id == -1) return;

        EntityInfo prev = entity(e.id);
        QString fromZone = e.zone.isEmpty() ? prev.zone : e.zone;
        int playerId    = e.playerId != -1 ? e.playerId : prev.playerId;
        QString cardId  = e.cardId.isEmpty() ? prev.cardId : e.cardId;

        EntityInfo updated = e;
        updated.zone = toZone;
        if (cardId.isEmpty() == false) updated.cardId = cardId;
        if (playerId != -1) updated.playerId = playerId;
        rememberEntity(updated);

        emit zoneChanged(e.id, playerId, cardId, fromZone, toZone);
        return;
    }
}
