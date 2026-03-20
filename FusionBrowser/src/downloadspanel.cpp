#include "downloadspanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QScrollArea>
#include <QFrame>
#include <QFont>

// ── DownloadItem ──────────────────────────────────────────────────────────────
DownloadItem::DownloadItem(QWebEngineDownloadRequest* dl, QWidget* parent)
    : QFrame(parent), m_dl(dl)
{
    setObjectName("dlItem");

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(10, 8, 10, 8);
    lay->setSpacing(4);

    QString name = dl->downloadFileName();
    if (name.isEmpty()) name = "file";
    QString short_name = name.length() > 30 ? name.left(29) + "…" : name;

    auto* lbl = new QLabel(short_name);
    lbl->setFont(QFont("Segoe UI", 11, QFont::Medium));
    lay->addWidget(lbl);

    m_bar = new QProgressBar;
    m_bar->setRange(0, 100);
    m_bar->setValue(0);
    m_bar->setFixedHeight(4);
    lay->addWidget(m_bar);

    m_info = new QLabel("Загрузка…");
    m_info->setFont(QFont("Segoe UI", 10));
    m_info->setStyleSheet("color: #8e8e93;");
    lay->addWidget(m_info);

    connect(dl, &QWebEngineDownloadRequest::receivedBytesChanged, this, &DownloadItem::tick);
    connect(dl, &QWebEngineDownloadRequest::isFinishedChanged,    this, &DownloadItem::done);
}

void DownloadItem::tick() {
    qint64 rx  = m_dl->receivedBytes();
    qint64 tot = m_dl->totalBytes();
    if (tot > 0) {
        m_bar->setValue(int(rx * 100 / tot));
        m_info->setText(QString("%1 / %2 MB")
            .arg(rx  / 1048576.0, 0, 'f', 1)
            .arg(tot / 1048576.0, 0, 'f', 1));
    }
}

void DownloadItem::done() {
    const bool ok = m_dl->state() == QWebEngineDownloadRequest::DownloadCompleted;
    m_bar->setValue(ok ? 100 : m_bar->value());
    if (ok) {
        m_info->setText("✓  Сохранено");
        m_info->setStyleSheet("color:#30d158;font-size:10px;");
    } else {
        m_info->setText("✗  Отменено");
        m_info->setStyleSheet("color:#ff3b30;font-size:10px;");
    }
}

// ── DownloadsPanel ────────────────────────────────────────────────────────────
DownloadsPanel::DownloadsPanel(QWidget* parent) : QWidget(parent) {
    setObjectName("rightPanel");

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // Header
    auto* hdr = new QHBoxLayout;
    hdr->setContentsMargins(14, 12, 12, 8);
    auto* title = new QLabel("Загрузки");
    title->setObjectName("panelHead");
    hdr->addWidget(title);
    hdr->addStretch();

    auto* clrBtn = new QPushButton("Очистить");
    clrBtn->setStyleSheet(
        "background:transparent;border:none;color:#ff3b30;"
        "font-size:12px;border-radius:6px;padding:4px 8px;");
    connect(clrBtn, &QPushButton::clicked, this, &DownloadsPanel::clearAll);
    hdr->addWidget(clrBtn);
    lay->addLayout(hdr);

    // Separator
    auto* sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("background:rgba(128,128,128,0.14);max-height:1px;");
    lay->addWidget(sep);

    // Scroll area
    auto* sc   = new QScrollArea;
    sc->setWidgetResizable(true);
    sc->setFrameShape(QFrame::NoFrame);

    auto* cont = new QWidget;
    m_itemLayout = new QVBoxLayout(cont);
    m_itemLayout->setContentsMargins(0, 6, 0, 6);
    m_itemLayout->setSpacing(2);
    m_itemLayout->addStretch();

    sc->setWidget(cont);
    lay->addWidget(sc);
}

void DownloadsPanel::addDownload(QWebEngineDownloadRequest* dl) {
    auto* item = new DownloadItem(dl, this);
    m_itemLayout->insertWidget(0, item);
    m_items.append(item);
}

void DownloadsPanel::clearAll() {
    for (auto* it : m_items) {
        it->setParent(nullptr);
        it->deleteLater();
    }
    m_items.clear();
}
