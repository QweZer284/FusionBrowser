#include "mainwindow.h"
#include "database.h"
#include "settings.h"
#include "themes.h"
#include "browsertab.h"
#include "tabbar.h"
#include "urlbar.h"
#include "loadbar.h"
#include "sidepanel.h"
#include "downloadspanel.h"
#include "settingspanel.h"
#include "extensions.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QKeySequence>
#include <QFileDialog>
#include <QWebEngineProfile>
#include <QWebEngineDownloadRequest>
#include <QCloseEvent>
#include <QDir>
#include <QFont>
#include <QIcon>

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <windowsx.h>
#endif

static constexpr int RESIZE_MARGIN = 6;

MainWindow::MainWindow(Database* db, Settings* settings, ThemeManager* theme,
                       QWidget* parent)
    : QMainWindow(parent), m_db(db), m_settings(settings), m_theme(theme)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setWindowTitle("Fusion");
    resize(1380, 900);
    setMinimumSize(800, 560);

    // Restore geometry
    const QByteArray geo = settings->windowGeometry();
    if (!geo.isEmpty()) restoreGeometry(geo);

    // ── WebEngine profile ─────────────────────────────────────────────
    const QString cacheDir = QDir::homePath() + "/.fusion9_cache";
    m_profile = new QWebEngineProfile("fusion9", this);
    m_profile->setCachePath(cacheDir);
    m_profile->setPersistentStoragePath(cacheDir);
    m_profile->setPersistentCookiesPolicy(
        QWebEngineProfile::AllowPersistentCookies);
    m_profile->setHttpUserAgent(
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
        "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36");
    connect(m_profile, &QWebEngineProfile::downloadRequested,
            this, &MainWindow::onDownloadRequested);

    // ── Extension manager (BEFORE first view) ─────────────────────────
    const QString extDir = QDir::homePath() + "/.fusion9_extensions";
    m_extMgr = new ExtensionManager(m_profile, extDir, this);
    for (const auto& ext : m_db->getExtensions()) {
        if (ext.enabled && QDir(ext.path).exists())
            m_extMgr->loadExt(ext.path);
    }

    buildUi();
    setupShortcuts();
    applyTheme();

    openNewTab(); // first tab
}

MainWindow::~MainWindow() {}

