#include "settingspanel.h"
#include "themes.h"
#include "settings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QFont>
#include <QDialog>
#include <QGridLayout>
#include <QColorDialog>
#include <QColor>
#include <QMap>
#include <QPushButton>
#include <QList>
#include <QLineEdit>

// Plain QDialog (no Q_OBJECT) - result stored directly.
class ThemeDialog : public QDialog {
public:
    ThemeDialog(ThemeManager* theme, QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Оформление");
        setFixedSize(480, 460);
        setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
        m_pickedTheme = theme->currentName();
        m_pickedAcc   = theme->accent();

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(24, 20, 24, 20);
        root->setSpacing(16);

        auto* title = new QLabel("Оформление браузера");
        title->setFont(QFont("Segoe UI", 15, QFont::Bold));
        root->addWidget(title);

        auto makeSep = [&]() -> QFrame* {
            auto* f = new QFrame;
            f->setFrameShape(QFrame::HLine);
            f->setStyleSheet("background:rgba(128,128,128,0.18);max-height:1px;");
            return f;
        };

        root->addWidget(makeSep());
        auto* bl = new QLabel("Базовая тема");
        bl->setFont(QFont("Segoe UI", 12, QFont::Medium));
        root->addWidget(bl);

        auto* baseRow = new QHBoxLayout;
        baseRow->setSpacing(8);
        const auto& P = ThemeManager::presets();
        for (const QString& name : QStringList{"light", "dark"}) {
            auto* b = new QPushButton(P[name].label);
            b->setObjectName("segBtn");
            b->setCheckable(true);
            b->setChecked(m_pickedTheme == name);
            b->setMinimumHeight(34);
            m_baseButtons[name] = b;
            connect(b, &QPushButton::clicked, this, [this, name]{
                m_pickedTheme = name;
                for (auto it = m_baseButtons.begin(); it != m_baseButtons.end(); ++it)
                    it.value()->setChecked(it.key() == name);
            });
            baseRow->addWidget(b);
        }
        root->addLayout(baseRow);

        root->addWidget(makeSep());
        auto* al = new QLabel("Цвет акцента");
        al->setFont(QFont("Segoe UI", 12, QFont::Medium));
        root->addWidget(al);

        const QList<QPair<QString,QString>> accents = {
            {"#0a7aff","Синий"}, {"#7f2eff","Фиолетовый"}, {"#28c840","Зелёный"},
            {"#ff3b30","Красный"}, {"#ff9500","Оранжевый"}, {"#ff375f","Розовый"},
            {"#32ade6","Голубой"}, {"#ff6b35","Коралловый"},
        };

        auto* grid = new QGridLayout;
        grid->setSpacing(8);
        for (int i = 0; i < accents.size(); ++i) {
            const QString hex   = accents[i].first;
            const QString label = accents[i].second;
            auto* btn = new QPushButton;
            btn->setObjectName("swatchBtn");
            btn->setCheckable(true);
            btn->setChecked(hex.toLower() == m_pickedAcc.toLower());
            btn->setToolTip(label);
            btn->setStyleSheet(QString(
                "QPushButton#swatchBtn{background:%1;border:2px solid transparent;"
                "border-radius:10px;min-width:36px;max-width:36px;"
                "min-height:36px;max-height:36px;}"
                "QPushButton#swatchBtn:checked{border-color:%1;}").arg(hex));
            m_swatchBtns[hex] = btn;
            connect(btn, &QPushButton::clicked, this, [this, hex]{
                m_pickedAcc = hex;
                for (auto it = m_swatchBtns.begin(); it != m_swatchBtns.end(); ++it)
                    it.value()->setChecked(it.key().toLower() == hex.toLower());
            });
            grid->addWidget(btn, i/4, i%4);
        }
        root->addLayout(grid);

        auto* customClr = new QPushButton("🎨  Выбрать свой цвет…");
        customClr->setObjectName("segBtn");
        customClr->setMinimumHeight(32);
        connect(customClr, &QPushButton::clicked, this, [this]{
            QColor c = QColorDialog::getColor(QColor(m_pickedAcc), this, "Выбери цвет акцента");
            if (c.isValid()) {
                m_pickedAcc = c.name();
                for (auto* b : m_swatchBtns) b->setChecked(false);
            }
        });
        root->addWidget(customClr);
        root->addStretch();

        auto* btnRow = new QHBoxLayout;
        btnRow->setSpacing(8);
        auto* cancelBtn = new QPushButton("Отмена");
        cancelBtn->setObjectName("segBtn");
        cancelBtn->setMinimumHeight(36);
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
        auto* applyBtn = new QPushButton("Применить");
        applyBtn->setObjectName("actionBtn");
        applyBtn->setMinimumHeight(36);
        connect(applyBtn, &QPushButton::clicked, this, &QDialog::accept);
        btnRow->addStretch();
        btnRow->addWidget(cancelBtn);
        btnRow->addWidget(applyBtn);
        root->addLayout(btnRow);
    }

