#include "settingsmanager.h"

#include <QSettings>
#include <QTranslator>
#include <QLocale>
#include <QCoreApplication>

#include <QDebug>

SettingsManager::SettingsManager(QObject *parent) : QObject(parent)
{
    initializeSettingsLibrary();
    loadTranslation();
    loadSettings();
}

bool SettingsManager::loadTranslation()
{
    QTranslator *translator = new QTranslator();
    bool success = translator->load(QLocale::system(), QLatin1String("qview"), QLatin1String("_"), QLatin1String(":/i18n"));
    if (success)
    {
        qInfo() << "Loaded translation";
        QCoreApplication::installTranslator(translator);
    }
    return success;
}

void SettingsManager::loadSettings()
{
    QSettings settings;
    settings.beginGroup("options");
    bool changed = false;

    const auto keys = settingsLibrary.keys();
    for (const auto &key : keys)
    {
         auto &setting = settingsLibrary[key];
         if (setting.value != settings.value(key, setting.defaultValue))
             changed = true;

         setting.value = settings.value(key, setting.defaultValue);
    }

    if (changed)
        emit settingsUpdated();
}

const QVariant SettingsManager::getSetting(const QString &key, bool defaults) const
{
    auto value = settingsLibrary.value(key);

    if (!defaults && !value.value.isNull())
        return value.value;

    if (!value.defaultValue.isNull())
        return value.defaultValue;

    qWarning() << "Error: Invalid settings key: " + key;
    return QVariant();
}

bool SettingsManager::getBoolean(const QString &key, bool defaults) const
{
    auto value = getSetting(key, defaults);

    if (value.canConvert(QMetaType::Bool))
        return value.toBool();

    qWarning() << "Error: Can't convert setting key " + key + " to bool";
    return false;
}

int SettingsManager::getInteger(const QString &key, bool defaults) const
{
    auto value = getSetting(key, defaults);

    if (value.canConvert(QMetaType::Int))
        return value.toInt();

    qWarning() << "Error: Can't convert setting key " + key + " to int";
    return 0;
}

double SettingsManager::getDouble(const QString &key, bool defaults) const
{
    auto value = getSetting(key, defaults);

    if (value.canConvert(QMetaType::Double))
        return value.toDouble();

    qWarning() << "Error: Can't convert setting key " + key + " to double";
    return 0;
}

const QString SettingsManager::getString(const QString &key, bool defaults) const
{
    auto value = getSetting(key, defaults);

    if (value.canConvert(QMetaType::QString))
        return value.toString();

    qWarning() << "Error: Can't convert setting key " + key + " to string";
    return "";
}

void SettingsManager::initializeSettingsLibrary()
{
    // Window
    settingsLibrary.insert("bgcolorenabled", {true, {}});
    settingsLibrary.insert("bgcolor", {"#212121", {}});
    settingsLibrary.insert("titlebarmode", {1, {}});
    settingsLibrary.insert("windowresizemode", {1, {}});
    settingsLibrary.insert("minwindowresizedpercentage", {20, {}});
    settingsLibrary.insert("maxwindowresizedpercentage", {70, {}});
    settingsLibrary.insert("titlebaralwaysdark", {true, {}});
    settingsLibrary.insert("menubarenabled", {false, {}});
    settingsLibrary.insert("fullscreendetails", {false, {}});
    // Image
    settingsLibrary.insert("filteringenabled", {true, {}});
    settingsLibrary.insert("scalingenabled", {true, {}});
    settingsLibrary.insert("scalingtwoenabled", {true, {}});
    settingsLibrary.insert("scalefactor", {25, {}});
    settingsLibrary.insert("scrollzoomsenabled", {true, {}});
    settingsLibrary.insert("cursorzoom", {true, {}});
    settingsLibrary.insert("cropmode", {0, {}});
    settingsLibrary.insert("pastactualsizeenabled", {true, {}});
    // Miscellaneous
    settingsLibrary.insert("sortmode", {0, {}});
    settingsLibrary.insert("sortdescending", {false, {}});
    settingsLibrary.insert("preloadingmode", {1, {}});
    settingsLibrary.insert("loopfoldersenabled", {true, {}});
    settingsLibrary.insert("slideshowreversed", {false, {}});
    settingsLibrary.insert("slideshowtimer", {5, {}});
    settingsLibrary.insert("saverecents", {true, {}});
    settingsLibrary.insert("updatenotifications", {false, {}});
}
