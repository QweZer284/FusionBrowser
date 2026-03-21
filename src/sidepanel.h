#pragma once
#include <QWidget>
#include <QListWidget>
#include <QStackedWidget>
#include <QPushButton>

class Database;

class SidebarPanel : public QWidget {
    Q_OBJECT
public:
    explicit SidebarPanel(Database* db, QWidget* parent = nullptr);

    void reloadBookmarks();
    void reloadHistory();

signals:
    void navigateTo(const QString& url);

private slots:
    void showTab(int idx);
    void clearHistory();

private:
    Database*      m_db;
    QStackedWidget* m_stack;
    QListWidget*    m_bmList;
    QListWidget*    m_hiList;
    QPushButton*    m_btnBm;
    QPushButton*    m_btnHi;
};
