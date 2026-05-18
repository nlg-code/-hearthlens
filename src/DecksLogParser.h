#pragma once

#include <QObject>
#include <QString>
#include <QHash>

class CardDatabase;

// Parses Decks.log and emits deckLoaded whenever "Finding Game With Deck:"
// is detected. Decodes the Hearthstone deckstring (base64 varint format)
// into a cardId -> count map using the CardDatabase's DBF ID index.
class DecksLogParser : public QObject {
    Q_OBJECT
public:
    explicit DecksLogParser(CardDatabase *cards, QObject *parent = nullptr);

public slots:
    void onLine(const QString &line);
    void reset() { m_state = 0; }

signals:
    // cardId -> count, format: 1=Wild 2=Standard 3=Classic
    void deckLoaded(const QHash<QString, int> &cards, int format);

private:
    QHash<QString, int> decodeDeckString(const QString &code, int &outFormat) const;
    static int readVarint(const QByteArray &data, int &pos);

    CardDatabase *m_cards;
    int m_state = 0; // 0=idle  1=collecting metadata/deckstring after trigger
};
