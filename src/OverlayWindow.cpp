#include "OverlayWindow.h"
#include "CardListPanel.h"
#include "AnimalCompanionWidget.h"
#include <QScreen>
#include <QApplication>

OverlayWindow::OverlayWindow(GameState *state, CardDatabase *cards,
                             ImageCache *images, QObject *parent)
    : QObject(parent), m_state(state), m_cards(cards)
{
    m_playerPanel = new CardListPanel("Your Deck", cards, images, state);
    m_oppPanel    = new CardListPanel("Opponent Cards Seen", cards, images, state);
    m_acWidget    = new AnimalCompanionWidget(cards, images, state);

    QScreen *screen = QApplication::primaryScreen();
    QRect avail = screen ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);
    int rightX = avail.right() - 280 - 16;
    int leftX  = avail.left() + 16;
    m_playerPanel->move(rightX, avail.top() + 60);
    m_oppPanel->move(leftX,    avail.top() + 60);

    connect(state, &GameState::stateChanged, this, &OverlayWindow::refresh);
    refresh();
}

void OverlayWindow::show()
{
    m_playerPanel->show();
    m_oppPanel->show();
    // m_acWidget shows itself only when relevant (driven by refresh).
}

void OverlayWindow::refresh()
{
    // ── Opponent ──────────────────────────────────────────────────────────────
    int oTotal = 0;
    for (int v : m_state->opponentCardsSeen()) oTotal += v;
    m_oppPanel->setTitle(QString("Opponent Cards Seen — %1").arg(oTotal));
    m_oppPanel->populate(m_state->opponentCardsSeen(), {}, {}, false,
                         m_state->opponentHandAges());
    m_oppPanel->populateSecrets(m_state->possibleOpponentSecrets(),
                                m_state->opponentSecretsInPlay());

    // ── Animal Companion widget ───────────────────────────────────────────────
    {
        const QHash<QString, int> &deck = m_state->playerDeck();
        bool deckHasAC  = deck.contains("EX1_537") || deck.contains("CORE_EX1_537");
        int  acUpgrades = m_state->animalCompanionUpgrades();
        m_acWidget->refresh(acUpgrades, m_state->currentAnimalCompanions(),
                            deckHasAC || acUpgrades > 0);
    }

    // ── Player ────────────────────────────────────────────────────────────────
    bool hasDeck = !m_state->playerDeck().isEmpty();
    if (hasDeck) {
        QHash<QString, int> deck      = m_state->playerDeck();
        QHash<QString, int> remaining = m_state->playerDeckRemaining();
        QHash<QString, int> hand      = m_state->playerHand();
        m_playerPanel->setTitle("Your Deck");
        m_playerPanel->populate(deck, remaining, hand, /*deckMode=*/true, {}, m_state->playerDeadMinions());
    } else {
        QString tag = (m_state->localPlayerId() == -1) ? " (waiting for game)" : " (loading deck...)";
        m_playerPanel->setTitle("Your Deck" + tag);
        m_playerPanel->populate({});
    }
}
