#pragma once
#include <QWidget>
#include <QPixmap>
#include <QSet>
#include <QString>

class QTimer;
class QListWidget;
class CardDatabase;
class ImageCache;

class CardDetailTooltip : public QWidget
{
    Q_OBJECT
public:
    CardDetailTooltip(CardDatabase *cards, ImageCache *images, QWidget *parent = nullptr);

    void showForCard(const QString &cardId, QWidget *anchor, QRect itemGlobalRect);
    void attachToList(QListWidget *list, QWidget *anchor);
    void scheduleHide();
    void cancelHide();
    void exclude(const QString &cardId) { m_excluded.insert(cardId); }

protected:
    bool eventFilter(QObject *obj, QEvent *e) override;
    void paintEvent(QPaintEvent *) override;

private slots:
    void onRenderReady(const QString &cardId);

private:
    void reposition(QWidget *anchor, QRect itemGlobalRect);

    CardDatabase *m_cards;
    ImageCache   *m_images;
    QTimer       *m_hideTimer = nullptr;
    QString       m_currentCard;
    QPixmap       m_render;
    QWidget      *m_currentAnchor = nullptr;
    QRect         m_currentItemRect;
    QSet<QString> m_excluded;
};