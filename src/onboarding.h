#pragma once
#include <QDialog>
#include <QStackedWidget>
#include <QPushButton>
#include <QList>
#include <QLabel>
#include <functional>

class Database;
class Settings;
class ThemeManager;

bool needsOnboarding();
void markOnboarded();

class OnboardingWizard : public QDialog {
    Q_OBJECT
public:
    OnboardingWizard(Database* db, Settings* settings,
                     ThemeManager* theme, QWidget* parent = nullptr);

signals:
    void finished(Settings* settings);

private slots:
    void goNext();
    void goBack();
    void doSkip();
    void importDone();

private:
    void goTo(int idx);
    void startImport(const QStringList& keys);
    QString wizQss() const;

    Database*      m_db;
    Settings*      m_settings;
    ThemeManager*  m_theme;

    QStackedWidget* m_stack;
    QPushButton*    m_btnNext;
    QPushButton*    m_btnBack;
    QPushButton*    m_btnSkip;
    QList<QLabel*>  m_dots;
    int             m_cur = -1;

    // Step 1 — import
    QWidget* m_importStep   = nullptr;
    class QProgressBar* m_importBar  = nullptr;
    QLabel*  m_importLabel  = nullptr;
    QList<class QCheckBox*> m_importCbs;
    QStringList m_importKeys;

    // Step 2 — profile
    class QLineEdit* m_nameEdit = nullptr;

    // Step 3 — theme
    QMap<QString, QPushButton*> m_themeCards;

    class QThread* m_importThread = nullptr;
};
