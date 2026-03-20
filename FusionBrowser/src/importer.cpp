#include "importer.h"
#include "database.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QUuid>
#include <QTemporaryFile>

// ── Path helpers ──────────────────────────────────────────────────────────────
static QMap<QString, QString> chromiumPaths() {
    const QString lad = qEnvironmentVariable("LOCALAPPDATA");
    const QString app = qEnvironmentVariable("APPDATA");
    return {
        {"chrome",   lad + R"(\Google\Chrome\User Data\Default)"},
        {"chromium", lad + R"(\Chromium\User Data\Default)"},
        {"brave",    lad + R"(\BraveSoftware\Brave-Browser\User Data\Default)"},
        {"edge",     lad + R"(\Microsoft\Edge\User Data\Default)"},
        {"opera",    app + R"(\Opera Software\Opera Stable)"},
        {"opera_gx", app + R"(\Opera Software\Opera GX Stable)"},
        {"vivaldi",  lad + R"(\Vivaldi\User Data\Default)"},
    };
}

static QMap<QString, QString> displayNames() {
    return {
        {"chrome",   "Google Chrome"},
        {"chromium", "Chromium"},
        {"brave",    "Brave"},
        {"edge",     "Microsoft Edge"},
        {"opera",    "Opera"},
        {"opera_gx", "Opera GX"},
        {"vivaldi",  "Vivaldi"},
        {"firefox",  "Firefox"},
    };
}

static QString bestFirefoxProfile() {
    const QString base = qEnvironmentVariable("APPDATA") + R"(\Mozilla\Firefox\Profiles)";
    if (!QDir(base).exists()) return {};
    for (const auto& e : QDir(base).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (e.fileName().contains("default-release", Qt::CaseInsensitive))
            return e.filePath();
    }
    for (const auto& e : QDir(base).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (e.fileName().contains("default", Qt::CaseInsensitive))
            return e.filePath();
    }
    const auto entries = QDir(base).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    return entries.isEmpty() ? QString() : entries.first().filePath();
}

QStringList detectInstalledBrowsers() {
    QStringList found;
    const auto paths = chromiumPaths();
    for (auto it = paths.begin(); it != paths.end(); ++it)
        if (QDir(it.value()).exists())
            found << it.key();
    if (!bestFirefoxProfile().isEmpty())
        found << "firefox";
    return found;
}

QString browserDisplayName(const QString& key) {
    return displayNames().value(key, key);
}

// ── Safe SQLite access ────────────────────────────────────────────────────────
static QString copyToTemp(const QString& src) {
    if (!QFile::exists(src)) return {};
    const QString ext = QFileInfo(src).suffix();
    const QString dst = QDir::tempPath() + "/_fusion_import_" +
                        QUuid::createUuid().toString(QUuid::WithoutBraces) +
                        "." + ext;
    return QFile::copy(src, dst) ? dst : QString();
}

static QSqlDatabase openTempDb(const QString& path, const QString& connName) {
    auto db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName("file:" + path + "?mode=ro&immutable=1");
    db.setConnectOptions("QSQLITE_OPEN_URI");
    db.open();
    return db;
}

// ── Chromium bookmark tree walker ─────────────────────────────────────────────
static void walkBookmarks(const QJsonObject& node, QList<BookmarkRow>& out) {
    if (node.value("type").toString() == "url") {
        const QString url = node.value("url").toString();
        if (url.startsWith("http://") || url.startsWith("https://"))
            out.append({url, node.value("name").toString(), {}});
    }
    for (const auto& child : node.value("children").toArray())
        walkBookmarks(child.toObject(), out);
}

static QList<BookmarkRow> chromiumBookmarks(const QString& profileDir) {
    QFile f(profileDir + "/Bookmarks");
    if (!f.open(QIODevice::ReadOnly)) return {};
    auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return {};
    const auto roots = doc.object().value("roots").toObject();
    QList<BookmarkRow> out;
    for (const auto& key : {"bookmark_bar", "other", "synced"})
        walkBookmarks(roots.value(key).toObject(), out);
    return out;
}

