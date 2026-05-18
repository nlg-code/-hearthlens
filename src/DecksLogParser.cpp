#include "DecksLogParser.h"
#include "CardDatabase.h"
#include <QByteArray>
#include <QDebug>

DecksLogParser::DecksLogParser(CardDatabase *cards, QObject *parent)
    : QObject(parent), m_cards(cards) {}

// Unsigned LEB128 varint used in Hearthstone deckstrings.
int DecksLogParser::readVarint(const QByteArray &data, int &pos)
{
    int result = 0, shift = 0;
    while (pos < data.size()) {
        uint8_t byte = static_cast<uint8_t>(data[pos++]);
        result |= (byte & 0x7F) << shift;
        if (!(byte & 0x80)) break;
        shift += 7;
    }
    return result;
}

// Hearthstone deckstring format (base64-encoded):
//   0x00  reserved
//   varint  version (1)
//   varint  format  (1=Wild 2=Standard 3=Classic)
//   varint  hero count
//   varint * hero count  hero DBF IDs
//   varint  single-copy card count
//   varint * count  DBF IDs (1 copy each)
//   varint  dual-copy card count
//   varint * count  DBF IDs (2 copies each)
//   varint  n-copy section count
//   (varint count, varint DBF ID) * n-copy count
QHash<QString, int> DecksLogParser::decodeDeckString(const QString &code, int &outFormat) const
{
    QByteArray bytes = QByteArray::fromBase64(code.trimmed().toLatin1());
    if (bytes.isEmpty()) return {};

    int pos = 0;
    if (static_cast<uint8_t>(bytes[pos]) != 0x00) return {};
    pos++;

    readVarint(bytes, pos);          // version
    outFormat = readVarint(bytes, pos); // 1=Wild 2=Standard 3=Classic

    int heroCount = readVarint(bytes, pos);
    for (int i = 0; i < heroCount; i++) readVarint(bytes, pos);

    QHash<QString, int> result;

    int singleCount = readVarint(bytes, pos);
    for (int i = 0; i < singleCount; i++) {
        QString id = m_cards->cardIdByDbfId(readVarint(bytes, pos));
        if (!id.isEmpty()) result[id] += 1;
    }

    int dualCount = readVarint(bytes, pos);
    for (int i = 0; i < dualCount; i++) {
        QString id = m_cards->cardIdByDbfId(readVarint(bytes, pos));
        if (!id.isEmpty()) result[id] += 2;
    }

    int nCount = readVarint(bytes, pos);
    for (int i = 0; i < nCount; i++) {
        int count = readVarint(bytes, pos);
        QString id = m_cards->cardIdByDbfId(readVarint(bytes, pos));
        if (!id.isEmpty()) result[id] += count;
    }

    return result;
}

void DecksLogParser::onLine(const QString &line)
{
    // Lines look like: "I HH:MM:SS.NNNNNNN <payload>"
    // Strip the "I timestamp " prefix.
    int p = line.indexOf(' ');
    if (p == -1) return;
    p = line.indexOf(' ', p + 1);
    if (p == -1) return;
    const QString payload = line.mid(p + 1);

    // Always restart the 4-line cycle when the trigger appears, regardless of
    // current state. This guards against stale state left over from a previous
    // session's Decks.log that ended mid-parse (e.g. cancelled queue).
    if (payload.startsWith("Finding Game With Deck")) {
        m_state = 1;
        return;
    }

    switch (m_state) {
    case 0:
        break;
    case 1: { // look for the deckstring; skip any metadata / name lines
        int format = 2;
        QHash<QString, int> deck = decodeDeckString(payload, format);
        if (!deck.isEmpty()) {
            qDebug() << "[DecksLogParser] Deck loaded:" << deck.size()
                     << "unique cards, format=" << format;
            emit deckLoaded(deck, format);
            m_state = 0;
        }
        // If decode failed this line isn't the deckstring — stay in state 1
        // and keep scanning. The "Finding Game With Deck" guard above resets
        // us to state 1 anyway if a new trigger arrives before we find it.
        break;
    }
    }
}
