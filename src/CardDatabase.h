#pragma once

#include <QObject>
#include <QString>
#include <QHash>
#include <QNetworkAccessManager>

struct CardInfo {
    QString id;
    QString name;
    int     cost   = 0;
    int     dbfId  = 0;
    QString cardClass;
    QString rarity;
    QString type;   // MINION, SPELL, WEAPON, HERO, etc.
    QString text;   // card description (may contain HTML like <b>)
    int     attack = -1;
    int     health = -1;
};

// Fetches and caches card data from api.hearthstonejson.com.
// Loads from local cache file if present to avoid hitting the network every launch.
class CardDatabase : public QObject {
    Q_OBJECT
public:
    explicit CardDatabase(QObject *parent = nullptr);
    void load(); // load from cache, fetch if missing
    CardInfo lookup(const QString &cardId) const;
    QString cardIdByDbfId(int dbfId) const;
    bool ready() const { return m_ready; }

signals:
    void loaded();

private slots:
    void onDownloadFinished();

private:
    void loadFromBytes(const QByteArray &json);
    QString cachePath() const;
    QHash<QString, CardInfo> m_cards;
    QHash<int, QString> m_dbfToCardId;
    QNetworkAccessManager *m_nam;
    bool m_ready = false;
};
