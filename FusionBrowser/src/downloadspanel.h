#pragma once
#include <QWidget>
#include <QVBoxLayout>
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
    class QProgressBar* m_bar;
    class QLabel*       m_info;
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
