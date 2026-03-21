#pragma once
#include <QString>

class Database;
class ThemeManager;
class Settings;

// Generates the new-tab homepage HTML string.
QString generateHomepageHtml(Database* db, ThemeManager* theme, Settings* settings);
