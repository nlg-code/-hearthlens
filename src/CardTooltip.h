#pragma once

#include <QWidget>
#include <QHash>
#include <QStringList>
#include "CardDatabase.h"

class ImageCache;
class GameState;
class QLabel;
class QListWidget;
class CardDetailTooltip;

class CardTooltip : public QWidget {
    Q_OBJECT
public:
    CardTooltip(CardDatabase *cards, ImageCache *images,
                GameState *state, QWidget *parent = nullptr);

    // Show the tooltip anchored to the left edge of `anchor`, vertically
    // aligned with `itemGlobalPos` (the hovered card row's global top-left).
    void showForCard(const QString &cardId, QWidget *anchor, QRect itemGlobalRect);

private:
    void populateList(QListWidget *list, const QStringList &cardIds, bool hideCount = false);

    CardDatabase *m_cards;
    ImageCache   *m_images;
    GameState    *m_state;

    QLabel      *m_title;
    QLabel      *m_normalLbl;
    QListWidget *m_normalList;
    QLabel      *m_corruptedLbl;
    QListWidget *m_corruptedList;

    CardDetailTooltip *m_cardDetail = nullptr;

    struct GeneratedCards {
        QStringList normal;
        QStringList corrupted;
        QString normalLabel    = "Normal";
        QString corruptedLabel = "Corrupted";
        // When true, the corrupted section is hidden in Standard format games.
        bool corruptedIsWildOnly = false;
    };

    static QHash<QString, GeneratedCards> buildData();

public:
    static const QHash<QString, GeneratedCards> s_data;
    // Card IDs handled dynamically (not in s_data) — excluded from CardDetailTooltip.
    static QStringList dynamicCardIds()
    {
        return { "EX1_537", "CORE_EX1_537" };
    }
};
