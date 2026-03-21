import os, re

BASE = "FusionBrowser/src"

# ── 1. downloadspanel.h ───────────────────────────────────────────────────────
open(f"{BASE}/downloadspanel.h", "w").write("""#pragma once
#include <QWidget>
#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QList>
#include <QWebEngineDownloadRequest>

class DownloadItem : public QFrame {
    Q_OBJECT
public:
    explicit DownloadItem(QWebEngineDownloadRequest* dl, QWidget* parent = nullptr);
private slots:
    void tick();
    void done();
private:
    QWebEngineDownloadRequest* m_dl;
    QProgressBar* m_bar;
    QLabel*       m_info;
};

class DownloadsPanel : public QWidget {
    Q_OBJECT
public:
    explicit DownloadsPanel(QWidget* parent = nullptr);
    void addDownload(QWebEngineDownloadRequest* dl);
private slots:
    void clearAll();
private:
    QVBoxLayout*         m_itemLayout;
    QList<DownloadItem*> m_items;
};
""")

# ── 2. tabbar.h ───────────────────────────────────────────────────────────────
open(f"{BASE}/tabbar.h", "w").write("""#pragma once
#include <QTabBar>
#include <QAbstractButton>
#include <QPainter>
#include <QTimer>
#include <QPoint>
#include <QPushButton>
#include <QWidget>
#include <QEnterEvent>

class Settings;

class TabCloseButton : public QAbstractButton {
    Q_OBJECT
public:
    explicit TabCloseButton(QTabBar* bar);
    QSize sizeHint() const override { return {16, 16}; }
protected:
    void enterEvent(QEnterEvent*) override { m_hover = true;  update(); }
    void leaveEvent(QEvent*)      override { m_hover = false; update(); }
    void paintEvent(QPaintEvent*) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
private:
    QTabBar* m_bar;
    bool     m_hover = false;
    int      tabIndex() const;
};

class FusionTabBar : public QTabBar {
    Q_OBJECT
public:
    explicit FusionTabBar(Settings* settings, QWidget* parent = nullptr);
signals:
    void tabLayoutChanged();
protected:
    void tabInserted(int index) override;
    void tabRemoved(int index)  override;
private:
    Settings* m_settings;
};

class TabStrip : public QWidget {
    Q_OBJECT
public:
    explicit TabStrip(QWidget* mainWin, Settings* settings);
    FusionTabBar* tabBar() const { return m_tabBar; }
signals:
    void newTabRequested();
protected:
    void resizeEvent(QResizeEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
private slots:
    void repositionPlusButton();
private:
    enum {
        TL_W     = 72,
        PLUS_W   = 22,
        PLUS_H   = 22,
        PLUS_GAP = 4
    };
    QWidget*      m_mainWin;
    FusionTabBar* m_tabBar;
    QPushButton*  m_plus;
    QPoint        m_dragStart;
    bool          m_dragging = false;
};
""")

# ── 3. mainwindow.cpp – fix GET_X/Y_LPARAM ───────────────────────────────────
mw = open(f"{BASE}/mainwindow.cpp").read()
if "#include <windowsx.h>" not in mw:
    mw = mw.replace("#include <windows.h>", "#include <windows.h>\n#include <windowsx.h>")
    open(f"{BASE}/mainwindow.cpp", "w").write(mw)

# ── 4. settingspanel.cpp – remove structured bindings ────────────────────────
sp = open(f"{BASE}/settingspanel.cpp").read()
# Replace structured binding loop for engines
sp = sp.replace(
    'for (const auto& [k, lb] : engines) {',
    'for (int _i = 0; _i < engines.size(); ++_i) { const QString k = engines[_i].first; const QString lb = engines[_i].second;'
)
open(f"{BASE}/settingspanel.cpp", "w").write(sp)

# ── 5. onboarding.cpp – remove tuple structured bindings ─────────────────────
ob = open(f"{BASE}/onboarding.cpp").read()

# Fix QPair loop in step 2
ob = ob.replace(
    'for (const auto& [k, lb] : QList<QPair<QString,QString>>{',
    'for (const auto& _p : QList<QPair<QString,QString>>{'
)
# Need to add the binding after the brace - this is tricky, let's do full replacement

old_pair_loop = '''        for (const auto& _p : QList<QPair<QString,QString>>{
            {"google","Google"},{"bing","Bing"},{"ddg","DuckDuckGo"}})
        {
            auto* b = new QPushButton(lb);'''
new_pair_loop = '''        for (const auto& _kv : QList<QPair<QString,QString>>{
            {"google","Google"},{"bing","Bing"},{"ddg","DuckDuckGo"}})
        {
            const QString k = _kv.first;
            const QString lb = _kv.second;
            auto* b = new QPushButton(lb);'''
