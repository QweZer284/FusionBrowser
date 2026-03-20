#include "onboarding.h"
#include "database.h"
#include "settings.h"
#include "themes.h"
#include "importer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QProgressBar>
#include <QFrame>
#include <QFont>
#include <QDir>
#include <QThread>
#include <QApplication>
#include <functional>

// ── Onboarding flag ───────────────────────────────────────────────────────────
static QString flagPath() {
    return QDir::homePath() + "/.fusion_setup_v9";
}

bool needsOnboarding() {
    return !QFile::exists(flagPath());
}

void markOnboarded() {
    QFile f(flagPath());
    f.open(QIODevice::WriteOnly);
}

// ── Helpers ───────────────────────────────────────────────────────────────────
static QFrame* makeSep() {
    auto* f = new QFrame;
    f->setFrameShape(QFrame::HLine);
    f->setStyleSheet("background:rgba(128,128,128,0.14);max-height:1px;");
    return f;
}

static QLabel* makeHead(const QString& text, int size = 22) {
    auto* l = new QLabel(text);
    l->setFont(QFont("Segoe UI", size, QFont::Bold));
    l->setWordWrap(true);
    return l;
}

static QLabel* makeSub(const QString& text) {
    auto* l = new QLabel(text);
    l->setObjectName("sub");
    l->setFont(QFont("Segoe UI", 13));
    l->setWordWrap(true);
    return l;
}

