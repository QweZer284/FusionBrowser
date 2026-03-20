#pragma once
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QIcon>
#include <QUrl>

class Database;
class ThemeManager;
class Settings;

class BrowserTab : public QWebEngineView {
    Q_OBJECT
public:
    BrowserTab(Database* db, QWebEngineProfile* profile,
               ThemeManager* theme, Settings* settings,
               QWidget* parent = nullptr);

    bool isNewTab() const { return m_isNewTab; }
    void go(const QString& url);
    void showHomepage();

    QString faviconDataUrl() const;

signals:
    void sigTitle(const QString& title);
    void sigUrl(const QString& url);
    void sigProgress(int pct);
    void sigIcon(const QIcon& icon);
    void sigNewTab(const QUrl& url);

private slots:
    void onTitle(const QString& t);
    void onUrlChanged(const QUrl& url);
    void onNewWindowRequested(QWebEngineNewWindowRequest& req);

private:
    Database*         m_db;
    ThemeManager*     m_theme;
    Settings*         m_settings;
    bool              m_isNewTab = false;
};
