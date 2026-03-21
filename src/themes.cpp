#include "themes.h"
#include "settings.h"
#include <QColor>

// ── Presets ───────────────────────────────────────────────────────────────────
const QMap<QString, ThemePreset>& ThemeManager::presets() {
    static const QMap<QString, ThemePreset> P = {
        {"light",       {"Светлая",        "#0a7aff", false}},
        {"dark",        {"Тёмная",         "#0a7aff", true }},
        {"blue",        {"Синяя",          "#0a7aff", false}},
        {"purple",      {"Фиолетовая",     "#7f2eff", false}},
        {"green",       {"Зелёная",        "#28c840", false}},
        {"red",         {"Красная",        "#ff3b30", false}},
        {"orange",      {"Оранжевая",      "#ff9500", false}},
        {"dark_blue",   {"Тёмная синяя",   "#0a7aff", true }},
        {"dark_purple", {"Тёмная фиолет.", "#7f2eff", true }},
        {"dark_green",  {"Тёмная зелёная", "#28c840", true }},
        {"custom",      {"Своя тема",      "#0a7aff", false}},
    };
    return P;
}

ThemeManager::ThemeManager(Settings* settings, QObject* parent)
    : QObject(parent), m_settings(settings) {}

QString ThemeManager::currentName() const {
    return m_settings->theme();
}

bool ThemeManager::isDark() const {
    const auto& P = presets();
    const QString name = currentName();
    if (P.contains(name)) return P[name].dark;
    return m_settings->dark();
}

QString ThemeManager::accent() const {
    const auto& P = presets();
    const QString name = currentName();
    if (name == "custom") return m_settings->customAccent();
    if (P.contains(name)) return P[name].accent;
    return "#0a7aff";
}

void ThemeManager::apply(const QString& name, const QString& accentHex) {
    m_settings->setTheme(name);
    const auto& P = presets();
    if (P.contains(name)) m_settings->setDark(P[name].dark);
    if (!accentHex.isEmpty()) m_settings->setCustomAccent(accentHex);
    m_settings->save();
}

QString ThemeManager::accRgb() const {
    QColor c(accent());
    return QString("%1,%2,%3").arg(c.red()).arg(c.green()).arg(c.blue());
}

// ── QSS Generation ────────────────────────────────────────────────────────────
QString ThemeManager::qss() const {
    const bool dark = isDark();
    const QString acc = accent();
    const QString rgb = accRgb();

    QString winBg, chromeBg, tsBg, tabOn, tabOnC, tabOffC, tabHov;
    QString urlBg, urlFocBg, urlC, btnC, btnHov, btnPre, btnDis;
    QString border, panelBg, panelBdr, lbl, lbl2, lbl3;
    QString liSel, liHov, dlBg, segOn, segBg, inpBg, stBg;

    if (dark) {
        winBg    = "#1c1c1e";
        chromeBg = "#2c2c2e";
        tsBg     = "#161618";
        tabOn    = "#3a3a3c";
        tabOnC   = "#ffffff";
        tabOffC  = "rgba(255,255,255,0.42)";
        tabHov   = "rgba(255,255,255,0.07)";
        urlBg    = "rgba(255,255,255,0.09)";
        urlFocBg = "#3a3a3c";
        urlC     = "#ffffff";
        btnC     = "rgba(255,255,255,0.72)";
        btnHov   = "rgba(255,255,255,0.10)";
        btnPre   = "rgba(255,255,255,0.18)";
        btnDis   = "rgba(255,255,255,0.24)";
        border   = "rgba(255,255,255,0.10)";
        panelBg  = "#2c2c2e";
        panelBdr = "rgba(255,255,255,0.09)";
        lbl      = "#ffffff";
        lbl2     = "rgba(255,255,255,0.72)";
        lbl3     = "#636366";
        liSel    = QString("rgba(%1,0.28)").arg(rgb);
        liHov    = "rgba(255,255,255,0.06)";
        dlBg     = "#3a3a3c";
        segOn    = "#3a3a3c";
        segBg    = "rgba(255,255,255,0.07)";
        inpBg    = "rgba(255,255,255,0.09)";
        stBg     = "rgba(28,28,30,0.96)";
    } else {
        winBg    = "#f0f0f0";
        chromeBg = "#ececec";
        tsBg     = "#d2d2d2";
        tabOn    = "rgba(255,255,255,0.96)";
        tabOnC   = "#1c1c1e";
        tabOffC  = "rgba(0,0,0,0.46)";
        tabHov   = "rgba(0,0,0,0.07)";
        urlBg    = "rgba(0,0,0,0.07)";
        urlFocBg = "#ffffff";
        urlC     = "#1c1c1e";
        btnC     = "rgba(0,0,0,0.68)";
        btnHov   = "rgba(0,0,0,0.08)";
        btnPre   = "rgba(0,0,0,0.14)";
        btnDis   = "rgba(0,0,0,0.26)";
        border   = "rgba(0,0,0,0.13)";
        panelBg  = "#f2f2f7";
        panelBdr = "rgba(0,0,0,0.10)";
        lbl      = "#1c1c1e";
        lbl2     = "#3c3c43";
        lbl3     = "#8e8e93";
        liSel    = QString("rgba(%1,0.12)").arg(rgb);
        liHov    = "rgba(0,0,0,0.05)";
        dlBg     = "#ffffff";
        segOn    = "#ffffff";
        segBg    = "rgba(0,0,0,0.06)";
        inpBg    = "rgba(0,0,0,0.07)";
        stBg     = "rgba(236,236,236,0.96)";
    }

    const QString chromeTop = chromeBg;
    const QString chromeBot = dark ? tsBg : "#e0e0e0";

    return QString(R"(
/* Fusion Browser — Generated QSS (theme: %1) */

* { font-family: -apple-system, 'Segoe UI', sans-serif; }

QMainWindow, QWidget#root { background: %2; }

/* ── Tab strip ── */
QWidget#tabStrip {
    background: %3;
    border-bottom: 1px solid %4;
    min-height: 34px; max-height: 34px;
}

/* ── Toolbar ── */
QWidget#toolbar {
    background: qlineargradient(x1:0,y1:0, x2:0,y2:1,
        stop:0 %5, stop:1 %6);
    border-bottom: 1px solid %4;
    min-height: 48px; max-height: 48px;
}

