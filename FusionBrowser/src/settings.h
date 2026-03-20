#pragma once
#include <QObject>
#include <QString>
#include <QVariant>
#include <QJsonObject>

// Settings — persisted to ~/.fusion9.json
class Settings : public QObject {
    Q_OBJECT
public:
    explicit Settings(QObject* parent = nullptr);

    QString  name()         const { return m_obj.value("name").toString(); }
    bool     dark()         const { return m_obj.value("dark").toBool(false); }
    QString  theme()        const { return m_obj.value("theme").toString("light"); }
    QString  customAccent() const { return m_obj.value("custom_accent").toString("#0a7aff"); }
    QString  search()       const { return m_obj.value("search").toString("google"); }

    // Window geometry
    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray& data);

    void setName(const QString& v)        { m_obj["name"] = v; }
    void setDark(bool v)                  { m_obj["dark"] = v; }
    void setTheme(const QString& v)       { m_obj["theme"] = v; }
    void setCustomAccent(const QString& v){ m_obj["custom_accent"] = v; }
    void setSearch(const QString& v)      { m_obj["search"] = v; }

    void save();
    void load();

    // Access raw json for themes
    QJsonObject& json() { return m_obj; }

private:
    QString     m_path;
    QJsonObject m_obj;
};
