#include "urlbar.h"
#include <QFont>
#include <QTimer>

UrlBar::UrlBar(QWidget* parent) : QLineEdit(parent) {
    setObjectName("urlBar");
    setFont(QFont("Segoe UI", 13));
    setAlignment(Qt::AlignCenter);
    setPlaceholderText("Поиск или адрес сайта");

    m_lock = new QLabel(this);
    m_lock->setFont(QFont("Segoe UI", 11));
    m_lock->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_lock->hide();

    m_reload = new QPushButton(this);
    m_reload->setObjectName("tbBtn");
    m_reload->setFont(QFont("Segoe UI", 12));
    m_reload->setText("↺");
    m_reload->setFixedSize(20, 20);
    m_reload->setCursor(Qt::PointingHandCursor);
    m_reload->setToolTip("Обновить / Стоп");
    m_reload->setFlat(true);
    m_reload->hide();

    connect(m_reload, &QPushButton::clicked, this, &UrlBar::reloadClicked);
}

void UrlBar::setUrl(const QString& url) {
    m_currentUrl = url;
    const bool internal = url.startsWith("about:");
    if (url.startsWith("https://"))
        m_lock->setText("🔒");
    else if (!url.isEmpty() && !internal)
        m_lock->setText("⚠");
    else
        m_lock->setText("");

    m_lock->setVisible(!url.isEmpty() && !internal);
    m_reload->setVisible(!url.isEmpty() && !internal);
    reposition();
}

void UrlBar::setLoading(bool loading) {
    m_loading = loading;
    m_reload->setText(loading ? "✕" : "↺");
}

void UrlBar::resizeEvent(QResizeEvent* e) {
    QLineEdit::resizeEvent(e);
    reposition();
}

void UrlBar::focusInEvent(QFocusEvent* e) {
    QLineEdit::focusInEvent(e);
    QTimer::singleShot(0, this, [this]{ selectAll(); });
}

void UrlBar::focusOutEvent(QFocusEvent* e) {
    QLineEdit::focusOutEvent(e);
    // Restore display URL after editing
    if (!m_currentUrl.startsWith("about:"))
        setText(m_currentUrl);
}

void UrlBar::reposition() {
    const int h = height();
    m_lock->adjustSize();
    m_lock->move(7, (h - m_lock->height()) / 2);
    m_reload->move(width() - 26, (h - 20) / 2);
}


