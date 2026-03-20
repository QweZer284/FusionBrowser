#include "loadbar.h"
#include "settings.h"
#include <QPainter>
#include <QLinearGradient>
#include <QColor>

LoadBar::LoadBar(QWidget* parent, Settings* settings)
    : QWidget(parent), m_settings(settings)
{
    setFixedHeight(2);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    hide();
}

void LoadBar::setValue(int v) {
    m_value = v;
    setVisible(v > 0 && v < 100);
    update();
}

void LoadBar::paintEvent(QPaintEvent*) {
    if (!m_value) return;
    QPainter p(this);
    const int w = int(width() * m_value / 100.0);
    QLinearGradient g(0, 0, w, 0);
    QColor acc(m_settings->customAccent());
    g.setColorAt(0, acc);
    g.setColorAt(1, QColor("#7f2eff"));
    p.fillRect(0, 0, w, 2, QBrush(g));
}
