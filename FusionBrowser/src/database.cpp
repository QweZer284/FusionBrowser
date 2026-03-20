#include "database.h"
#include <QDir>
#include <QDateTime>
#include <QMutexLocker>
#include <QtSql/QSqlError>
#include <QDebug>

Database::Database(QObject* parent) : QObject(parent) {
    m_dbPath = QDir::homePath() + "/.fusion9.db";
    m_db = QSqlDatabase::addDatabase("QSQLITE", "fusion9_conn");
    m_db.setDatabaseName(m_dbPath);

    if (!m_db.open()) {
        qWarning() << "Database::open failed:" << m_db.lastError().text();
        return;
    }

    // WAL mode for better concurrency
    QSqlQuery q(m_db);
    q.exec("PRAGMA journal_mode=WAL");

    execScript(R"(
        CREATE TABLE IF NOT EXISTS history(
            id      INTEGER PRIMARY KEY AUTOINCREMENT,
            url     TEXT,
            title   TEXT,
            favicon TEXT,
            ts      TEXT);
        CREATE TABLE IF NOT EXISTS bookmarks(
            id      INTEGER PRIMARY KEY AUTOINCREMENT,
            url     TEXT UNIQUE,
            title   TEXT,
            favicon TEXT,
            ts      TEXT);
        CREATE TABLE IF NOT EXISTS extensions(
            id      INTEGER PRIMARY KEY AUTOINCREMENT,
            name    TEXT,
            path    TEXT,
            enabled INTEGER DEFAULT 1,
            ts      TEXT);
    )");
}

Database::~Database() {
    m_db.close();
}

void Database::execScript(const QString& sql) {
    // Split on semicolon and execute each statement
    const auto stmts = sql.split(';', Qt::SkipEmptyParts);
    QSqlQuery q(m_db);
    for (const auto& s : stmts) {
        const QString trimmed = s.trimmed();
        if (!trimmed.isEmpty()) {
            if (!q.exec(trimmed)) {
                qWarning() << "DB exec error:" << q.lastError().text() << "\n" << trimmed;
            }
        }
    }
}

// ── History ───────────────────────────────────────────────────────────────────
void Database::addHistory(const QString& url, const QString& title, const QString& favicon) {
    QMutexLocker lk(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO history(url,title,favicon,ts) VALUES(?,?,?,?)");
    q.addBindValue(url);
    q.addBindValue(title);
    q.addBindValue(favicon);
    q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    q.exec();
}

QList<HistoryRow> Database::getHistory(int n) const {
    QMutexLocker lk(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("SELECT url,title,favicon FROM history ORDER BY id DESC LIMIT ?");
    q.addBindValue(n);
    q.exec();
    QList<HistoryRow> out;
    while (q.next())
        out.append({q.value(0).toString(), q.value(1).toString(), q.value(2).toString()});
    return out;
}

QList<HistoryRow> Database::getFrequent(int n) const {
    QMutexLocker lk(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT url,title,favicon,COUNT(*) c FROM history
        WHERE url NOT LIKE 'about:%'
        GROUP BY url ORDER BY c DESC LIMIT ?)");
    q.addBindValue(n);
    q.exec();
    QList<HistoryRow> out;
    while (q.next())
        out.append({q.value(0).toString(), q.value(1).toString(), q.value(2).toString()});
    return out;
}

void Database::clearHistory() {
    QMutexLocker lk(&m_mutex);
    QSqlQuery q(m_db);
    q.exec("DELETE FROM history");
}

// ── Bookmarks ─────────────────────────────────────────────────────────────────
bool Database::addBookmark(const QString& url, const QString& title, const QString& favicon) {
    QMutexLocker lk(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO bookmarks(url,title,favicon,ts) VALUES(?,?,?,?)");
    q.addBindValue(url);
    q.addBindValue(title);
    q.addBindValue(favicon);
    q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    return q.exec();
}

void Database::removeBookmark(const QString& url) {
    QMutexLocker lk(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM bookmarks WHERE url=?");
    q.addBindValue(url);
    q.exec();
}

QList<BookmarkRow> Database::getBookmarks() const {
    QMutexLocker lk(&m_mutex);
    QSqlQuery q(m_db);
    q.exec("SELECT url,title,favicon FROM bookmarks ORDER BY id ASC");
    QList<BookmarkRow> out;
    while (q.next())
        out.append({q.value(0).toString(), q.value(1).toString(), q.value(2).toString()});
    return out;
}

bool Database::isBookmark(const QString& url) const {
    QMutexLocker lk(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("SELECT 1 FROM bookmarks WHERE url=?");
    q.addBindValue(url);
    q.exec();
    return q.next();
}

// ── Extensions ────────────────────────────────────────────────────────────────
void Database::addExtension(const QString& name, const QString& path) {
    QMutexLocker lk(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO extensions(name,path,enabled,ts) VALUES(?,?,1,?)");
    q.addBindValue(name);
    q.addBindValue(path);
    q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    q.exec();
}

QList<ExtRow> Database::getExtensions() const {
    QMutexLocker lk(&m_mutex);
    QSqlQuery q(m_db);
    q.exec("SELECT id,name,path,enabled FROM extensions");
    QList<ExtRow> out;
    while (q.next())
        out.append({q.value(0).toInt(), q.value(1).toString(),
                    q.value(2).toString(), q.value(3).toBool()});
    return out;
}

void Database::toggleExtension(int id, bool enabled) {
    QMutexLocker lk(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("UPDATE extensions SET enabled=? WHERE id=?");
    q.addBindValue(enabled ? 1 : 0);
    q.addBindValue(id);
    q.exec();
}

void Database::removeExtension(int id) {
    QMutexLocker lk(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM extensions WHERE id=?");
    q.addBindValue(id);
    q.exec();
}