ob = ob.replace(old_pair_loop, new_pair_loop)

# Fix std::tuple structured binding
old_tuple = '''        for (const auto& [key, lbl, emoji, desc] : QList<std::tuple<QString,QString,QString,QString>>{
            {"light","Светлая","☀️","Светлый фон, тёмный текст"},
            {"dark", "Тёмная", "🌙","Тёмный фон, мягкий для глаз"},
        }) {
            auto* btn = new QPushButton(QString("  %1  %2  —  %3").arg(emoji, lbl, desc));'''
new_tuple = '''        struct ThemeEntry { QString key, lbl, emoji, desc; };
        for (const auto& te : QList<ThemeEntry>{
            {"light","Светлая","☀️","Светлый фон, тёмный текст"},
            {"dark", "Тёмная", "🌙","Тёмный фон, мягкий для глаз"},
        }) {
            const QString key = te.key, lbl = te.lbl, emoji = te.emoji, desc = te.desc;
            auto* btn = new QPushButton(QString("  %1  %2  —  %3").arg(emoji, lbl, desc));'''
ob = ob.replace(old_tuple, new_tuple)

open(f"{BASE}/onboarding.cpp", "w").write(ob)

# ── 6. extensions.cpp – fix structured binding ───────────────────────────────
ex = open(f"{BASE}/extensions.cpp").read()
old_acc = '''        const QList<QPair<QString,QString>> accents = {
            {"#0a7aff","Синий"}, {"#7f2eff","Фиолетовый"}, {"#28c840","Зелёный"},
            {"#ff3b30","Красный"}, {"#ff9500","Оранжевый"}, {"#ff375f","Розовый"},
            {"#32ade6","Голубой"}, {"#ff6b35","Коралловый"},
        };

        auto* grid = new QGridLayout;
        grid->setSpacing(8);
        for (int i = 0; i < accents.size(); ++i) {
            const auto& [hex, label] = accents[i];'''
new_acc = '''        const QList<QPair<QString,QString>> accents = {
            {"#0a7aff","Синий"}, {"#7f2eff","Фиолетовый"}, {"#28c840","Зелёный"},
            {"#ff3b30","Красный"}, {"#ff9500","Оранжевый"}, {"#ff375f","Розовый"},
            {"#32ade6","Голубой"}, {"#ff6b35","Коралловый"},
        };

        auto* grid = new QGridLayout;
        grid->setSpacing(8);
        for (int i = 0; i < accents.size(); ++i) {
            const QString hex   = accents[i].first;
            const QString label = accents[i].second;'''
ex = ex.replace(old_acc, new_acc)
open(f"{BASE}/extensions.cpp", "w").write(ex)

# ── 7. importer.cpp – fix structured bindings ────────────────────────────────
im = open(f"{BASE}/importer.cpp").read()
old_im = '''    for (const auto& [k, lb] : QList<QPair<QString,QString>>{
        {"google","Google"}, {"bing","Bing"}, {"ddg","DDG"}
    }) {'''
new_im = '''    for (const auto& _e : QList<QPair<QString,QString>>{
        {"google","Google"}, {"bing","Bing"}, {"ddg","DDG"}
    }) { const QString k = _e.first; const QString lb = _e.second;'''
im = im.replace(old_im, new_im)
open(f"{BASE}/importer.cpp", "w").write(im)

# ── 8. browsertab.cpp – use QWebEngineNewWindowRequest ───────────────────────
bt = open(f"{BASE}/browsertab.cpp").read()
if '#include <QWebEngineNewWindowRequest>' not in bt:
    bt = bt.replace('#include "browsertab.h"', '#include "browsertab.h"\n#include <QWebEngineNewWindowRequest>')
    open(f"{BASE}/browsertab.cpp", "w").write(bt)

# ── 9. urlbar.cpp – fix focusInEvent ─────────────────────────────────────────
ub = open(f"{BASE}/urlbar.cpp").read()
# Make sure QTimer is included
if '#include <QTimer>' not in ub:
    ub = ub.replace('#include "urlbar.h"', '#include "urlbar.h"\n#include <QTimer>')
    open(f"{BASE}/urlbar.cpp", "w").write(ub)

# ── 10. homepage.cpp – fix QRegularExpression ────────────────────────────────
hp = open(f"{BASE}/homepage.cpp").read()
if '#include <QRegularExpression>' not in hp:
    hp = hp.replace('#include <QDateTime>', '#include <QDateTime>\n#include <QRegularExpression>')
    open(f"{BASE}/homepage.cpp", "w").write(hp)

print("All patches applied OK")