// ── Build UI ──────────────────────────────────────────────────────────────────
void MainWindow::buildUi() {
    auto* root = new QWidget;
    root->setObjectName("root");
    setCentralWidget(root);

    auto* rl = new QVBoxLayout(root);
    rl->setContentsMargins(0, 0, 0, 0);
    rl->setSpacing(0);

    // ── Tab strip ──────────────────────────────────────────────────────
    m_tabStrip = new TabStrip(this, m_settings);
    m_tabBar   = m_tabStrip->tabBar();
    connect(m_tabBar, &QTabBar::currentChanged,       this, &MainWindow::tabSwitched);
    connect(m_tabBar, &QTabBar::tabCloseRequested,    this, &MainWindow::closeTab);
    connect(m_tabStrip, &TabStrip::newTabRequested,   this, [this]{ openNewTab(); });
    rl->addWidget(m_tabStrip);

    // ── Toolbar ────────────────────────────────────────────────────────
    m_toolbar = new QWidget;
    m_toolbar->setObjectName("toolbar");
    m_toolbar->setFixedHeight(48);
    auto* tb = new QHBoxLayout(m_toolbar);
    tb->setContentsMargins(10, 9, 10, 9);
    tb->setSpacing(4);

    m_btnSb  = makeToolButton("⊞", "Боковая панель", 14, true);
    m_btnBack = makeToolButton("‹", "Назад  Alt+←", 18);
    m_btnFwd  = makeToolButton("›", "Вперёд  Alt+→", 18);
    tb->addWidget(m_btnSb);
    tb->addWidget(m_btnBack);
    tb->addWidget(m_btnFwd);
    tb->addSpacing(4);

    connect(m_btnSb,   &QPushButton::clicked, this, [this]{ togglePanel("sb"); });
    connect(m_btnBack, &QPushButton::clicked, this, &MainWindow::goBack);
    connect(m_btnFwd,  &QPushButton::clicked, this, &MainWindow::goFwd);

    m_urlBar = new UrlBar(m_toolbar);
    connect(m_urlBar, &QLineEdit::returnPressed, this, &MainWindow::go);
    connect(m_urlBar, &UrlBar::reloadClicked,    this, &MainWindow::reload);
    tb->addWidget(m_urlBar, 1);
    tb->addSpacing(4);

    m_btnBm = new QPushButton("☆");
    m_btnBm->setObjectName("tbBtn");
    m_btnBm->setFont(QFont("Segoe UI", 15));
    m_btnBm->setFixedSize(30, 30);
    m_btnBm->setToolTip("Закладка  Ctrl+D");
    connect(m_btnBm, &QPushButton::clicked, this, &MainWindow::toggleBookmark);
    tb->addWidget(m_btnBm);
    tb->addSpacing(2);

    m_btnDl  = makeToolButton("↓", "Загрузки", 14, true);
    m_btnExt = makeToolButton("⬡", "Расширения", 14, true);
    m_btnCfg = makeToolButton("⚙", "Настройки  Ctrl+,", 14, true);
    tb->addWidget(m_btnDl);
    tb->addWidget(m_btnExt);
    tb->addWidget(m_btnCfg);

    connect(m_btnDl,  &QPushButton::clicked, this, [this]{ togglePanel("dl"); });
    connect(m_btnExt, &QPushButton::clicked, this, [this]{ togglePanel("ext"); });
    connect(m_btnCfg, &QPushButton::clicked, this, [this]{ togglePanel("cfg"); });

    rl->addWidget(m_toolbar);

    // ── Load bar (2px, overlaid) ───────────────────────────────────────
    m_loadBar = new LoadBar(m_toolbar, m_settings);
    m_loadBar->setGeometry(0, 46, 1380, 2);

    // ── Content area ───────────────────────────────────────────────────
    auto* cw = new QWidget;
    auto* cl = new QHBoxLayout(cw);
    cl->setContentsMargins(0, 0, 0, 0);
    cl->setSpacing(0);

    m_sidebar = new SidebarPanel(m_db, cw);
    m_sidebar->hide();
    connect(m_sidebar, &SidebarPanel::navigateTo, this, &MainWindow::navTo);

    m_webStack = new QStackedWidget;

    m_dlPanel  = new DownloadsPanel(cw);
    m_dlPanel->hide();

    m_extPanel = new ExtensionPanel(m_db, m_extMgr, m_profile, cw);
    m_extPanel->setOpenTabFn([this](const QString& url){ openNewTab(url); });
    m_extPanel->hide();

    m_cfgPanel = new SettingsPanel(m_theme, m_settings, cw);
    m_cfgPanel->hide();
    connect(m_cfgPanel, &SettingsPanel::themeChanged, this, &MainWindow::onThemeChanged);

    cl->addWidget(m_sidebar);
    cl->addWidget(m_webStack, 1);
    cl->addWidget(m_dlPanel);
    cl->addWidget(m_extPanel);
    cl->addWidget(m_cfgPanel);
    rl->addWidget(cw, 1);

    // ── Status bar ─────────────────────────────────────────────────────
    m_statusBar = new QLabel;
    m_statusBar->setObjectName("statusBar");
    m_statusBar->hide();
    rl->addWidget(m_statusBar);

    // Panel maps
    m_panelWidgets  = {{"sb",m_sidebar},{"dl",m_dlPanel},{"ext",m_extPanel},{"cfg",m_cfgPanel}};
    m_panelButtons  = {{"sb",m_btnSb},  {"dl",m_btnDl},  {"ext",m_btnExt},  {"cfg",m_btnCfg}};
    m_panelOpen     = {{"sb",false},    {"dl",false},     {"ext",false},      {"cfg",false}};

    // Hover debounce (50 ms)
    m_hoverTimer.setSingleShot(true);
    m_hoverTimer.setInterval(50);
    connect(&m_hoverTimer, &QTimer::timeout, [this]{
        m_statusBar->setVisible(!m_statusBar->text().isEmpty());
    });

    // App icon
    const QString icoPath = QApplication::applicationDirPath() + "/fusion.ico";
    if (QFile::exists(icoPath))
        setWindowIcon(QIcon(icoPath));
}

