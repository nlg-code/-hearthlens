#include "CardListPanel.h"
#include "CardDelegate.h"
#include "CardTooltip.h"
#include "CardDetailTooltip.h"
#include "CardDatabase.h"
#include "GameState.h"
#include "ImageCache.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTimer>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QApplication>
#include <QVector>
#include <QPair>
#include <QDebug>
#include <algorithm>

static const QString kListStyle =
    "QListWidget { background: transparent; border: none; }"
    "QListWidget::item { padding: 0px; }"
    "QScrollBar:vertical { background: rgba(255,255,255,20); width: 6px; border-radius: 3px; }"
    "QScrollBar::handle:vertical { background: rgba(255,255,255,60); border-radius: 3px; }"
    "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }";

static const QString kToggleStyle =
    "QPushButton {"
    "  background: rgba(25,25,40,210);"
    "  color: #aaddff;"
    "  border: none;"
    "  border-radius: 3px;"
    "  padding: 4px 6px;"
    "  font-weight: bold;"
    "  text-align: left;"
    "}"
    "QPushButton:hover { background: rgba(40,40,65,220); }";

// ── Helpers ───────────────────────────────────────────────────────────────────

QListWidget *CardListPanel::makeList(ImageCache *images, CardDatabase *cards)
{
    auto *list = new QListWidget(this);
    list->setStyleSheet(kListStyle);
    list->setItemDelegate(new CardDelegate(cards, images, list));
    list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(images, &ImageCache::tileReady, list, QOverload<>::of(&QWidget::update));
    return list;
}

void CardListPanel::wireList(QListWidget *list)
{
    list->setMouseTracking(true);
    list->viewport()->setMouseTracking(true);
    list->viewport()->installEventFilter(this);

    connect(list, &QListWidget::itemEntered, this,
            [this, list](QListWidgetItem *item) {
                m_tooltipTimer->stop();
                if (!item) { m_tooltipTimer->start(); return; }
                QRect r = list->visualItemRect(item);
                QRect globalR(list->viewport()->mapToGlobal(r.topLeft()), r.size());
                m_tooltip->showForCard(item->data(Qt::UserRole).toString(), this, globalR);
            });
}

