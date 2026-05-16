#pragma once

#include <QObject>
#include <QString>
#include <QHash>
#include <QSet>
#include <QPixmap>
#include <QNetworkAccessManager>

// Downloads and caches card tile images from art.hearthstonejson.com.
// tile() returns the pixmap immediately if cached, triggers a background
// download otherwise and emits tileReady(cardId) when it arrives.
class ImageCache : public QObject {
    Q_OBJECT
public:
    explicit ImageCache(QObject *parent = nullptr);
    QPixmap tile(const QString &cardId);
    QPixmap cardRender(const QString &cardId);

signals:
    void tileReady(const QString &cardId);
    void renderReady(const QString &cardId);

private slots:
    void onReplyFinished();

private:
    QString cachePath(const QString &cardId) const;
    QString renderCachePath(const QString &cardId) const;

    QHash<QString, QPixmap> m_memory;
    QSet<QString>           m_pending;
    QHash<QString, QPixmap> m_renderMemory;
    QSet<QString>           m_renderPending;
    QNetworkAccessManager  *m_nam;
};