// ── Shortcuts ─────────────────────────────────────────────────────────────────
void MainWindow::setupShortcuts() {
    auto sc = [this](const QString& key, auto&& fn) {
        auto* s = new QShortcut(QKeySequence(key), this);
        connect(s, &QShortcut::activated, this, fn);
    };

    sc("Ctrl+T",         [this]{ openNewTab(); });
    sc("Ctrl+W",         [this]{ closeCurrentTab(); });
    sc("Ctrl+L",         [this]{ m_urlBar->setFocus(); m_urlBar->selectAll(); });
    sc("Ctrl+D",         [this]{ toggleBookmark(); });
    sc("Ctrl+R",         [this]{ reload(); });
    sc("F5",             [this]{ reload(); });
    sc("F11",            [this]{
        if (isFullScreen()) showNormal(); else showFullScreen();
    });
    sc("F12",            [this]{ openDevTools(); });
    sc("Alt+Left",       [this]{ goBack(); });
    sc("Alt+Right",      [this]{ goFwd(); });
    sc("Ctrl+Tab",       [this]{
        m_tabBar->setCurrentIndex((m_tabBar->currentIndex()+1) % qMax(m_tabBar->count(),1));
    });
    sc("Ctrl+Shift+Tab", [this]{
        m_tabBar->setCurrentIndex((m_tabBar->currentIndex()-1+m_tabBar->count())
                                   % qMax(m_tabBar->count(),1));
    });
    sc("Ctrl+,",         [this]{ togglePanel("cfg"); });
    sc("Escape",         [this]{ if (isFullScreen()) showNormal(); });
}

// ── Tab Management ────────────────────────────────────────────────────────────
void MainWindow::openNewTab(const QString& url) {
    auto* t = new BrowserTab(m_db, m_profile, m_theme, m_settings, m_webStack);

    connect(t, &BrowserTab::sigTitle,    this, &MainWindow::onTabTitle);
    connect(t, &BrowserTab::sigUrl,      this, &MainWindow::onTabUrl);
    connect(t, &BrowserTab::sigProgress, m_loadBar, &LoadBar::setValue);
    connect(t, &BrowserTab::loadStarted, this, &MainWindow::onLoadStart);
    connect(t, &BrowserTab::loadFinished,this, &MainWindow::onLoadFinish);
    connect(t, &BrowserTab::sigIcon,     this, &MainWindow::onTabIcon);
    connect(t, &BrowserTab::sigNewTab,   this, &MainWindow::onNewTabRequested);
    connect(t->page(), &QWebEnginePage::linkHovered, this, &MainWindow::onHover);

    m_tabs.append(t);
    m_webStack->addWidget(t);
    const int idx = m_tabBar->addTab("Новая вкладка");
    m_tabBar->setCurrentIndex(idx);
    m_webStack->setCurrentWidget(t);

    if (!url.isEmpty()) t->go(url);
    else                t->showHomepage();

    updateNavButtons();
}

void MainWindow::closeTab(int idx) {
    if (m_tabBar->count() == 1) openNewTab();

    QPointer<BrowserTab> t = m_tabs.value(idx);
    if (!t) return;

    m_webStack->removeWidget(t);
    m_tabs.removeAt(idx);
    m_tabBar->removeTab(idx);

    if (t) t->deleteLater();
}

void MainWindow::closeCurrentTab() {
    closeTab(m_tabBar->currentIndex());
}

void MainWindow::tabSwitched(int idx) {
    if (idx < 0 || idx >= m_tabs.size()) return;
    auto* t = m_tabs[idx];
    m_webStack->setCurrentWidget(t);
    const QString u = t->url().toString();
    m_urlBar->setText(u.startsWith("about:") ? QString() : u);
    m_urlBar->setUrl(u);
    updateNavButtons();
    updateBookmarkButton();
}

void MainWindow::onNewTabRequested(const QUrl& url) {
    openNewTab(url.toString());
}

// ── Navigation ────────────────────────────────────────────────────────────────
BrowserTab* MainWindow::currentTab() const {
    const int i = m_tabBar->currentIndex();
    return (i >= 0 && i < m_tabs.size()) ? m_tabs[i] : nullptr;
}