// ── OnboardingWizard ──────────────────────────────────────────────────────────
OnboardingWizard::OnboardingWizard(Database* db, Settings* settings,
                                   ThemeManager* theme, QWidget* parent)
    : QDialog(parent), m_db(db), m_settings(settings), m_theme(theme)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setFixedSize(500, 560);
    setStyleSheet(wizQss());

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(36, 28, 36, 24);
    root->setSpacing(0);

    // Dots
    auto* dotsRow = new QHBoxLayout;
    dotsRow->setSpacing(6);
    for (int i = 0; i < 5; ++i) {
        auto* d = new QLabel("●");
        d->setFont(QFont("Segoe UI", 8));
        m_dots.append(d);
        dotsRow->addWidget(d);
    }
    dotsRow->addStretch();
    root->addLayout(dotsRow);
    root->addSpacing(14);

    m_stack = new QStackedWidget;

    // ── Step 0: Welcome ───────────────────────────────────────────────
    {
        auto* w   = new QWidget;
        auto* lay = new QVBoxLayout(w);
        lay->setSpacing(14);
        lay->setAlignment(Qt::AlignHCenter);

        auto* logo = new QLabel("F");
        logo->setFixedSize(80, 80);
        logo->setAlignment(Qt::AlignCenter);
        logo->setFont(QFont("Segoe UI", 34, QFont::Bold));
        logo->setStyleSheet(
            "background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
            "stop:0 #1a6eff,stop:1 #7f2eff);"
            "border-radius:20px; color:white;");
        lay->addWidget(logo, 0, Qt::AlignHCenter);

        auto* t = makeHead("Добро пожаловать в Fusion", 22);
        t->setAlignment(Qt::AlignHCenter);
        lay->addWidget(t);

        auto* s = makeSub("Быстрый браузер на движке Chromium.\nНастройка займёт около минуты.");
        s->setAlignment(Qt::AlignHCenter);
        lay->addWidget(s);
        lay->addStretch();
        m_stack->addWidget(w);
    }

    // ── Step 1: Import ────────────────────────────────────────────────
    {
        auto* w   = new QWidget;
        m_importStep = w;
        auto* lay = new QVBoxLayout(w);
        lay->setSpacing(10);
        lay->addWidget(makeHead("Импорт данных"));
        lay->addWidget(makeSub("Выбери браузеры для переноса закладок и истории."));
        lay->addWidget(makeSep());

        const QStringList found = detectInstalledBrowsers();
        if (found.isEmpty()) {
            lay->addWidget(makeSub("Установленные браузеры не найдены — пропусти этот шаг."));
        } else {
            for (const auto& key : found) {
                auto* cb = new QCheckBox(browserDisplayName(key));
                cb->setChecked(true);
                m_importCbs.append(cb);
                m_importKeys.append(key);
                lay->addWidget(cb);
            }
        }

        lay->addWidget(makeSep());

        m_importLabel = new QLabel("");
        m_importLabel->setObjectName("sub");
        m_importLabel->hide();
        lay->addWidget(m_importLabel);

        m_importBar = new QProgressBar;
        m_importBar->setRange(0, 100);
        m_importBar->hide();
        lay->addWidget(m_importBar);

        lay->addStretch();
        m_stack->addWidget(w);
    }

    // ── Step 2: Profile ───────────────────────────────────────────────
    {
        auto* w   = new QWidget;
        auto* lay = new QVBoxLayout(w);
        lay->setSpacing(12);
        lay->addWidget(makeHead("Твой профиль"));
        lay->addWidget(makeSub("Fusion будет приветствовать тебя по имени."));
        lay->addWidget(makeSep());

        auto* nl = new QLabel("Как тебя зовут?");
        nl->setFont(QFont("Segoe UI", 12, QFont::Medium));
        lay->addWidget(nl);

        m_nameEdit = new QLineEdit(settings->name());
        m_nameEdit->setObjectName("wizInp");
        m_nameEdit->setPlaceholderText("Введи имя…");
        m_nameEdit->setFont(QFont("Segoe UI", 14));
        lay->addWidget(m_nameEdit);

        lay->addWidget(makeSep());

        auto* sl = new QLabel("Поиск по умолчанию");
        sl->setFont(QFont("Segoe UI", 12, QFont::Medium));
        lay->addWidget(sl);

        auto* srow = new QHBoxLayout;
        srow->setSpacing(8);
        const QString curSearch = settings->search();
        for (const auto& [k, lb] : QList<QPair<QString,QString>>{
            {"google","Google"},{"bing","Bing"},{"ddg","DuckDuckGo"}})
        {
            auto* b = new QPushButton(lb);
            b->setObjectName("segBtn");
            b->setCheckable(true);
            b->setChecked(curSearch == k);
            connect(b, &QPushButton::clicked, this, [this, k, b]{
                m_settings->setSearch(k);
            });
            srow->addWidget(b);
        }
        lay->addLayout(srow);
        lay->addStretch();
        m_stack->addWidget(w);
    }

    // ── Step 3: Theme ─────────────────────────────────────────────────
    {
        auto* w   = new QWidget;
        auto* lay = new QVBoxLayout(w);
        lay->setSpacing(10);
        lay->addWidget(makeHead("Выбери тему"));
        lay->addWidget(makeSub("Можно изменить в любой момент (⚙ → Тема)."));
        lay->addWidget(makeSep());

        const QString curTheme = settings->theme();
        for (const auto& [key, lbl, emoji, desc] : QList<std::tuple<QString,QString,QString,QString>>{
            {"light","Светлая","☀️","Светлый фон, тёмный текст"},
            {"dark", "Тёмная", "🌙","Тёмный фон, мягкий для глаз"},
        }) {
            auto* btn = new QPushButton(QString("  %1  %2  —  %3").arg(emoji, lbl, desc));
            btn->setCheckable(true);
            btn->setChecked(key == curTheme);
            btn->setFont(QFont("Segoe UI", 12));
            btn->setMinimumHeight(44);
            btn->setObjectName("segBtn");
            connect(btn, &QPushButton::clicked, this, [this, key]{
                const auto& P = ThemeManager::presets();
                m_settings->setTheme(key);
                m_settings->setDark(P[key].dark);
                for (auto* b : m_themeCards.values()) b->setChecked(false);
                if (m_themeCards.contains(key)) m_themeCards[key]->setChecked(true);
            });
            m_themeCards[key] = btn;
            lay->addWidget(btn);
        }
        lay->addStretch();
        m_stack->addWidget(w);
    }

    // ── Step 4: Done ──────────────────────────────────────────────────
    {
        auto* w   = new QWidget;
        auto* lay = new QVBoxLayout(w);
        lay->setSpacing(14);
        lay->setAlignment(Qt::AlignHCenter);

        auto* ico = new QLabel("✅");
        ico->setFont(QFont("Segoe UI", 52));
        ico->setAlignment(Qt::AlignHCenter);
        lay->addWidget(ico);

        auto* t = makeHead("Fusion готов!", 22);
        t->setAlignment(Qt::AlignHCenter);
        lay->addWidget(t);

        auto* s = makeSub("Настройки сохранены. Приятного пользования!");
        s->setAlignment(Qt::AlignHCenter);
        lay->addWidget(s);
        lay->addStretch();
        m_stack->addWidget(w);
    }

    root->addWidget(m_stack, 1);
    root->addSpacing(14);

    // Nav buttons
    auto* nav = new QHBoxLayout;
    nav->setSpacing(8);

    m_btnSkip = new QPushButton("Пропустить");
    m_btnSkip->setObjectName("wizSkip");
    connect(m_btnSkip, &QPushButton::clicked, this, &OnboardingWizard::doSkip);
    nav->addWidget(m_btnSkip);
    nav->addStretch();

    m_btnBack = new QPushButton("Назад");
    m_btnBack->setObjectName("wizBack");
    m_btnBack->hide();
    connect(m_btnBack, &QPushButton::clicked, this, &OnboardingWizard::goBack);
    nav->addWidget(m_btnBack);

    m_btnNext = new QPushButton("Начать");
    m_btnNext->setObjectName("wizNext");
    m_btnNext->setFixedWidth(130);
    connect(m_btnNext, &QPushButton::clicked, this, &OnboardingWizard::goNext);
    nav->addWidget(m_btnNext);

    root->addLayout(nav);

    goTo(0);
}

