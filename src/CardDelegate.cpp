#include "CardDelegate.h"
#include "CardDatabase.h"
#include "ImageCache.h"
#include <QPainter>
#include <QLinearGradient>
#include <QFont>
#include <QFontMetrics>

static constexpr int ROW_H      = 34;
static constexpr int COST_W     = 36; // width of mana cost badge on left
static constexpr int COUNT_W    = 22; // width of count badge on right
static constexpr int TILE_BLEND = 80; // pixels over which tile fades to background

CardDelegate::CardDelegate(CardDatabase *cards, ImageCache *images, QObject *parent)
    : QStyledItemDelegate(parent), m_cards(cards), m_images(images) {}

QSize CardDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
    return QSize(0, ROW_H);
}

void CardDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    const QString cardId   = index.data(Qt::UserRole).toString();
    const int count        = index.data(Qt::UserRole + 1).toInt();
    const bool exhausted   = index.data(Qt::UserRole + 2).toBool();
    const int handTurn     = index.data(Qt::UserRole + 3).toInt();
    const int deadCount    = index.data(Qt::UserRole + 4).toInt();
    const CardInfo info    = m_cards->lookup(cardId);

    if (exhausted) painter->setOpacity(0.35);

    const QRect r = QRect(option.rect.left(), option.rect.top(), option.rect.width(), ROW_H);
    painter->setClipRect(r);

    // ── Rarity colour ────────────────────────────────────────────────────────
    QColor rarityCol;
    const QString &rar = info.rarity;
    if      (rar == "LEGENDARY") rarityCol = QColor(255, 140,  10);
    else if (rar == "EPIC")      rarityCol = QColor(163,  53, 238);
    else if (rar == "RARE")      rarityCol = QColor( 40, 105, 255);
    else                         rarityCol = QColor(180, 180, 180);

    // ── Background ───────────────────────────────────────────────────────────
    painter->fillRect(option.rect, QColor(30, 30, 42));
    painter->fillRect(r, QColor(30, 30, 42));

    // ── Rarity strip  ────────────────────────────────────────────────────────
    painter->fillRect(QRect(r.left(), r.top(), 3, r.height()), rarityCol);

    // ── Tile image (right-aligned, fades left into background) ───────────────
    QPixmap tile = m_images->tile(cardId);
    if (!tile.isNull()) {
        QPixmap scaled = tile.scaledToHeight(r.height(), Qt::SmoothTransformation);
        int availW = r.width() - COST_W - COUNT_W;
        int drawW  = qMin(scaled.width(), availW);
        QRect dest(r.left() + COST_W + (availW - drawW), r.top(), drawW, r.height());
        QRect src(scaled.width() - drawW, 0, drawW, scaled.height());
        painter->drawPixmap(dest, scaled, src);
        QLinearGradient fade(dest.left(), 0, dest.left() + TILE_BLEND, 0);
        fade.setColorAt(0.0, QColor(30, 30, 42, 255));
        fade.setColorAt(1.0, QColor(30, 30, 42,   0));
        painter->fillRect(dest, fade);
    }

    // ── Mana cost ──────────────────────────────────────────────────────────────
    {
        QRect badge(r.left() + 5, r.top() + 2, COST_W - 4, r.height() - 4);
        painter->setBrush(QColor(25, 75, 165));
        painter->setPen(QPen(QColor(180, 200, 255), 1));
        painter->drawEllipse(badge);
        painter->setPen(Qt::white);
        QFont f = painter->font();
        f.setBold(true);
        f.setPixelSize(15);
        painter->setFont(f);
        painter->drawText(badge, Qt::AlignCenter, info.cost >= 0 ? QString::number(info.cost) : "?");
    }

    // ── Card name ─────────────────────────────────────────────────────────────
    {
        int rightReserved = (count >= 0) ? COUNT_W + 8 : 4;
        QRect nameRect(r.left() + COST_W + 7, r.top(), r.width() - COST_W - rightReserved, r.height());
        painter->setPen(Qt::white);
        QFont f = painter->font();
        f.setFamily("Segoe UI");
        f.setBold(true);
        f.setPixelSize(15);
        painter->setFont(f);
        QString name = info.name.isEmpty() ? cardId : info.name;
        QFontMetrics fm(painter->font());
        name = fm.elidedText(name, Qt::ElideRight, nameRect.width());
        painter->drawText(nameRect, Qt::AlignVCenter | Qt::AlignLeft, name);
    }

    // ── Count badge ─────────────────
    if (count >= 0) {
        const bool isLegendary = (rar == "LEGENDARY");
        QRect badge(r.right() - COUNT_W, r.top(), COUNT_W, r.height());
        if (isLegendary) {
            painter->setBrush(rarityCol.darker(180));
            painter->setPen(QPen(rarityCol, 1));
        } else {
            painter->setBrush(QColor(160, 120, 20));
            painter->setPen(QPen(QColor(230, 190, 80), 1));
        }
        painter->drawRect(badge);
        painter->setPen(Qt::white);
        QFont f = painter->font();
        f.setBold(true);
        f.setPixelSize(isLegendary ? 13 : 14);
        painter->setFont(f);
        painter->drawText(badge, Qt::AlignCenter, isLegendary ? QStringLiteral("★") : QString::number(count));
    }

    // ── Skull badge — shown when copies have died this game ──────────────────
    int rightOffset = COUNT_W;
    if (deadCount > 0) {
        constexpr int SKULL_W = 20;
        QRect badge(r.right() - rightOffset - SKULL_W - 2,
                    r.top() + 5, SKULL_W, r.height() - 10);
        painter->setBrush(QColor(60, 15, 15, 220));
        painter->setPen(QPen(QColor(180, 50, 50), 1));
        painter->drawRoundedRect(badge, 2, 2);
        painter->setPen(QColor(220, 80, 80));
        QFont f = painter->font();
        f.setBold(true);
        f.setPixelSize(13);
        painter->setFont(f);
        painter->drawText(badge, Qt::AlignCenter, QStringLiteral("☠"));
        rightOffset += SKULL_W + 2;
    }

    // ── Hand-age badge ("T3") — shown when handTurn > 0 ──────────────────────
    if (handTurn > 0) {
        constexpr int TURN_W = 24;
        QRect badge(r.right() - rightOffset - TURN_W - 2,
                    r.top() + 5, TURN_W, r.height() - 10);
        painter->setBrush(QColor(20, 90, 70));
        painter->setPen(QPen(QColor(60, 180, 140), 1));
        painter->drawRoundedRect(badge, 2, 2);
        painter->setPen(Qt::white);
        QFont f = painter->font();
        f.setBold(true);
        f.setPixelSize(12);
        painter->setFont(f);
        painter->drawText(badge, Qt::AlignCenter, QString("T%1").arg(handTurn));
    }

    // ── Selection highlight ────────────────────────────────────────────────────
    if (option.state & QStyle::State_Selected)
        painter->fillRect(r, QColor(255, 255, 255, 30));

    // ── Row separator ─────────────────────────────────────────────────────────
    painter->setOpacity(1.0);
    painter->setClipping(false);
    painter->setPen(QPen(QColor(10, 10, 18), 1));
    painter->drawLine(r.left(), r.bottom(), r.right(), r.bottom());

    painter->restore();
}
