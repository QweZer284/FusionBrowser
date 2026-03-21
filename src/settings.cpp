#include "settings.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QByteArray>
#include <QDebug>

Settings::Settings(QObject* parent) : QObject(parent) {
    m_path = QDir::homePath() + "/.fusion9.json";
    load();
}

void Settings::load() {
    QFile f(m_path);
    if (f.open(QIODevice::ReadOnly)) {
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(f.readAll(), &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            m_obj = doc.object();
            return;
        }
    }
    // Defaults
    m_obj = QJsonObject{
        {"name",          ""},
        {"dark",          false},
        {"theme",         "light"},
        {"custom_accent", "#0a7aff"},
        {"search",        "google"},
    };
}

void Settings::save() {
    QFile f(m_path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Settings::save failed to open" << m_path;
        return;
    }
    f.write(QJsonDocument(m_obj).toJson(QJsonDocument::Indented));
}

QByteArray Settings::windowGeometry() const {
    auto v = m_obj.value("_win_geometry").toString();
    return QByteArray::fromBase64(v.toLatin1());
}

void Settings::setWindowGeometry(const QByteArray& data) {
    m_obj["_win_geometry"] = QString::fromLatin1(data.toBase64());
}