void OnboardingWizard::goTo(int idx) {
    m_cur = idx;
    m_stack->setCurrentIndex(idx);
    const bool last = (idx == 4);
    m_btnBack->setVisible(idx > 0 && idx < 4);
    m_btnSkip->setVisible(idx == 1);
    m_btnNext->setText(last ? "Готово" : (idx > 0 ? "Продолжить" : "Начать"));

    for (int i = 0; i < m_dots.size(); ++i) {
        m_dots[i]->setStyleSheet(
            i == idx ? "color:#0a7aff;" : "color:rgba(128,128,128,0.30);");
    }
}

void OnboardingWizard::goNext() {
    if (m_cur == 1) {
        QStringList selectedKeys;
        for (int i = 0; i < m_importCbs.size(); ++i)
            if (m_importCbs[i]->isChecked()) selectedKeys << m_importKeys[i];
        if (!selectedKeys.isEmpty()) {
            startImport(selectedKeys);
            return;
        }
    }
    if (m_cur == 4) {
        // Finish
        m_settings->setName(m_nameEdit ? m_nameEdit->text().trimmed() : QString());
        m_settings->save();
        markOnboarded();
        emit finished(m_settings);
        accept();
        return;
    }
    goTo(m_cur + 1);
}

void OnboardingWizard::goBack() {
    if (m_cur > 0) goTo(m_cur - 1);
}

void OnboardingWizard::doSkip() {
    goTo(m_cur + 1);
}

