#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QMap>
#include <QPushButton>

class ThemeManager;
class Settings;

class SettingsPanel : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPanel(ThemeManager* theme, Settings* settings, QWidget* parent = nullptr);

signals:
    void themeChanged();

private slots:
    void pickTheme();
    void applyTheme(const QString& themeName, const QString& accent);
    void setSearch(const QString& key);
    void save();
    void refreshThemeLabel();

private:
    ThemeManager*            m_theme;
    Settings*                m_settings;
    QLineEdit*               m_nameEdit;
    QLabel*                  m_themeLabel;
    QMap<QString, QPushButton*> m_searchBtns;
};
