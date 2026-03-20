#include "homepage.h"
#include "database.h"
#include "themes.h"
#include "settings.h"
#include <QDateTime>
#include <QUrl>
#include <QRegularExpression>

static QString cardHtml(const QString& url, const QString& title, const QString& fav) {
    static const QStringList palette = {
        "#0a7aff","#ff6b35","#30d158","#ff3b30",
        "#ff9f0a","#5e5ce6","#32ade6","#ff375f"
    };

    QString lbl = title.length() > 18
        ? title.left(16) + "…"
        : (title.isEmpty() ? url : title);

    QString img;
    if (!fav.isEmpty() && fav.startsWith("data:")) {
        img = QString(R"(<img src="%1" style="width:42px;height:42px;border-radius:10px;object-fit:contain;">)").arg(fav);
    } else {
        QUrl qu(url);
        QString domain = qu.host().remove(QRegularExpression("^www\\."));
        QChar letter = domain.isEmpty() ? QChar('?') : domain.at(0).toUpper();
        int idx = qAbs(qHash(domain)) % palette.size();
        QString col = palette[idx];
        img = QString(
            R"(<div style="width:42px;height:42px;border-radius:10px;background:%1;display:flex;align-items:center;justify-content:center;color:white;font-size:21px;font-weight:600;">%2</div>)")
            .arg(col).arg(letter);
    }

    return QString(R"(<a href="%1" class="card"><div class="iw">%2</div><span class="lb">%3</span></a>)")
        .arg(url.toHtmlEscaped()).arg(img).arg(lbl.toHtmlEscaped());
}

QString generateHomepageHtml(Database* db, ThemeManager* theme, Settings* settings) {
    const int hour = QTime::currentTime().hour();
    QString word;
    if      (hour >= 5  && hour < 12) word = "Доброе утро";
    else if (hour >= 12 && hour < 17) word = "Добрый день";
    else if (hour >= 17 && hour < 22) word = "Добрый вечер";
    else                              word = "Доброй ночи";

    const QString name = settings->name().trimmed();
    const QString greeting = name.isEmpty() ? word : word + ", " + name;

    auto pal = theme->homepagePalette();

    // Bookmarks
    QString bmHtml;
    const auto bms = db->getBookmarks();
    if (!bms.isEmpty()) {
        for (const auto& r : bms)
            bmHtml += cardHtml(r.url, r.title, r.favicon);
    } else {
        bmHtml = R"(<p class="em">Нажми ★ чтобы добавить сайт в Favorites</p>)";
    }

    // Frequent
    QString frqHtml;
    const auto frq = db->getFrequent(12);
    if (!frq.isEmpty()) {
        for (const auto& r : frq)
            frqHtml += cardHtml(r.url, r.title, r.favicon);
    } else {
        frqHtml = R"(<p class="em">Посети несколько сайтов — они появятся здесь</p>)";
    }

    return QString(R"(<!DOCTYPE html>
<html lang="ru"><head>
<meta charset="utf-8">
<title>Новая вкладка</title>
<style>
*, *::before, *::after { margin:0; padding:0; box-sizing:border-box; }
body {
    font-family: -apple-system, 'Segoe UI', sans-serif;
    background: %1;
    min-height: 100vh;
    padding: 52px 9vw 64px;
    overflow-y: auto;
    overflow-x: hidden;
}
.gr {
    font-size: 26px; font-weight: 300;
    color: %2; margin-bottom: 36px;
    letter-spacing: -0.4px;
}
.sec { margin-bottom: 38px; }
h2 {
    font-size: 18px; font-weight: 600;
    color: %3; margin-bottom: 14px;
    letter-spacing: -0.2px;
}
.grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, 92px);
    gap: 4px;
}
.card {
    display: flex; flex-direction: column; align-items: center;
    text-decoration: none; color: %4;
    width: 92px; padding: 12px 6px 10px;
    border-radius: 14px; gap: 8px;
    transition: background 0.15s, opacity 0.15s;
    will-change: opacity;
}
.card:hover { background: %5; }
.iw {
    width: 64px; height: 64px;
    background: %6; border-radius: 16px;
    display: flex; align-items: center; justify-content: center;
    box-shadow: %7; overflow: hidden; flex-shrink: 0;
}
.lb {
    font-size: 11px; text-align: center; line-height: 1.25;
    max-width: 86px; overflow: hidden;
    white-space: nowrap; text-overflow: ellipsis;
}
.em { color: %8; font-size: 12px; padding: 4px 0; }
</style>
</head><body>
<div class="gr">%9</div>
<div class="sec"><h2>Favorites</h2><div class="grid">%10</div></div>
<div class="sec"><h2>Frequently Visited</h2><div class="grid">%11</div></div>
</body></html>)")
        .arg(pal.bg)
        .arg(pal.gr)
        .arg(pal.h2)
        .arg(pal.cardC)
        .arg(pal.cHov)
        .arg(pal.iBg)
        .arg(pal.iSh)
        .arg(pal.em)
        .arg(greeting.toHtmlEscaped())
        .arg(bmHtml)
        .arg(frqHtml);
}
