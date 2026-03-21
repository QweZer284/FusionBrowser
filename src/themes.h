#pragma once
#include <QObject>
#include <QString>
#include <QMap>
#include <QColor>

struct ThemePreset {
    QString label;
    QString accent;
    bool    dark;
};

class Settings;

class ThemeManager : public QObject {
    Q_OBJECT
public:
    explicit ThemeManager(Settings* settings, QObject* parent = nullptr);

    static const QMap<QString, ThemePreset>& presets();

    QString currentName()  const;
    bool    isDark()       const;
    QString accent()       const;

    void apply(const QString& name, const QString& accentHex = {});

    QString qss() const;

    // Palette values for homepage HTML
    struct HomePalette {
        QString bg, h2, cardC, cHov, iBg, iSh, gr, em, acc;
    };
    HomePalette homepagePalette() const;

private:
    Settings* m_settings;
    QString   accRgb() const;
};
