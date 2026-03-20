#include "sidepanel.h"
#include "database.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QFont>

static QFrame* makeSep() {
    auto* f = new QFrame;
    f->setFrameShape(QFrame::HLine);
    f->setStyleSheet("background: rgba(128,128,128,0.14); max-height: 1px;");
    return f;
}

SidebarPanel::SidebarPanel(Database* db, QWidget* parent)
    : QWidget(parent), m_db(db)
{
    setObjectName("sidePanel");

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 10, 0, 0);
    lay->setSpacing(0);

    // Segment buttons row
    auto* row = new QHBoxLayout;
    row->setContentsMargins(10, 0, 10, 10);
    row->setSpacing(4);

    m_btnBm = new QPushButton("Закладки");
    m_btnHi = new QPushButton("История");
    for (auto* b : {m_btnBm, m_btnHi}) {
        b->setObjectName("segBtn");
        b->setCheckable(true);
        row->addWidget(b);
    }
    m_btnBm->setChecked(true);

    connect(m_btnBm, &QPushButton::clicked, this, [this]{ showTab(0); });
    connect(m_btnHi, &QPushButton::clicked, this, [this]{ showTab(1); });

    lay->addLayout(row);
    lay->addWidget(makeSep());

    // Stack
    m_stack  = new QStackedWidget;
    m_bmList = new QListWidget;
    m_hiList = new QListWidget;
    m_stack->addWidget(m_bmList);
    m_stack->addWidget(m_hiList);
    lay->addWidget(m_stack, 1);

    // Clear history button
    auto* clr = new QPushButton("Очистить историю");
    clr->setStyleSheet(
        "margin:6px 10px;background:transparent;border:none;"
        "color:#ff3b30;font-size:11px;border-radius:7px;padding:5px 0;");
    connect(clr, &QPushButton::clicked, this, &SidebarPanel::clearHistory);
    lay->addWidget(clr);

    // Navigation on double-click
    auto navLambda = [this](QListWidgetItem* it){
        emit navigateTo(it->data(Qt::UserRole).toString());
    };
    connect(m_bmList, &QListWidget::itemDoubleClicked, this, navLambda);
    connect(m_hiList, &QListWidget::itemDoubleClicked, this, navLambda);

    reloadBookmarks();
}

void SidebarPanel::showTab(int idx) {
    m_stack->setCurrentIndex(idx);
    m_btnBm->setChecked(idx == 0);
    m_btnHi->setChecked(idx == 1);
    if (idx == 0) reloadBookmarks();
    else          reloadHistory();
}

void SidebarPanel::reloadBookmarks() {
    m_bmList->clear();
    for (const auto& r : m_db->getBookmarks()) {
        auto* it = new QListWidgetItem("🔖  " + (r.title.isEmpty() ? r.url : r.title));
        it->setData(Qt::UserRole, r.url);
        m_bmList->addItem(it);
    }
}

void SidebarPanel::reloadHistory() {
    m_hiList->clear();
    for (const auto& r : m_db->getHistory()) {
        auto* it = new QListWidgetItem("🕐  " + (r.title.isEmpty() ? r.url : r.title));
        it->setData(Qt::UserRole, r.url);
        m_hiList->addItem(it);
    }
}

void SidebarPanel::clearHistory() {
    m_db->clearHistory();
    m_hiList->clear();
}
