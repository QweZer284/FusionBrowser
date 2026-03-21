#include <QApplication>
#include <QWebEngineSettings>
#include <QFont>
#include <QIcon>
#include <QDir>
#include <QFile>

// Qt WebEngine requires initialization before QApplication on some platforms
#include <QtWebEngineWidgets/QtWebEngineWidgets>
#include <QtWebEngineCore/QtWebEngineCore>

#include "database.h"
#include "settings.h"
#include "themes.h"
#include "mainwindow.h"
#include "onboarding.h"

int main(int argc, char* argv[]) {
    // ── Must set before QApplication ─────────────────────────────────
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    qputenv("QT_ENABLE_HIGHDPI_SCALING",  "1");
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-logging");

    // WebEngine initialization (Qt 6.6+)
    // QtWebEngineQuick::initialize() is for Quick; for Widgets we rely on
    // AA_ShareOpenGLContexts set above.

    QApplication app(argc, argv);
    app.setApplicationName("Fusion");
    app.setApplicationDisplayName("Fusion Browser");
    app.setApplicationVersion("9.0");
    app.setOrganizationName("Fusion Browser");
    app.setOrganizationDomain("fusion.browser");
    app.setFont(QFont("Segoe UI", 10));

    const QString icoPath = QApplication::applicationDirPath() + "/fusion.ico";
    if (QFile::exists(icoPath))
        app.setWindowIcon(QIcon(icoPath));

    // ── Core objects ──────────────────────────────────────────────────
    Settings    settings;
    Database    db;
    ThemeManager theme(&settings);

    // ── Onboarding (first run) ────────────────────────────────────────
    if (needsOnboarding()) {
        OnboardingWizard wiz(&db, &settings, &theme);
        QObject::connect(&wiz, &OnboardingWizard::finished,
                         [&](Settings* s){ Q_UNUSED(s); });
        wiz.exec();
    }

    // ── Main window ───────────────────────────────────────────────────
    MainWindow win(&db, &settings, &theme);
    win.show();

    return app.exec();
}