int MainWindow::tabIndexOf(QObject* snd) const {
    for (int i = 0; i < m_tabs.size(); ++i)
        if (m_tabs[i] == snd) return i;
    return -1;
}

void MainWindow::go() {
    if (auto* t = currentTab()) t->go(m_urlBar->text());
}

void MainWindow::navTo(const QString& url) {
    if (auto* t = currentTab()) t->go(url);
}

void MainWindow::goBack() {
    if (auto* t = currentTab())
        if (t->history()->canGoBack()) t->back();
}

void MainWindow::goFwd() {
    if (auto* t = currentTab())
        if (t->history()->canGoForward()) t->forward();
}

void MainWindow::reload() {
    auto* t = currentTab();
    if (!t) return;
    if (m_urlBar->isLoading()) t->stop();
    else                        t->reload();
}

void MainWindow::openDevTools() {
    auto* t = currentTab();
    if (!t) return;
    auto* dv = new QWebEngineView(nullptr);
    dv->setAttribute(Qt::WA_DeleteOnClose);
    dv->setWindowTitle("Fusion DevTools");
    t->page()->setDevToolsPage(dv->page());
    dv->resize(1100, 700);
    dv->show();
}

// ── Tab signal handlers ───────────────────────────────────────────────────────
void MainWindow::onTabTitle(const QString& title) {
    auto* snd = qobject_cast<BrowserTab*>(sender());
    const int idx = tabIndexOf(snd);
    if (idx < 0) return;

    // Intercept CWS install
    if (title.startsWith("__fusion_install__")) {
        const QString installUrl = title.mid(18);
        m_extPanel->installFromCwsUrl(installUrl);
        if (snd) snd->page()->runJavaScript("document.title='Новая вкладка';");
        return;
    }

    QString display = title.isEmpty() || title.toLower().startsWith("about:")
        ? "Новая вкладка" : title;
    if (display.length() > 20) display = display.left(18) + "…";
    m_tabBar->setTabText(idx, display);
}

void MainWindow::onTabUrl(const QString& url) {
    if (sender() != currentTab()) return;
    m_urlBar->setText(url.startsWith("about:") ? QString() : url);
    m_urlBar->setUrl(url);
    updateNavButtons();
    updateBookmarkButton();
}

void MainWindow::onLoadStart() {
    m_urlBar->setLoading(true);
}

void MainWindow::onLoadFinish(bool) {
    m_urlBar->setLoading(false);
    updateNavButtons();
    updateBookmarkButton();
}

void MainWindow::onTabIcon(const QIcon& icon) {
    if (icon.isNull()) return;
    const int idx = tabIndexOf(sender());
    if (idx >= 0) m_tabBar->setTabIcon(idx, icon);
}

void MainWindow::onHover(const QString& url) {
    m_statusBar->setText(url);
    if (url.isEmpty()) {
        m_statusBar->hide();
    } else {
        // Debounced show
        m_hoverTimer.start();
    }
}

// ── Bookmark ──────────────────────────────────────────────────────────────────
void MainWindow::toggleBookmark() {
    auto* t = currentTab();
    if (!t) return;
    const QString url = t->url().toString();
    if (url.startsWith("about:")) return;

    if (m_db->isBookmark(url)) m_db->removeBookmark(url);
    else m_db->addBookmark(url, t->title(), t->faviconDataUrl());

    updateBookmarkButton();
    if (m_panelOpen["sb"]) m_sidebar->reloadBookmarks();
    refreshAllNewTabs();
}

void MainWindow::updateBookmarkButton() {
    auto* t = currentTab();
    if (!t) return;
    const bool bm = m_db->isBookmark(t->url().toString());
    m_btnBm->setText(bm ? "★" : "☆");
    m_btnBm->setStyleSheet(bm
        ? "color:#f5a623;background:transparent;border:none;" : "");
}

