#pragma once
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

// URL bar with inline lock icon (left) and reload/stop button (right).
class UrlBar : public QLineEdit {
    Q_OBJECT
public:
    explicit UrlBar(QWidget* parent = nullptr);

    void setUrl(const QString& url);
    void setLoading(bool loading);
    bool isLoading() const { return m_loading; }

signals:
    void reloadClicked();

protected:
    void resizeEvent(QResizeEvent* e) override;
    void focusInEvent(QFocusEvent* e) override;
    void focusOutEvent(QFocusEvent* e) override;

private:
    void reposition();

    QLabel*      m_lock;
    QPushButton* m_reload;
    bool         m_loading = false;
    QString      m_currentUrl;
};
