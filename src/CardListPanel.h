#pragma once

#include <QWidget>
#include <QPoint>
#include <QHash>
#include <QString>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QTimer;
class QVBoxLayout;
class CardDatabase;
class GameState;
class ImageCache;
class CardTooltip;
class CardDetailTooltip;

class CardListPanel : public QWidget {
    Q_OBJECT
public:
    CardListPanel(const QString &title, CardDatabase *cards,
                  ImageCache *images, GameState *state,
                  QWidget *parent = nullptr);

    void setTitle(const QString &title);

    // cards     = full card map (original deck or opponent seen)
    // remaining = cards still in the deck pile (empty = all drawn, every row dims to 0)
    // hand      = cards currently in the player's hand
    // deckMode  = true → always show In Hand + In Deck sections (even if hand empty)
    // handAges  = cardId → game turn when drawn (shown as "T3" badge; opponent only)
    void populate(const QHash<QString, int> &cards,
                  const QHash<QString, int> &remaining = {},
                  const QHash<QString, int> &hand = {},
                  bool deckMode = false,
                  const QHash<QString, int> &handAges = {},
                  const QHash<QString, int> &deadMinions = {});

    // Show / refresh the possible-secrets section.
    // possibleCardIds = secrets the opponent could still have active.
    // secretsInPlay   = actual count of face-down secrets currently on board.
    void populateSecrets(const QStringList &possibleCardIds, int secretsInPlay);


protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
    bool eventFilter(QObject *obj, QEvent *e) override;

private:
    QListWidget *makeList(ImageCache *images, CardDatabase *cards);
    void wireList(QListWidget *list);
    void fillList(QListWidget *list,
                  const QVector<QPair<int,QString>> &sorted,
                  const QHash<QString,int> &counts,
                  bool markExhausted,
                  const QHash<QString,int> &handAges = {},
                  const QHash<QString,int> &deadMinions = {});
    void updateToggle(QPushButton *btn, const QString &label, int count, bool collapsed);

    CardDatabase      *m_cards;
    QLabel            *m_titleLbl;
    QVBoxLayout       *m_root;

    QPushButton       *m_secretToggle    = nullptr;
    QListWidget       *m_secretList      = nullptr;
    bool               m_secretCollapsed = false;

    QPushButton       *m_handToggle    = nullptr;
    QListWidget       *m_handList      = nullptr;
    bool               m_handCollapsed = false;

    QPushButton       *m_deckToggle    = nullptr;
    QListWidget       *m_deckList      = nullptr;
    bool               m_deckCollapsed = false;

    CardTooltip       *m_tooltip       = nullptr;
    QTimer            *m_tooltipTimer  = nullptr;
    CardDetailTooltip *m_cardDetail    = nullptr;

    bool   m_dragging   = false;
    QPoint m_dragOffset;
};
