#pragma once
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QString>
#include <QList>
#include <QMutex>
#include <QVariant>

struct HistoryRow  { QString url; QString title; QString favicon; };
struct BookmarkRow { QString url; QString title; QString favicon; };
struct ExtRow      { int id; QString name; QString path; bool enabled; };

class Database : public QObject {
    Q_OBJECT
public:
    explicit Database(QObject* parent = nullptr);
    ~Database() override;

    // ── History ──────────────────────────────────────────────────────
    void addHistory(const QString& url, const QString& title, const QString& favicon = {});
    QList<HistoryRow> getHistory(int n = 200) const;
    QList<HistoryRow> getFrequent(int n = 12) const;
    void clearHistory();

    // ── Bookmarks ────────────────────────────────────────────────────
    bool addBookmark(const QString& url, const QString& title, const QString& favicon = {});
    void removeBookmark(const QString& url);
    QList<BookmarkRow> getBookmarks() const;
    bool isBookmark(const QString& url) const;

    // ── Extensions ───────────────────────────────────────────────────
    void addExtension(const QString& name, const QString& path);
    QList<ExtRow> getExtensions() const;
    void toggleExtension(int id, bool enabled);
    void removeExtension(int id);

private:
    QString   m_dbPath;
    QSqlDatabase m_db;
    mutable QMutex m_mutex;

    void execScript(const QString& sql);
};