/* ── Tab bar ── */
QTabBar {
    background: transparent;
    border: none;
    qproperty-drawBase: 0;
    qproperty-expanding: 0;
}
QTabBar::tab {
    background: transparent;
    color: %7;
    border: none;
    border-radius: 7px;
    padding: 4px 6px 4px 8px;
    margin: 3px 1px;
    font-size: 12px;
    min-width: 60px; max-width: 200px;
    min-height: 22px; max-height: 22px;
}
QTabBar::tab:selected {
    background: %8;
    color: %9;
    font-weight: 500;
}
QTabBar::tab:hover:!selected { background: %10; }

/* ── URL bar ── */
QLineEdit#urlBar {
    background: %11;
    border: 1.5px solid transparent;
    border-radius: 10px;
    padding: 4px 28px 4px 26px;
    font-size: 13px;
    color: %12;
    selection-background-color: %13;
    min-height: 28px; max-height: 28px;
}
QLineEdit#urlBar:focus {
    background: %14;
    border-color: %13;
}

/* ── Toolbar buttons ── */
QPushButton#tbBtn {
    background: transparent; border: none; border-radius: 6px;
    color: %15;
    min-width: 30px; max-width: 30px;
    min-height: 30px; max-height: 30px;
    font-size: 14px; padding: 0;
}
QPushButton#tbBtn:hover   { background: %16; }
QPushButton#tbBtn:pressed { background: %17; }
QPushButton#tbBtn:disabled{ color: %18; }
QPushButton#tbBtn:checked { background: rgba(%19,0.14); color: %13; }

/* ── Traffic-light window controls ── */
QPushButton#tlClose {
    background: #ff5f57; border: 0.5px solid rgba(0,0,0,0.14);
    border-radius: 6px;
    min-width: 12px; max-width: 12px; min-height: 12px; max-height: 12px;
}
QPushButton#tlClose:hover { background: #ff3b30; }
QPushButton#tlMin {
    background: #febc2e; border: 0.5px solid rgba(0,0,0,0.14);
    border-radius: 6px;
    min-width: 12px; max-width: 12px; min-height: 12px; max-height: 12px;
}
QPushButton#tlMin:hover { background: #f0a500; }
QPushButton#tlMax {
    background: #28c840; border: 0.5px solid rgba(0,0,0,0.14);
    border-radius: 6px;
    min-width: 12px; max-width: 12px; min-height: 12px; max-height: 12px;
}
QPushButton#tlMax:hover { background: #20a330; }

/* ── + new-tab button ── */
QPushButton#plusBtn {
    background: transparent; border: none; border-radius: 5px;
    color: %15;
    min-width: 22px; max-width: 22px; min-height: 22px; max-height: 22px;
    font-size: 17px; font-weight: 300; padding: 0;
}
QPushButton#plusBtn:hover { background: %16; }

/* ── Side panels ── */
QWidget#sidePanel {
    background: %20;
    border-right: 1px solid %21;
    min-width: 260px; max-width: 260px;
}
QWidget#rightPanel {
    background: %20;
    border-left: 1px solid %21;
    min-width: 290px; max-width: 290px;
}

QLabel#panelHead { font-size: 15px; font-weight: 600; color: %22; }
QLabel#hint      { font-size: 11px; color: %23; }