void OnboardingWizard::startImport(const QStringList& keys) {
    m_importLabel->show();
    m_importBar->show();
    for (auto* cb : m_importCbs) cb->setEnabled(false);
    m_btnNext->setEnabled(false);
    m_btnBack->setEnabled(false);
    m_btnSkip->setEnabled(false);

    auto* thread = new QThread(this);
    m_importThread = thread;

    // Run in thread via lambda + signals
    auto* worker = new QObject();
    worker->moveToThread(thread);

    QStringList keyCopy = keys;
    Database*   dbRef   = m_db;

    connect(thread, &QThread::started, worker, [this, worker, keyCopy, dbRef]{
        runImport(keyCopy, dbRef, [this](int pct, const QString& msg){
            // Cross-thread signal via queued connection
            QMetaObject::invokeMethod(this, [this, pct, msg]{
                m_importBar->setValue(pct);
                m_importLabel->setText(msg);
            }, Qt::QueuedConnection);
        });
        QMetaObject::invokeMethod(this, &OnboardingWizard::importDone,
                                  Qt::QueuedConnection);
        worker->deleteLater();
    });

    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void OnboardingWizard::importDone() {
    if (m_importThread) { m_importThread->quit(); m_importThread = nullptr; }
    m_btnNext->setEnabled(true);
    goTo(2);
}

QString OnboardingWizard::wizQss() const {
    const bool dark = m_settings->dark();
    const QString bg  = dark ? "#1c1c1e" : "#f2f2f7";
    const QString txt = dark ? "#ffffff"  : "#1c1c1e";
    const QString sub = dark ? "rgba(255,255,255,0.52)" : "rgba(28,28,30,0.52)";
    const QString ibg = dark ? "rgba(255,255,255,0.09)" : "rgba(0,0,0,0.06)";
    const QString ibr = dark ? "rgba(255,255,255,0.15)" : "rgba(0,0,0,0.12)";
    const QString ib2 = dark ? "#3a3a3c" : "#ffffff";
    const QString sbg = dark ? "rgba(255,255,255,0.07)" : "rgba(0,0,0,0.06)";
    const QString sc  = dark ? "rgba(255,255,255,0.60)" : "#3c3c43";
    const QString son = dark ? "#3a3a3c" : "#ffffff";
    const QString sc2 = dark ? "#ffffff"  : "#1c1c1e";

    return QString(R"(
QDialog   { background: %1; }
QLabel    { color: %2; font-family:'Segoe UI'; }
QLabel#sub{ color: %3; }
QLineEdit#wizInp {
    background: %4; border: 1.5px solid %5;
    border-radius: 10px; padding: 8px 12px;
    font-size: 14px; color: %2; min-height: 36px;
}
QLineEdit#wizInp:focus { border-color: #0a7aff; background: %6; }
QPushButton#wizNext {
    background: #0a7aff; border: none; border-radius: 11px;
    color: white; font-size: 14px; font-weight: 500;
    min-height: 42px; padding: 0 32px;
}
QPushButton#wizNext:hover    { background: #0060d0; }
QPushButton#wizNext:disabled { background: rgba(10,122,255,0.36); }
QPushButton#wizBack, QPushButton#wizSkip {
    background: %7; border: none; border-radius: 11px;
    color: %8; font-size: 13px; min-height: 42px; padding: 0 24px;
}
QPushButton#wizBack:hover, QPushButton#wizSkip:hover {
    background: rgba(128,128,128,0.16);
}
QPushButton#segBtn {
    background: %7; border: none; border-radius: 8px;
    color: %8; font-size: 12px; font-weight: 500;
    padding: 6px 16px; min-height: 30px;
}
QPushButton#segBtn:checked { background: %9; color: %10; }
QCheckBox { color: %2; font-size: 13px; spacing: 8px; }
QCheckBox::indicator {
    width: 18px; height: 18px; border-radius: 5px;
    border: 1.5px solid %5; background: %4;
}
QCheckBox::indicator:checked { background: #0a7aff; border-color: #0a7aff; }
QProgressBar {
    background: rgba(128,128,128,0.15); border: none;
    border-radius: 4px; min-height: 8px; max-height: 8px; font-size: 0;
}
QProgressBar::chunk {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 #0a7aff, stop:1 #7f2eff);
    border-radius: 4px;
}
)")
    .arg(bg).arg(txt).arg(sub).arg(ibg).arg(ibr)
    .arg(ib2).arg(sbg).arg(sc).arg(son).arg(sc2);
}
