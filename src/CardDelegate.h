#pragma once

#include <QStyledItemDelegate>

class CardDatabase;
class ImageCache;

// Paints each QListWidget row as a card tile:
//   [cost circle] [artwork tile + gradient] [card name]  [count badge]
// Items must store cardId in Qt::UserRole and count in Qt::UserRole+1.
class CardDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit CardDelegate(CardDatabase *cards, ImageCache *images,
                          QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

private:
    CardDatabase *m_cards;
    ImageCache   *m_images;
};
