#pragma once

#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QStringList>

class QTimer;
class CardDatabase;
class ImageCache;
class GameState;
class CardTooltip;

// Small floating circular icon for the Animal Companion tracker.
// Shows the medallion artwork; hovering reveals the current 3 summonable companions.
// An upgrade badge (+N) appears when companions have been upgraded.
class AnimalCompanionWidget : public QWidget {
    Q_OBJECT
public:
    explicit AnimalCompanionWidget(CardDatabase *cards, ImageCache *images,
                                   GameState *state, QWidget *parent = nullptr);

    void refresh(int upgrades, const QStringList &companions, bool visible);

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    bool eventFilter(QObject *obj, QEvent *e) override;

private:
    void savePosition();
    void loadPosition();

    QPixmap      m_icon;
    int          m_upgrades   = 0;
    CardTooltip *m_tooltip;
    QTimer      *m_tooltipTimer;
    bool         m_dragging   = false;
    QPoint       m_dragOffset;
};
