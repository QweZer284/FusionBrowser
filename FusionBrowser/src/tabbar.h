#pragma once
#include <QTabBar>
#include <QAbstractButton>
#include <QPainter>
#include <QTimer>

class Settings;

// ── Close button drawn manually ───────────────────────────────────────────────
class TabCloseButton : public QAbstractButton {
    Q_OBJECT
public:
    explicit TabCloseButton(QTabBar* bar);
    QSize sizeHint() const override { return {16, 16}; }

protected:
    void enterEvent(QEnterEvent*) override { m_hover = true;  update(); }
    void leaveEvent(QEvent*)      override { m_hover = false; update(); }
    void paintEvent(QPaintEvent*) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    QTabBar* m_bar;
    bool     m_hover = false;
    int      tabIndex() const;
};

// ── Custom tab bar ────────────────────────────────────────────────────────────
class FusionTabBar : public QTabBar {
    Q_OBJECT
public:
    explicit FusionTabBar(Settings* settings, QWidget* parent = nullptr);

signals:
    void tabLayoutChanged();

protected:
    void tabInserted(int index) override;
    void tabRemoved(int index)  override;

private:
    Settings* m_settings;
};

// ── Tab strip (traffic lights + tab bar + + button) ───────────────────────────
class TabStrip : public QWidget {
    Q_OBJECT
public:
    explicit TabStrip(QWidget* mainWin, Settings* settings);

    FusionTabBar* tabBar() const { return m_tabBar; }

signals:
    void newTabRequested();

protected:
    void resizeEvent(QResizeEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;

private slots:
    void repositionPlusButton();

private:
    static constexpr int TL_W     = 72;
    static constexpr int PLUS_W   = 22;
    static constexpr int PLUS_H   = 22;
    static constexpr int PLUS_GAP = 4;

    QWidget*      m_mainWin;
    FusionTabBar* m_tabBar;
    QPushButton*  m_plus;
    QPoint        m_dragStart;
    bool          m_dragging = false;
};