// ── Downloads ─────────────────────────────────────────────────────────────────
void MainWindow::onDownloadRequested(QWebEngineDownloadRequest* dl) {
    const QString dlDir = QDir::homePath() + "/Downloads";
    QDir().mkpath(dlDir);

    const QString path = QFileDialog::getSaveFileName(
        this, "Сохранить файл",
        dlDir + "/" + dl->suggestedFileName());

    if (!path.isEmpty()) {
        dl->setDownloadDirectory(QFileInfo(path).absolutePath());
        dl->setDownloadFileName(QFileInfo(path).fileName());
        dl->accept();
        m_dlPanel->addDownload(dl);
        if (!m_panelOpen["dl"]) togglePanel("dl");
    } else {
        dl->cancel();
    }
}

// ── Panels ────────────────────────────────────────────────────────────────────
void MainWindow::togglePanel(const QString& name) {
    // Right panels are mutually exclusive
    if (!m_panelOpen[name] && name != "sb") {
        for (const auto& n : {"dl","ext","cfg"}) {
            if (n != name && m_panelOpen[n]) {
                m_panelWidgets[n]->hide();
                m_panelButtons[n]->setChecked(false);
                m_panelOpen[n] = false;
            }
        }
    }

    m_panelOpen[name] = !m_panelOpen[name];
    m_panelWidgets[name]->setVisible(m_panelOpen[name]);
    m_panelButtons[name]->setChecked(m_panelOpen[name]);

    if (name == "sb" && m_panelOpen["sb"]) m_sidebar->reloadBookmarks();
    if (name == "ext"&& m_panelOpen["ext"]) m_extPanel->refresh();
}

// ── Theme ─────────────────────────────────────────────────────────────────────
void MainWindow::applyTheme() {
    setStyleSheet(m_theme->qss());
}

void MainWindow::onThemeChanged() {
    applyTheme();
    refreshAllNewTabs();
}

void MainWindow::refreshAllNewTabs() {
    for (auto* t : m_tabs)
        if (t && t->isNewTab())
            QTimer::singleShot(0, t, &BrowserTab::showHomepage);
}

// ── Helpers ───────────────────────────────────────────────────────────────────
void MainWindow::updateNavButtons() {
    auto* t = currentTab();
    m_btnBack->setEnabled(t && t->history()->canGoBack());
    m_btnFwd ->setEnabled(t && t->history()->canGoForward());
}

void MainWindow::repositionLoadBar() {
    if (m_loadBar)
        m_loadBar->setGeometry(0, 46, m_toolbar->width(), 2);
}

void MainWindow::resizeEvent(QResizeEvent* e) {
    QMainWindow::resizeEvent(e);
    repositionLoadBar();
}

void MainWindow::closeEvent(QCloseEvent* e) {
    m_settings->setWindowGeometry(saveGeometry());
    m_settings->save();
    QMainWindow::closeEvent(e);
}

QPushButton* MainWindow::makeToolButton(const QString& text, const QString& tip,
                                        int fontSize, bool checkable)
{
    auto* b = new QPushButton(text);
    b->setObjectName("tbBtn");
    b->setFont(QFont("Segoe UI", fontSize));
    b->setFixedSize(30, 30);
    b->setToolTip(tip);
    b->setCheckable(checkable);
    return b;
}

// ── Windows resize handles ────────────────────────────────────────────────────
#ifdef Q_OS_WIN
bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_NCHITTEST) {
            const int x = GET_X_LPARAM(msg->lParam);
            const int y = GET_Y_LPARAM(msg->lParam);
            const QRect g = frameGeometry();
            const int m   = RESIZE_MARGIN;

            const bool left   = (x - g.left())  < m;
            const bool right  = (g.right() - x) < m;
            const bool top    = (y - g.top())   < m;
            const bool bottom = (g.bottom() - y)< m;

            if (left  && top)    { *result = HTTOPLEFT;     return true; }
            if (right && top)    { *result = HTTOPRIGHT;    return true; }
            if (left  && bottom) { *result = HTBOTTOMLEFT;  return true; }
            if (right && bottom) { *result = HTBOTTOMRIGHT; return true; }
            if (left)            { *result = HTLEFT;        return true; }
            if (right)           { *result = HTRIGHT;       return true; }
            if (top)             { *result = HTTOP;         return true; }
            if (bottom)          { *result = HTBOTTOM;      return true; }
        }
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif
