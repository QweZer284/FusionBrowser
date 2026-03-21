#pragma once
#include <QObject>
#include <QStringList>
#include <functional>

class Database;

// Available browser keys
QStringList detectInstalledBrowsers();
QString     browserDisplayName(const QString& key);

// Run import — blocks, call from worker thread.
// progress_cb(percent, message)
void runImport(const QStringList& keys,
               Database* db,
               std::function<void(int, const QString&)> progressCb = {});
