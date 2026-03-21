#pragma once
#include <QWidget>

class Settings;

// 2-pixel gradient progress bar overlaid at the bottom of the toolbar.
class LoadBar : public QWidget {
    Q_OBJECT
public:
    explicit LoadBar(QWidget* parent, Settings* settings);

public slots:
    void setValue(int v);   // 0–100; hides when v==0 or v==100

protected:
    void paintEvent(QPaintEvent*) override;

private:
    int      m_value = 0;
    Settings* m_settings;
};
