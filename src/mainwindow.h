#pragma once
#include <QMainWindow>
#include <QList>
#include <QPointer>
#include <QTimer>

class Database;
class Settings;
class ThemeManager;
class BrowserTab;
class TabStrip;
class FusionTabBar;
class UrlBar;
class LoadBar;
class SidebarPanel;
class DownloadsPanel;
class ExtensionPanel;
class SettingsPanel;
class QWebEngineProfile;
class QStackedWidget;
class QLabel;
class QPushButton;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(Database* db, Settings* settings, ThemeManager* theme,
               QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void resizeEvent(QResizeEvent*) override;
    void closeEvent(QCloseEvent*)  override;

    // Native resize handles for frameless window
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray& eventType, void* message,
                     qintptr* result) override;
#endif

private slots:
    // Tab management
    void openNewTab(const QString& url = {});
    void closeTab(int idx);
    void closeCurrentTab();
    void tabSwitched(int idx);
    void onNewTabRequested(const QUrl& url);

    // Navigation
    void go();
    void navTo(const QString& url);
    void goBack();
    void goFwd();
    void reload();
    void openDevTools();

    // Tab signal handlers (use sender() for proper tab lookup)
    void onTabTitle(const QString& title);
    void onTabUrl(const QString& url);
    void onLoadStart();
    void onLoadFinish(bool ok);
    void onTabIcon(const QIcon& icon);
    void onHover(const QString& url);

    // Bookmark
    void toggleBookmark();

    // Download
    void onDownloadRequested(QWebEngineDownloadRequest* dl);

    // Panels
    void togglePanel(const QString& name);

    // Theme
    void onThemeChanged();

private:
    void buildUi();
    void setupShortcuts();
    void applyTheme();
    void updateNavButtons();
    void updateBookmarkButton();
    void repositionLoadBar();
    void refreshAllNewTabs();

    BrowserTab*  currentTab() const;
    int          tabIndexOf(QObject* sender) const;

    QPushButton* makeToolButton(const QString& text, const QString& tip,
                                int fontSize = 14, bool checkable = false);

    // Core
    Database*          m_db;
    Settings*          m_settings;
    ThemeManager*      m_theme;
    QWebEngineProfile* m_profile;

    // Tabs
    QList<BrowserTab*> m_tabs;

    // UI
    TabStrip*       m_tabStrip;
    FusionTabBar*   m_tabBar;
    QWidget*        m_toolbar;
    UrlBar*         m_urlBar;
    LoadBar*        m_loadBar;
    QStackedWidget* m_webStack;

    // Toolbar buttons
    QPushButton* m_btnSb;
    QPushButton* m_btnBack;
    QPushButton* m_btnFwd;
    QPushButton* m_btnBm;
    QPushButton* m_btnDl;
    QPushButton* m_btnExt;
    QPushButton* m_btnCfg;

    // Panels
    SidebarPanel*    m_sidebar;
    DownloadsPanel*  m_dlPanel;
    ExtensionPanel*  m_extPanel;
    SettingsPanel*   m_cfgPanel;
    QLabel*          m_statusBar;

    // Panel visibility state
    QMap<QString, bool>        m_panelOpen;
    QMap<QString, QWidget*>    m_panelWidgets;
    QMap<QString, QPushButton*> m_panelButtons;

    // Status bar debounce
    QTimer m_hoverTimer;
    QTimer m_statusTimer;

    // Extension manager
    class ExtensionManager* m_extMgr;
};