/* ── Segmented control ── */
QPushButton#segBtn {
    background: %24; border: none; border-radius: 7px;
    color: %25; font-size: 12px; font-weight: 500;
    padding: 5px 14px; min-height: 28px;
}
QPushButton#segBtn:checked { background: %26; color: %22; }
QPushButton#segBtn:hover:!checked { background: %16; }

/* ── Generic action button ── */
QPushButton#actionBtn {
    background: %13; border: none; border-radius: 9px;
    color: white; font-size: 13px; font-weight: 500;
    min-height: 36px; padding: 0 20px;
}
QPushButton#actionBtn:hover { background: rgba(%19,0.85); }

/* ── Lists ── */
QListWidget {
    background: transparent; border: none; outline: none; font-size: 12px;
}
QListWidget::item {
    padding: 8px 14px; border-radius: 8px; color: %25; margin: 1px 8px;
}
QListWidget::item:selected { background: %27; color: %13; }
QListWidget::item:hover:!selected { background: %28; }

/* ── Download item ── */
QFrame#dlItem { background: %29; border-radius: 10px; margin: 3px 10px; }
QProgressBar {
    background: rgba(128,128,128,0.13); border: none;
    border-radius: 2px; max-height: 4px; font-size: 0px;
}
QProgressBar::chunk {
    background: %13;
    border-radius: 2px;
}

/* ── Settings inputs ── */
QLineEdit#settingsInput {
    background: %30; border: 1px solid %4;
    border-radius: 9px; padding: 7px 12px;
    font-size: 13px; color: %12; min-height: 32px;
}
QLineEdit#settingsInput:focus { border-color: %13; background: %14; }

/* ── Color swatch button ── */
QPushButton#swatchBtn {
    border: 2px solid transparent;
    border-radius: 10px;
    min-width: 36px; max-width: 36px;
    min-height: 36px; max-height: 36px;
    padding: 0;
}
QPushButton#swatchBtn:checked { border-color: %13; }

/* ── Scrollbars ── */
QScrollBar:vertical { background: transparent; width: 5px; }
QScrollBar::handle:vertical {
    background: rgba(128,128,128,0.26); border-radius: 3px; min-height: 24px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { background: transparent; height: 5px; }
QScrollBar::handle:horizontal {
    background: rgba(128,128,128,0.26); border-radius: 3px;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

/* ── Status bar ── */
QLabel#statusBar {
    background: %31; color: %23;
    font-size: 11px; padding: 1px 12px;
    border-top: 1px solid %4;
    min-height: 16px; max-height: 16px;
}

/* ── Dialog / Popup ── */
QDialog { background: %2; }
QDialog QLabel { color: %22; }
)")
    .arg(currentName()) // %1
    .arg(winBg)         // %2
    .arg(tsBg)          // %3
    .arg(border)        // %4
    .arg(chromeTop)     // %5
    .arg(chromeBot)     // %6
    .arg(tabOffC)       // %7
    .arg(tabOn)         // %8
    .arg(tabOnC)        // %9
    .arg(tabHov)        // %10
    .arg(urlBg)         // %11
    .arg(urlC)          // %12
    .arg(acc)           // %13
    .arg(urlFocBg)      // %14
    .arg(btnC)          // %15
    .arg(btnHov)        // %16
    .arg(btnPre)        // %17
    .arg(btnDis)        // %18
    .arg(rgb)           // %19
    .arg(panelBg)       // %20
    .arg(panelBdr)      // %21
    .arg(lbl)           // %22
    .arg(lbl3)          // %23
    .arg(segBg)         // %24
    .arg(lbl2)          // %25
    .arg(segOn)         // %26
    .arg(liSel)         // %27
    .arg(liHov)         // %28
    .arg(dlBg)          // %29
    .arg(inpBg)         // %30
    .arg(stBg);         // %31
}

ThemeManager::HomePalette ThemeManager::homepagePalette() const {
    const bool dark = isDark();
    const QString acc = accent();
    if (dark) {
        return {
            "linear-gradient(160deg,#1a1a2e 0%,#16213e 52%,#0f3460 100%)",
            "rgba(255,255,255,0.72)",
            "rgba(255,255,255,0.54)",
            "rgba(255,255,255,0.10)",
            "#2c2c2e",
            "0 2px 14px rgba(0,0,0,0.44)",
            "rgba(255,255,255,0.88)",
            "rgba(255,255,255,0.28)",
            acc
        };
    } else {
        return {
            "linear-gradient(160deg,#ddd8f0 0%,#d2e8f8 52%,#d0ecf8 100%)",
            "rgba(28,28,30,0.72)",
            "rgba(28,28,30,0.54)",
            "rgba(255,255,255,0.65)",
            "#ffffff",
            "0 2px 10px rgba(0,0,0,0.10)",
            "rgba(28,28,30,0.88)",
            "rgba(28,28,30,0.30)",
            acc
        };
    }
}