void CardListPanel::fillList(QListWidget *list,
                             const QVector<QPair<int,QString>> &sorted,
                             const QHash<QString,int> &counts,
                             bool markExhausted,
                             const QHash<QString,int> &handAges,
                             const QHash<QString,int> &deadMinions)
{
    list->clear();
    for (const auto &p : sorted) {
        int count = counts.value(p.second, 0);
        bool isExhausted = markExhausted && (count == 0);

        int dead = deadMinions.value(p.second, 0);
        int displayDead = (markExhausted && isExhausted) ? dead : 0;

        auto *item = new QListWidgetItem(list);
        item->setData(Qt::UserRole,     p.second);
        item->setData(Qt::UserRole + 1, count);
        item->setData(Qt::UserRole + 2, isExhausted ? 1 : 0);
        item->setData(Qt::UserRole + 3, handAges.value(p.second, -1));
        item->setData(Qt::UserRole + 4, displayDead);
        item->setSizeHint(QSize(0, 34));
    }
    constexpr int rowH = 35, maxRows = 10;
    int h = qMin(list->count() * rowH + 2, maxRows * rowH);
    list->setFixedHeight(h);
    list->setVerticalScrollBarPolicy(
        list->count() > maxRows ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
}

void CardListPanel::updateToggle(QPushButton *btn, const QString &label,
                                 int count, bool collapsed)
{
    btn->setText(QString("%1  %2  (%3)")
                     .arg(collapsed ? QStringLiteral("▶") : QStringLiteral("▼"))
                     .arg(label)
                     .arg(count));
}

// ── Constructor ───────────────────────────────────────────────────────────────

CardListPanel::CardListPanel(const QString &title, CardDatabase *cards,
                             ImageCache *images, GameState *state,
                             QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
    m_cards(cards)
{
    setAttribute(Qt::WA_TranslucentBackground);
    resize(280, 770);

    m_root = new QVBoxLayout(this);
    m_root->setContentsMargins(8, 8, 8, 8);
    m_root->setSpacing(4);

    m_titleLbl = new QLabel(title, this);
    m_titleLbl->setStyleSheet(
        "background: rgba(20,20,30,220);"
        "color: #f0d080;"
        "padding: 6px;"
        "border-radius: 4px;"
        "font-weight: bold;"
        );
    m_root->addWidget(m_titleLbl);

    m_secretToggle = new QPushButton(this);
    m_secretToggle->setStyleSheet(
        "QPushButton {"
        "  background: rgba(60,25,80,210);"
        "  color: #ddaaff;"
        "  border: none;"
        "  border-radius: 3px;"
        "  padding: 4px 6px;"
        "  font-weight: bold;"
        "  text-align: left;"
        "}"
        "QPushButton:hover { background: rgba(80,35,110,220); }");
    m_secretToggle->setFlat(true);
    m_secretToggle->setCursor(Qt::PointingHandCursor);
    m_secretToggle->setVisible(false);
    m_root->addWidget(m_secretToggle);

    m_secretList = makeList(images, cards);
    m_secretList->setFixedHeight(2);
    m_secretList->setVisible(false);
    m_root->addWidget(m_secretList);

    connect(m_secretToggle, &QPushButton::clicked, this, [this] {
        m_secretCollapsed = !m_secretCollapsed;
        m_secretList->setVisible(!m_secretCollapsed);
        QString arrow = m_secretCollapsed ? QStringLiteral("▶") : QStringLiteral("▼");
        m_secretToggle->setText(QString("%1  Possible Secrets").arg(arrow));
    });

    m_handToggle = new QPushButton(this);
    m_handToggle->setStyleSheet(kToggleStyle);
    m_handToggle->setFlat(true);
    m_handToggle->setCursor(Qt::PointingHandCursor);
    updateToggle(m_handToggle, "In Hand", 0, m_handCollapsed);
    m_root->addWidget(m_handToggle);

    m_handList = makeList(images, cards);
    m_handList->setFixedHeight(2);
    m_root->addWidget(m_handList);

    connect(m_handToggle, &QPushButton::clicked, this, [this] {
        m_handCollapsed = !m_handCollapsed;
        m_handList->setVisible(!m_handCollapsed);
        updateToggle(m_handToggle, "In Hand", m_handList->count(), m_handCollapsed);
    });

    m_deckToggle = new QPushButton(this);
    m_deckToggle->setStyleSheet(kToggleStyle);
    m_deckToggle->setFlat(true);
    m_deckToggle->setCursor(Qt::PointingHandCursor);
    updateToggle(m_deckToggle, "In Deck", 0, m_deckCollapsed);
    m_root->addWidget(m_deckToggle);

    m_deckList = makeList(images, cards);
    m_root->addWidget(m_deckList, 1);

    connect(m_deckToggle, &QPushButton::clicked, this, [this] {
        m_deckCollapsed = !m_deckCollapsed;
        m_deckList->setVisible(!m_deckCollapsed);
        updateToggle(m_deckToggle, "In Deck", m_deckList->count(), m_deckCollapsed);
    });

    m_tooltip = new CardTooltip(cards, images, state);

    m_tooltipTimer = new QTimer(this);
    m_tooltipTimer->setSingleShot(true);
    m_tooltipTimer->setInterval(120);
    connect(m_tooltipTimer, &QTimer::timeout, m_tooltip, &QWidget::hide);

    m_tooltip->installEventFilter(this);

    m_cardDetail = new CardDetailTooltip(cards, images);
    for (const QString &id : CardTooltip::s_data.keys())
        m_cardDetail->exclude(id);
    for (const QString &id : CardTooltip::dynamicCardIds())
        m_cardDetail->exclude(id);
    m_cardDetail->attachToList(m_handList, this);
    m_cardDetail->attachToList(m_deckList, this);
    m_cardDetail->attachToList(m_secretList, this);

    wireList(m_handList);
    wireList(m_deckList);
    wireList(m_secretList);
}

// ── Public API ────────────────────────────────────────────────────────────────

void CardListPanel::setTitle(const QString &title)
{
    m_titleLbl->setText(title);
}

void CardListPanel::populate(const QHash<QString, int> &cards,
                             const QHash<QString, int> &remaining,
                             const QHash<QString, int> &hand,
                             bool deckMode,
                             const QHash<QString, int> &handAges,
                             const QHash<QString, int> &deadMinions)
{
    QVector<QPair<int, QString>> sortable;
    for (auto it = cards.constBegin(); it != cards.constEnd(); ++it) {
        CardInfo c = m_cards->lookup(it.key());
        if (c.name.isEmpty() || c.cost < 0) continue;
        sortable.append({c.cost, it.key()});
    }
    std::sort(sortable.begin(), sortable.end(),
              [&](const auto &a, const auto &b) {
                  if (a.first != b.first) return a.first < b.first;
                  return m_cards->lookup(a.second).name < m_cards->lookup(b.second).name;
              });

    if (deckMode) {
        QVector<QPair<int,QString>> handCards, deckCards;
        QSet<QString> deckCardIds;
        for (const auto &p : sortable) {
            deckCardIds.insert(p.second);
            if (hand.value(p.second, 0) > 0)
                handCards.append(p);
            deckCards.append(p);
        }
        // Generated cards (in hand but not in the original deck)
        for (auto it = hand.constBegin(); it != hand.constEnd(); ++it) {
            if (it.value() > 0 && !deckCardIds.contains(it.key())) {
                CardInfo c = m_cards->lookup(it.key());
                if (!c.name.isEmpty() && c.cost >= 0)
                    handCards.append({c.cost, it.key()});
            }
        }
        std::sort(handCards.begin(), handCards.end(), [&](const auto &a, const auto &b) {
            if (a.first != b.first) return a.first < b.first;
            return m_cards->lookup(a.second).name < m_cards->lookup(b.second).name;
        });

        // ── DEBUG: log remaining / hand / total for each card ────────────
        for (const auto &p : deckCards) {
            int inDeck = remaining.value(p.second, 0);
            int inHand = hand.value(p.second, 0);
            if (inDeck > 0 || inHand > 0) {
                qDebug() << "[CardListPanel]" << p.second
                         << "deck=" << inDeck
                         << "hand=" << inHand
                         << "total=" << (inDeck + inHand);
            }
        }

        // Hand section
        if (!handCards.isEmpty()) {
            fillList(m_handList, handCards, hand, false);
            int handTotal = 0;
            for (const auto &p : handCards) handTotal += hand.value(p.second, 0);
            updateToggle(m_handToggle, "In Hand", handTotal, m_handCollapsed);
            m_handToggle->setVisible(true);
            m_handList->setVisible(!m_handCollapsed);
        } else {
            m_handToggle->setVisible(false);
            m_handList->setVisible(false);
        }

        // Deck section: badge = deck + hand (total remaining copies).
        // Grey when total == 0 (all copies drawn AND played/discarded).
        QHash<QString,int> totalAvailable;
        for (const auto &p : deckCards) {
            int inDeck = remaining.value(p.second, 0);
            int inHand = hand.value(p.second, 0);
            totalAvailable[p.second] = inDeck + inHand;
        }

        fillList(m_deckList, deckCards, totalAvailable, true, {}, deadMinions);

        int deckTotal = 0;
        for (const auto &p : deckCards) deckTotal += remaining.value(p.second, 0);
        updateToggle(m_deckToggle, "In Deck", deckTotal, m_deckCollapsed);
        m_deckList->setMaximumHeight(QWIDGETSIZE_MAX);
        m_deckList->setMinimumHeight(0);

    } else {
        m_handToggle->setVisible(false);
        m_handList->setVisible(false);

        fillList(m_deckList, sortable, cards, false, handAges, deadMinions);
        updateToggle(m_deckToggle, "Cards Seen", m_deckList->count(), m_deckCollapsed);
        m_deckList->setMaximumHeight(QWIDGETSIZE_MAX);
        m_deckList->setMinimumHeight(0);
    }

    m_deckList->setVisible(!m_deckCollapsed);
}

void CardListPanel::populateSecrets(const QStringList &possibleCardIds, int secretsInPlay)
{
    if (secretsInPlay == 0) {
        m_secretToggle->setVisible(false);
        m_secretList->setVisible(false);
        m_secretList->clear();
        return;
    }

    m_secretList->clear();
    for (const QString &cardId : possibleCardIds) {
        auto *item = new QListWidgetItem(m_secretList);
        item->setData(Qt::UserRole,     cardId);
        item->setData(Qt::UserRole + 1, -1);
        item->setData(Qt::UserRole + 2, 0);
        item->setData(Qt::UserRole + 3, -1);
        item->setSizeHint(QSize(0, 34));
    }

    constexpr int rowH = 35, maxRows = 8;
    int h = qMin(m_secretList->count() * rowH + 2, maxRows * rowH);
    m_secretList->setFixedHeight(h);
    m_secretList->setVerticalScrollBarPolicy(
        m_secretList->count() > maxRows ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);

    QString arrow = m_secretCollapsed ? QStringLiteral("▶") : QStringLiteral("▼");
    m_secretToggle->setText(
        QString("%1  Possible Secrets  (%2 active)").arg(arrow).arg(secretsInPlay));
    m_secretToggle->setVisible(true);
    m_secretList->setVisible(!m_secretCollapsed);
}

// ── Event handling ────────────────────────────────────────────────────────────

bool CardListPanel::eventFilter(QObject *obj, QEvent *e)
{
    const bool isListVp = (obj == m_handList->viewport() ||
                           obj == m_deckList->viewport() ||
                           obj == m_secretList->viewport());
    if (e->type() == QEvent::Leave) {
        if (isListVp || obj == m_tooltip)
            m_tooltipTimer->start();
    }
    if (e->type() == QEvent::Enter && obj == m_tooltip)
        m_tooltipTimer->stop();
    return QWidget::eventFilter(obj, e);
}

void CardListPanel::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton &&
        m_titleLbl->geometry().contains(e->pos())) {
        m_dragging   = true;
        m_dragOffset = e->globalPosition().toPoint() - frameGeometry().topLeft();
        e->accept();
    }
}

void CardListPanel::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging && (e->buttons() & Qt::LeftButton)) {
        move(e->globalPosition().toPoint() - m_dragOffset);
        e->accept();
    } else {
        m_dragging = false;
    }
}

void CardListPanel::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu menu(this);
    menu.addAction("Exit", qApp, &QApplication::quit);
    menu.exec(e->globalPos());
}