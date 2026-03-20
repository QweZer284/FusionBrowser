#pragma once
#include <QObject>
#include <QWidget>
#include <QListWidget>
#include <QString>
#include <QList>
#include <functional>

class QWebEngineProfile;
class Database;

// CRX unpacking result
struct ExtInstallResult {
    bool    ok;
    QString path;
    QString name;
    QString error;
};

// ── Extension manager ─────────────────────────────────────────────────────────
class ExtensionManager : public QObject {
    Q_OBJECT
public:
    ExtensionManager(QWebEngineProfile* profile, const QString& storageDir,
                     QObject* parent = nullptr);

    bool    loadExt(const QString& path);
    QString storageDir() const { return m_dir; }

    // Install from local .crx file (synchronous)
    ExtInstallResult installFromFile(const QString& crxPath);

    // Install from CWS (async, emits done/failed)
    void installFromCws(const QString& urlOrId,
                        std::function<void(const QString& path, const QString& name)> onDone,
                        std::function<void(const QString& err)> onFail);

    void openPopup(const QString& extDir, QWidget* parent = nullptr);

    static QString extractExtId(const QString& urlOrId);
    static ExtInstallResult unpackCrx(const QByteArray& data, const QString& outDir);

private:
    void injectChromeApi();

    QWebEngineProfile* m_profile;
    QString            m_dir;
};

// ── Extension panel widget ────────────────────────────────────────────────────
class ExtensionPanel : public QWidget {
    Q_OBJECT
public:
    ExtensionPanel(Database* db, ExtensionManager* mgr,
                   QWebEngineProfile* profile, QWidget* parent = nullptr);

    void refresh();
    void setOpenTabFn(std::function<void(const QString&)> fn);
    void installFromCwsUrl(const QString& url);

signals:
    void extChanged();

private slots:
    void installLocal();
    void installUnpacked();
    void openCws();
    void showContextMenu(const QPoint& pos);
    void onItemDoubleClicked(QListWidgetItem* item);

private:
    void registerExt(const QString& name, const QString& path);

    Database*          m_db;
    ExtensionManager*  m_mgr;
    QWebEngineProfile* m_profile;
    QListWidget*       m_list;
    std::function<void(const QString&)> m_openTabFn;
};