    QString pickedTheme() const { return m_pickedTheme; }
    QString pickedAccent() const { return m_pickedAcc; }

private:
    QString m_pickedTheme;
    QString m_pickedAcc;
    QMap<QString, QPushButton*> m_baseButtons;
    QMap<QString, QPushButton*> m_swatchBtns;
};

// ── SettingsPanel ──────────────────────────────────────────────────────────────
static QFrame* makeSettingSep() {
    auto* f = new QFrame;
    f->setFrameShape(QFrame::HLine);
    f->setStyleSheet("background:rgba(128,128,128,0.14);max-height:1px;");
    return f;
}

SettingsPanel::SettingsPanel(ThemeManager* theme, Settings* settings, QWidget* parent)
    : QWidget(parent), m_theme(theme), m_settings(settings)
{
    setObjectName("rightPanel");
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(18, 16, 18, 16);
    lay->setSpacing(14);

    auto* t = new QLabel("Настройки");
    t->setObjectName("panelHead");
    t->setFont(QFont("Segoe UI", 15, QFont::Bold));
    lay->addWidget(t);
    lay->addWidget(makeSettingSep());

    auto* nl = new QLabel("Ваше имя");
    nl->setFont(QFont("Segoe UI", 12, QFont::Medium));
    lay->addWidget(nl);

    m_nameEdit = new QLineEdit(settings->name());
    m_nameEdit->setObjectName("settingsInput");
    m_nameEdit->setPlaceholderText("Введите имя…");
    lay->addWidget(m_nameEdit);

    auto* hint = new QLabel("Отображается в приветствии на новой вкладке");
    hint->setObjectName("hint");
    hint->setWordWrap(true);
    lay->addWidget(hint);

    lay->addWidget(makeSettingSep());

    auto* sl = new QLabel("Поиск по умолчанию");
    sl->setFont(QFont("Segoe UI", 12, QFont::Medium));
    lay->addWidget(sl);

    auto* sr = new QHBoxLayout;
    sr->setSpacing(6);
    const QList<QPair<QString,QString>> engines = {
        {"google","Google"}, {"bing","Bing"}, {"ddg","DDG"}
    };
    for (const auto& pair : engines) {
        const QString k = pair.first;
        const QString lb = pair.second;
        auto* b = new QPushButton(lb);
        b->setObjectName("segBtn");
        b->setCheckable(true);
        b->setChecked(settings->search() == k);
        connect(b, &QPushButton::clicked, this, [this, k]{ setSearch(k); });
        m_searchBtns[k] = b;
        sr->addWidget(b);
    }
    lay->addLayout(sr);

    lay->addWidget(makeSettingSep());

    auto* tl = new QLabel("Оформление");
    tl->setFont(QFont("Segoe UI", 12, QFont::Medium));
    lay->addWidget(tl);

    auto* themeBtn = new QPushButton("🎨  Изменить тему…");
    themeBtn->setObjectName("segBtn");
    themeBtn->setMinimumHeight(34);
    connect(themeBtn, &QPushButton::clicked, this, &SettingsPanel::pickTheme);
    lay->addWidget(themeBtn);

    m_themeLabel = new QLabel;
    m_themeLabel->setObjectName("hint");
    m_themeLabel->setWordWrap(true);
    lay->addWidget(m_themeLabel);
    refreshThemeLabel();

    lay->addStretch();

    auto* sv = new QPushButton("Сохранить");
    sv->setObjectName("actionBtn");
    sv->setFont(QFont("Segoe UI", 13, QFont::Medium));
    connect(sv, &QPushButton::clicked, this, &SettingsPanel::save);
    lay->addWidget(sv);
}

void SettingsPanel::pickTheme() {
    ThemeDialog dlg(m_theme, this);
    if (dlg.exec() == QDialog::Accepted)
        applyTheme(dlg.pickedTheme(), dlg.pickedAccent());
}

void SettingsPanel::applyTheme(const QString& name, const QString& accent) {
    m_theme->apply(name, accent);
    refreshThemeLabel();
    emit themeChanged();
}

void SettingsPanel::setSearch(const QString& key) {
    m_settings->setSearch(key);
    m_settings->save();
    for (auto it = m_searchBtns.begin(); it != m_searchBtns.end(); ++it)
        it.value()->setChecked(it.key() == key);
}

void SettingsPanel::save() {
    m_settings->setName(m_nameEdit->text().trimmed());
    m_settings->save();
}

void SettingsPanel::refreshThemeLabel() {
    const auto& P   = ThemeManager::presets();
    const QString name  = m_theme->currentName();
    const QString label = P.contains(name) ? P[name].label : "Своя";
    m_themeLabel->setText(QString("Текущая: %1  (%2)").arg(label, m_theme->accent()));
}