static QList<HistoryRow> chromiumHistory(const QString& profileDir, int limit = 500) {
    const QString tmp = copyToTemp(profileDir + "/History");
    if (tmp.isEmpty()) return {};
    const QString conn = "imp_ch_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto db = openTempDb(tmp, conn);
    QList<HistoryRow> out;
    if (db.isOpen()) {
        QSqlQuery q(db);
        q.prepare("SELECT url,title FROM urls ORDER BY visit_count DESC,last_visit_time DESC LIMIT ?");
        q.addBindValue(limit);
        q.exec();
        while (q.next()) {
            const QString url = q.value(0).toString();
            if (url.startsWith("http://") || url.startsWith("https://"))
                out.append({url, q.value(1).toString()});
        }
    }
    db.close();
    QSqlDatabase::removeDatabase(conn);
    QFile::remove(tmp);
    return out;
}

static QList<BookmarkRow> firefoxBookmarks(int limit = 500) {
    const QString profile = bestFirefoxProfile();
    if (profile.isEmpty()) return {};
    const QString tmp  = copyToTemp(profile + "/places.sqlite");
    if (tmp.isEmpty()) return {};
    const QString conn = "imp_ff_bm_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto db = openTempDb(tmp, conn);
    QList<BookmarkRow> out;
    if (db.isOpen()) {
        QSqlQuery q(db);
        q.prepare(R"(
            SELECT p.url,b.title FROM moz_bookmarks b
            JOIN moz_places p ON b.fk=p.id
            WHERE b.type=1 AND p.url LIKE 'http%'
            ORDER BY b.dateAdded DESC LIMIT ?)");
        q.addBindValue(limit);
        q.exec();
        while (q.next())
            out.append({q.value(0).toString(), q.value(1).toString(), {}});
    }
    db.close();
    QSqlDatabase::removeDatabase(conn);
    QFile::remove(tmp);
    return out;
}

static QList<HistoryRow> firefoxHistory(int limit = 500) {
    const QString profile = bestFirefoxProfile();
    if (profile.isEmpty()) return {};
    const QString tmp  = copyToTemp(profile + "/places.sqlite");
    if (tmp.isEmpty()) return {};
    const QString conn = "imp_ff_hi_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto db = openTempDb(tmp, conn);
    QList<HistoryRow> out;
    if (db.isOpen()) {
        QSqlQuery q(db);
        q.prepare(R"(
            SELECT url,title FROM moz_places
            WHERE url LIKE 'http%' AND visit_count>0
            ORDER BY visit_count DESC LIMIT ?)");
        q.addBindValue(limit);
        q.exec();
        while (q.next())
            out.append({q.value(0).toString(), q.value(1).toString()});
    }
    db.close();
    QSqlDatabase::removeDatabase(conn);
    QFile::remove(tmp);
    return out;
}

// ── Public runImport ──────────────────────────────────────────────────────────
void runImport(const QStringList& keys, Database* db,
               std::function<void(int, const QString&)> progressCb)
{
    const auto paths = chromiumPaths();
    const int total  = keys.size() * 2;
    int done = 0;

    auto tick = [&](const QString& msg) {
        ++done;
        if (progressCb) progressCb(done * 100 / qMax(total, 1), msg);
    };

    for (const auto& key : keys) {
        const QString name = browserDisplayName(key);
        // Bookmarks
        try {
            const auto bms = (key == "firefox")
                ? firefoxBookmarks()
                : chromiumBookmarks(paths.value(key));
            for (const auto& r : bms)
                db->addBookmark(r.url, r.title, r.favicon);
        } catch (...) {}
        tick("Закладки: " + name);

        // History
        try {
            const auto hist = (key == "firefox")
                ? firefoxHistory()
                : chromiumHistory(paths.value(key));
            for (const auto& r : hist)
                db->addHistory(r.url, r.title);
        } catch (...) {}
        tick("История: " + name);
    }
}
