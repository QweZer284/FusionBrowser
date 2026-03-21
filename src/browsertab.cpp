#include "browsertab.h"
#include "database.h"
#include "themes.h"
#include "settings.h"
#include "homepage.h"
#include <QWebEngineNewWindowRequest>
#include <QBuffer>
#include <QPixmap>

BrowserTab::BrowserTab(Database* db, QWebEngineProfile* profile,
                       ThemeManager* theme, Settings* settings,
                       QWidget* parent)
    : QWebEngineView(parent), m_db(db), m_theme(theme), m_settings(settings)
{
    auto* page = new QWebEnginePage(profile, this);
    setPage(page);

    connect(this, &QWebEngineView::titleChanged,   this, &BrowserTab::onTitle);
    connect(this, &QWebEngineView::urlChanged,     this, &BrowserTab::onUrlChanged);
    connect(this, &QWebEngineView::loadProgress,   this, &BrowserTab::sigProgress);
    connect(this, &QWebEngineView::iconChanged,    this, &BrowserTab::sigIcon);
    connect(page, &QWebEnginePage::newWindowRequested,
            this, &BrowserTab::onNewWindowRequested);
}

void BrowserTab::onTitle(const QString& t) {
    emit sigTitle(t);
    const QString url = this->url().toString();
    if (!url.isEmpty() && !url.startsWith("about:")) {
        m_db->addHistory(url, t, faviconDataUrl());
    }
}

void BrowserTab::onUrlChanged(const QUrl& url) {
    emit sigUrl(url.toString());
}

void BrowserTab::onNewWindowRequested(QWebEngineNewWindowRequest& req) {
    emit sigNewTab(req.requestedUrl());
    req.reject();
}

QString BrowserTab::faviconDataUrl() const {
    const QIcon ico = icon();
    if (ico.isNull()) return {};
    const QPixmap px = ico.pixmap(32, 32);
    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);
    px.save(&buf, "PNG");
    return "data:image/png;base64," + ba.toBase64();
}

void BrowserTab::go(const QString& rawUrl) {
    QString url = rawUrl.trimmed();
    if (url.isEmpty()) return;
    m_isNewTab = false;

    if (url.startsWith("http://") || url.startsWith("https://") ||
        url.startsWith("file://")  || url.startsWith("about:")) {
        setUrl(QUrl(url));
        return;
    }

    static const QMap<QString, QString> engines = {
        {"google", "https://www.google.com/search?q="},
        {"bing",   "https://www.bing.com/search?q="},
        {"ddg",    "https://duckduckgo.com/?q="},
    };
    const QString base = engines.value(m_settings->search(), engines["google"]);

    if (url.contains(' ') || !url.contains('.')) {
        setUrl(QUrl(base + QString(url).replace(' ', '+')));
    } else {
        setUrl(QUrl("https://" + url));
    }
}

void BrowserTab::showHomepage() {
    m_isNewTab = true;
    setHtml(generateHomepageHtml(m_db, m_theme, m_settings), QUrl("about:newtab"));
}
