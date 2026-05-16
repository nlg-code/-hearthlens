#include "CardDatabase.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>

static constexpr const char *CARDS_URL =
    "https://api.hearthstonejson.com/v1/latest/enUS/cards.json";

CardDatabase::CardDatabase(QObject *parent) : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
}

QString CardDatabase::cachePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/cards.json";
}

void CardDatabase::load()
{
    QFile f(cachePath());
    if (f.exists() && f.open(QIODevice::ReadOnly)) {
        loadFromBytes(f.readAll());
        f.close();
        if (!m_cards.isEmpty()) {
            m_ready = true;
            emit loaded();
            return;
        }
    }

    qDebug() << "[CardDatabase] Cache miss, downloading...";
    QNetworkRequest req{QUrl(CARDS_URL)};
    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, &CardDatabase::onDownloadFinished);
}

void CardDatabase::onDownloadFinished()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[CardDatabase] Download failed:" << reply->errorString();
        reply->deleteLater();
        return;
    }
    QByteArray data = reply->readAll();
    reply->deleteLater();

    // Persist to cache
    QFile out(cachePath());
    if (out.open(QIODevice::WriteOnly)) { out.write(data); out.close(); }

    loadFromBytes(data);
    m_ready = true;
    emit loaded();
}

void CardDatabase::loadFromBytes(const QByteArray &json)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "[CardDatabase] JSON parse error:" << err.errorString();
        return;
    }
    if (!doc.isArray()) return;
    QJsonArray arr = doc.array();
    m_cards.clear();
    m_dbfToCardId.clear();
    m_cards.reserve(arr.size());
    for (const auto &v : std::as_const(arr)) {
        QJsonObject o = v.toObject();
        CardInfo c;
        c.id        = o.value("id").toString();
        c.name      = o.value("name").toString();
        c.cost      = o.value("cost").toInt(-1);
        c.dbfId     = o.value("dbfId").toInt();
        c.cardClass = o.value("cardClass").toString();
        c.rarity    = o.value("rarity").toString();
        c.type      = o.value("type").toString();
        c.text      = o.value("text").toString();
        c.attack    = o.value("attack").toInt(-1);
        c.health    = o.value("health").toInt(-1);
        if (!c.id.isEmpty()) {
            m_cards.insert(c.id, c);
            if (c.dbfId > 0) m_dbfToCardId.insert(c.dbfId, c.id);
        }
    }
    qDebug() << "[CardDatabase] Loaded" << m_cards.size() << "cards";
}

CardInfo CardDatabase::lookup(const QString &cardId) const
{
    return m_cards.value(cardId);
}

QString CardDatabase::cardIdByDbfId(int dbfId) const
{
    return m_dbfToCardId.value(dbfId);
}
