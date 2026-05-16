#pragma once

#include <QObject>
#include "GameState.h"
#include "CardDatabase.h"

class ImageCache;
class CardListPanel;
class AnimalCompanionWidget;

class OverlayWindow : public QObject {
    Q_OBJECT
public:
    OverlayWindow(GameState *state, CardDatabase *cards,
                  ImageCache *images, QObject *parent = nullptr);
    void show();

public slots:
    void refresh();

private:
    GameState              *m_state;
    CardDatabase           *m_cards;
    CardListPanel          *m_playerPanel;
    CardListPanel          *m_oppPanel;
    AnimalCompanionWidget  *m_acWidget;
};
