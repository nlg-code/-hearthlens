#include "ImageCache.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QUrl>
#include <QDebug>

static constexpr const char *TILE_URL   = "https://art.hearthstonejson.com/v1/tiles/%1.jpg";
static constexpr const char *RENDER_URL = "https://art.hearthstonejson.com/v1/render/latest/enUS/256x/%1.png";

ImageCache::ImageCache(QObject *parent) : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
}

QString ImageCache::cachePath(const QString &cardId) const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/tiles";
    QDir().mkpath(dir);
    return dir + "/" + cardId + ".jpg";
}

QString ImageCache::renderCachePath(const QString &cardId) const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/renders";
    QDir().mkpath(dir);
    return dir + "/" + cardId + ".png";
}

QPixmap ImageCache::cardRender(const QString &cardId)
{
    if (cardId.isEmpty()) return {};

    auto it = m_renderMemory.find(cardId);
    if (it != m_renderMemory.end()) return it.value();

    QString path = renderCachePath(cardId);
    if (QFile::exists(path)) {
        QImage img(path);
        if (!img.isNull()) {
            QPixmap px = QPixmap::fromImage(img.convertToFormat(QImage::Format_ARGB32));
            m_renderMemory.insert(cardId, px);
            return px;
        }
    }

    if (!m_renderPending.contains(cardId)) {
        m_renderPending.insert(cardId);
        QNetworkRequest req{QUrl(QString(RENDER_URL).arg(cardId))};
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
        QNetworkReply *reply = m_nam->get(req);
        reply->setProperty("cardId",  cardId);
        reply->setProperty("isRender", true);
        connect(reply, &QNetworkReply::finished, this, &ImageCache::onReplyFinished);
    }
    return {};
}

QPixmap ImageCache::tile(const QString &cardId)
{
    if (cardId.isEmpty()) return {};

    // Return from memory cache.
    auto it = m_memory.find(cardId);
    if (it != m_memory.end()) return it.value();

    // Try disk cache.
    QString path = cachePath(cardId);
    if (QFile::exists(path)) {
        QPixmap px(path);
        if (!px.isNull()) {
            m_memory.insert(cardId, px);
            return px;
        }
    }

    // Kick off a download (once per cardId).
    if (!m_pending.contains(cardId)) {
        m_pending.insert(cardId);
        QNetworkRequest req{QUrl(QString(TILE_URL).arg(cardId))};
        QNetworkReply *reply = m_nam->get(req);
        reply->setProperty("cardId", cardId);
        connect(reply, &QNetworkReply::finished, this, &ImageCache::onReplyFinished);
    }
    return {};
}

void ImageCache::onReplyFinished()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;

    const QString cardId   = reply->property("cardId").toString();
    const bool    isRender = reply->property("isRender").toBool();

    if (isRender) m_renderPending.remove(cardId);
    else          m_pending.remove(cardId);

    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QPixmap px;
    if (isRender) {
        QImage img;
        img.loadFromData(data);
        if (img.isNull()) return;
        px = QPixmap::fromImage(img.convertToFormat(QImage::Format_ARGB32));
    } else {
        px.loadFromData(data);
    }
    if (px.isNull()) return;

    if (isRender) {
        QFile f(renderCachePath(cardId));
        if (f.open(QIODevice::WriteOnly)) { f.write(data); f.close(); }
        m_renderMemory.insert(cardId, px);
        emit renderReady(cardId);
    } else {
        QFile f(cachePath(cardId));
        if (f.open(QIODevice::WriteOnly)) { f.write(data); f.close(); }
        m_memory.insert(cardId, px);
        emit tileReady(cardId);
    }
}
