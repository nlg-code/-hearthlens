#include "CardDetailTooltip.h"
#include "ImageCache.h"
#include <QTimer>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPainter>
#include <QBitmap>
#include <QScreen>
#include <QApplication>
#include <QEvent>

CardDetailTooltip::CardDetailTooltip(CardDatabase *cards, ImageCache *images, QWidget *parent)
    : QWidget(nullptr, Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
    m_cards(cards), m_images(images)
{
    Q_UNUSED(parent);
    setAttribute(Qt::WA_ShowWithoutActivating);

    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(150);
    connect(m_hideTimer, &QTimer::timeout, this, &QWidget::hide);

    connect(images, &ImageCache::renderReady, this, &CardDetailTooltip::onRenderReady);
    hide();
}

void CardDetailTooltip::showForCard(const QString &cardId, QWidget *anchor, QRect itemGlobalRect)
{
    if (cardId.isEmpty() || m_excluded.contains(cardId)) { scheduleHide(); return; }
    m_currentAnchor   = anchor;
    m_currentItemRect = itemGlobalRect;
    if (cardId != m_currentCard) {
        m_currentCard = cardId;
        m_render = m_images->cardRender(cardId);
    }
    if (m_render.isNull()) {
        hide();
        return;
    }
    setFixedSize(m_render.size());
    reposition(anchor, itemGlobalRect);
    show();
    setMask(QBitmap::fromImage(m_render.toImage().createAlphaMask()));
    raise();
    repaint();
}

void CardDetailTooltip::scheduleHide() { m_hideTimer->start(); }
void CardDetailTooltip::cancelHide()   { m_hideTimer->stop(); }

void CardDetailTooltip::attachToList(QListWidget *list, QWidget *anchor)
{
    list->setMouseTracking(true);
    list->viewport()->setMouseTracking(true);
    list->viewport()->installEventFilter(this);
    connect(list, &QListWidget::itemEntered, this,
            [this, list, anchor](QListWidgetItem *item) {
                cancelHide();
                if (item) {
                    QRect r = list->visualItemRect(item);
                    QRect globalR(list->viewport()->mapToGlobal(r.topLeft()), r.size());
                    showForCard(item->data(Qt::UserRole).toString(), anchor, globalR);
                } else {
                    scheduleHide();
                }
            });
}

void CardDetailTooltip::onRenderReady(const QString &cardId)
{
    if (cardId != m_currentCard) return;
    m_render = m_images->cardRender(cardId);
    if (m_render.isNull()) return;
    setFixedSize(m_render.size());
    if (m_currentAnchor) {
        reposition(m_currentAnchor, m_currentItemRect);
        show();
        setMask(QBitmap::fromImage(m_render.toImage().createAlphaMask()));
        raise();
    }
    repaint();
}

void CardDetailTooltip::reposition(QWidget *anchor, QRect itemGlobalRect)
{
    QPoint anchorTL = anchor->mapToGlobal(QPoint(0, 0));
    int centeredY = itemGlobalRect.center().y() - height() / 2;
    QPoint pos(anchorTL.x() - width() - 4, centeredY);
    QScreen *screen = QApplication::screenAt(anchorTL);
    if (screen) {
        const QRect avail = screen->availableGeometry();
        if (pos.x() < avail.left())
            pos.setX(anchorTL.x() + anchor->width() + 4);
        pos.setX(qMax(avail.left(), qMin(avail.right() - width(), pos.x())));
        pos.setY(qMax(avail.top(),  qMin(avail.bottom() - height(), pos.y())));
    }
    move(pos);
}

bool CardDetailTooltip::eventFilter(QObject *obj, QEvent *e)
{
    Q_UNUSED(obj);
    if (e->type() == QEvent::Leave)
        scheduleHide();
    return false;
}

void CardDetailTooltip::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    if (!m_render.isNull()) {
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.drawPixmap(0, 0, m_render);
    }
}