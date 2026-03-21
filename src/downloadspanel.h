#pragma once
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
