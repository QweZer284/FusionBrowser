#include "tabbar.h"
#include "settings.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QMouseEvent>
#include <QPen>
#include <QFont>

// ── TabCloseButton ────────────────────────────────────────────────────────────
TabCloseButton::TabCloseButton(QTabBar* bar)
    : QAbstractButton(bar), m_bar(bar)
{
    setFixedSize(16, 16);
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::ArrowCursor);
}

int TabCloseButton::tabIndex() const {
    for (int i = 0; i < m_bar->count(); ++i) {
        if (m_bar->tabButton(i, QTabBar::RightSide) == this)
            return i;
    }
    return -1;
}

void TabCloseButton::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRect r = rect();

    if (m_hover) {
        // Soft hover circle
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(128, 128, 128, 44));
        p.drawEllipse(r.adjusted(1, 1, -1, -1));
    }

    QPen pen(QColor(120, 120, 120), 1.4);
    pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);
    const int m = 4;
    p.drawLine(r.left()+m, r.top()+m,    r.right()-m, r.bottom()-m);
    p.drawLine(r.right()-m, r.top()+m,   r.left()+m,  r.bottom()-m);
}

void TabCloseButton::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        const int idx = tabIndex();
        if (idx >= 0) emit m_bar->tabCloseRequested(idx);
    }
    QAbstractButton::mouseReleaseEvent(e);
}

// ── FusionTabBar ──────────────────────────────────────────────────────────────
FusionTabBar::FusionTabBar(Settings* settings, QWidget* parent)
    : QTabBar(parent), m_settings(settings)
{
    setTabsClosable(false);   // we manage close buttons manually
    setMovable(true);
    setExpanding(false);
    setDocumentMode(false);
    setFont(QFont("Segoe UI", 11));
    setIconSize(QSize(14, 14));
}

void FusionTabBar::tabInserted(int index) {
    QTabBar::tabInserted(index);
    setTabButton(index, RightSide, new TabCloseButton(this));
    QTimer::singleShot(0, this, &FusionTabBar::tabLayoutChanged);
}

void FusionTabBar::tabRemoved(int index) {
    QTabBar::tabRemoved(index);
    QTimer::singleShot(0, this, &FusionTabBar::tabLayoutChanged);
}

// ── TabStrip ──────────────────────────────────────────────────────────────────
TabStrip::TabStrip(QWidget* mainWin, Settings* settings)
    : QWidget(mainWin), m_mainWin(mainWin)
{
    setObjectName("tabStrip");
    setFixedHeight(34);

    // Traffic lights container
    auto* tlW = new QWidget(this);
    tlW->setFixedSize(TL_W, 34);
    auto* tlLay = new QHBoxLayout(tlW);
    tlLay->setContentsMargins(8, 0, 10, 0);
    tlLay->setSpacing(8);

    auto mkTL = [&](const QString& name, const QString& tip) -> QPushButton* {
        auto* b = new QPushButton(tlW);
        b->setObjectName(name);
        b->setToolTip(tip);
        b->setFixedSize(12, 12);
        tlLay->addWidget(b);
        return b;
    };
    auto* tlClose = mkTL("tlClose", "Закрыть");
    auto* tlMin   = mkTL("tlMin",   "Свернуть");
    auto* tlMax   = mkTL("tlMax",   "Развернуть");

    connect(tlClose, &QPushButton::clicked, mainWin, &QWidget::close);
    connect(tlMin,   &QPushButton::clicked, mainWin, &QWidget::showMinimized);
    connect(tlMax,   &QPushButton::clicked, this, [mainWin]{
        if (mainWin->isMaximized()) mainWin->showNormal();
        else mainWin->showMaximized();
    });

    // Tab bar
    m_tabBar = new FusionTabBar(settings, this);
    connect(m_tabBar, &FusionTabBar::tabLayoutChanged, this, &TabStrip::repositionPlusButton);
    connect(m_tabBar, &QTabBar::currentChanged,        this, &TabStrip::repositionPlusButton);
    connect(m_tabBar, &QTabBar::tabMoved,              this, &TabStrip::repositionPlusButton);

    // + button
    m_plus = new QPushButton("+", this);
    m_plus->setObjectName("plusBtn");
    m_plus->setToolTip("Новая вкладка  Ctrl+T");
    connect(m_plus, &QPushButton::clicked, this, &TabStrip::newTabRequested);
}

void TabStrip::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    const int tbW = width() - TL_W - PLUS_W - PLUS_GAP * 2 - 6;
    m_tabBar->setGeometry(TL_W, 0, qMax(tbW, 40), height());
    repositionPlusButton();
}

void TabStrip::repositionPlusButton() {
    const int n    = m_tabBar->count();
    const int barX = m_tabBar->x();
    const int barW = m_tabBar->width();

    int x;
    if (n > 0)
        x = barX + m_tabBar->tabRect(n - 1).right() + PLUS_GAP;
    else
        x = barX + PLUS_GAP;

    x = qMin(x, barX + barW);
    x = qMax(x, TL_W + 2);

    const int y = (height() - PLUS_H) / 2;
    m_plus->setGeometry(x, y, PLUS_W, PLUS_H);
    m_plus->raise();
}

void TabStrip::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        QWidget* ch = childAt(e->pos());
        if (!ch || ch == this || ch == m_tabBar) {
            m_dragStart = e->globalPos() - m_mainWin->frameGeometry().topLeft();
            m_dragging  = true;
        }
    }
    QWidget::mousePressEvent(e);
}

void TabStrip::mouseMoveEvent(QMouseEvent* e) {
    if ((e->buttons() & Qt::LeftButton) && m_dragging && !m_mainWin->isMaximized()) {
        m_mainWin->move(e->globalPos() - m_dragStart);
    }
    QWidget::mouseMoveEvent(e);
}

void TabStrip::mouseReleaseEvent(QMouseEvent* e) {
    m_dragging = false;
    QWidget::mouseReleaseEvent(e);
}

void TabStrip::mouseDoubleClickEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        if (m_mainWin->isMaximized()) m_mainWin->showNormal();
        else m_mainWin->showMaximized();
    }
    QWidget::mouseDoubleClickEvent(e);
}
