#include "AnimalCompanionWidget.h"
#include "CardTooltip.h"
#include <QTimer>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QApplication>
#include <QSettings>

static constexpr int SZ = 50;

AnimalCompanionWidget::AnimalCompanionWidget(CardDatabase *cards, ImageCache *images,
                                             GameState *state, QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setFixedSize(SZ, SZ);

    m_icon.load(":/ac_icon.png");

    m_tooltip = new CardTooltip(cards, images, state);

    m_tooltipTimer = new QTimer(this);
    m_tooltipTimer->setSingleShot(true);
    m_tooltipTimer->setInterval(150);
    connect(m_tooltipTimer, &QTimer::timeout, m_tooltip, &QWidget::hide);

    m_tooltip->installEventFilter(this);
    installEventFilter(this);

    loadPosition();
    hide();
}

// ── Public API ────────────────────────────────────────────────────────────────

void AnimalCompanionWidget::refresh(int upgrades, const QStringList & /*companions*/, bool visible)
{
    if (!visible) {
        m_tooltip->hide();
        hide();
        return;
    }
    if (m_upgrades != upgrades) {
        m_upgrades = upgrades;
        update();
    }
    show();
}

// ── Painting ──────────────────────────────────────────────────────────────────

void AnimalCompanionWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    const QRectF r = rect();
    const qreal  radius = r.width() / 2.0;
    const QPointF center = r.center();

    // Clip everything to a circle — the black corners of the PNG are outside this
    // boundary and never drawn (widget background is transparent).
    QPainterPath circle;
    circle.addEllipse(center, radius, radius);
    p.setClipPath(circle);

    if (!m_icon.isNull()) {
        QPixmap scaled = m_icon.scaled(SZ, SZ, Qt::KeepAspectRatioByExpanding,
                                       Qt::SmoothTransformation);
        int ox = (scaled.width()  - SZ) / 2;
        int oy = (scaled.height() - SZ) / 2;
        p.drawPixmap(0, 0, scaled, ox, oy, SZ, SZ);
    } else {
        // Fallback dark circle if image didn't load.
        p.fillPath(circle, QColor(15, 35, 15, 210));
    }

    // Upgrade badge — top-right quadrant.
    if (m_upgrades > 0) {
        p.setClipping(false);
        const QString label = QString("+%1").arg(m_upgrades);
        QFont f;
        f.setBold(true);
        f.setPixelSize(12);
        p.setFont(f);
        QFontMetrics fm(f);
        int tw = fm.horizontalAdvance(label) + 8;
        QRect badge(r.right() - tw - 2, r.top() + 2, tw, 18);
        p.setBrush(QColor(10, 60, 10, 230));
        p.setPen(QPen(QColor(80, 200, 80), 1));
        p.drawRoundedRect(badge, 3, 3);
        p.setPen(Qt::white);
        p.drawText(badge, Qt::AlignCenter, label);
    }
}

// ── Event filter (hover → tooltip) ───────────────────────────────────────────

bool AnimalCompanionWidget::eventFilter(QObject *obj, QEvent *e)
{
    if (e->type() == QEvent::Leave) {
        if (obj == this || obj == m_tooltip)
            m_tooltipTimer->start();
    }
    if (e->type() == QEvent::Enter) {
        if (obj == m_tooltip) {
            m_tooltipTimer->stop();
        } else if (obj == this) {
            m_tooltipTimer->stop();
            QRect globalRect(mapToGlobal(QPoint(0, 0)), size());
            m_tooltip->showForCard("EX1_537", this, globalRect);
        }
    }
    return QWidget::eventFilter(obj, e);
}

// ── Drag to move ──────────────────────────────────────────────────────────────

void AnimalCompanionWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_dragging   = true;
        m_dragOffset = e->globalPosition().toPoint() - frameGeometry().topLeft();
        e->accept();
    }
}

void AnimalCompanionWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging && (e->buttons() & Qt::LeftButton)) {
        move(e->globalPosition().toPoint() - m_dragOffset);
        e->accept();
    } else {
        m_dragging = false;
    }
}

void AnimalCompanionWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        savePosition();
        e->accept();
    }
}

// ── Position persistence ──────────────────────────────────────────────────────

void AnimalCompanionWidget::savePosition()
{
    QSettings s("HearthLens", "HearthLens");
    s.setValue("acWidgetPos", pos());
}

void AnimalCompanionWidget::loadPosition()
{
    QScreen *scr = QApplication::primaryScreen();
    QRect avail  = scr ? scr->availableGeometry() : QRect(0, 0, 1920, 1080);

    QSettings s("HearthLens", "HearthLens");
    if (s.contains("acWidgetPos")) {
        QPoint pos = s.value("acWidgetPos").toPoint();
        pos.setX(qBound(avail.left(), pos.x(), avail.right()  - SZ));
        pos.setY(qBound(avail.top(),  pos.y(), avail.bottom() - SZ));
        move(pos);
    } else {
        move(avail.center().x() - SZ / 2, avail.bottom() - 220);
    }
}
