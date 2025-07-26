#include "settingsmanager.h"

#include <QSettings>
#include <QTranslator>
#include <QLocale>
#include <QCoreApplication>
#include <QDir>
#include <QMetaEnum>

#include <QDebug>

// Setting definition structure for initialization
struct SettingDefinition {
    SettingsManager::Setting setting;
    QVariant defaultValue;
    const char* key;
};

// Master list of all settings with their data and keys
static const SettingDefinition settingDefinitions[] = {
    // Window settings
    {SettingsManager::Setting::BgColorEnabled, true, "bgcolorenabled"},
    {SettingsManager::Setting::BgColor, "#212121", "bgcolor"},
    {SettingsManager::Setting::TitleBarMode, 1, "titlebarmode"},
    {SettingsManager::Setting::WindowResizeMode, 1, "windowresizemode"},
    {SettingsManager::Setting::MinWindowResizedPercentage, 20, "minwindowresizedpercentage"},
    {SettingsManager::Setting::MaxWindowResizedPercentage, 70, "maxwindowresizedpercentage"},
    {SettingsManager::Setting::TitleBarAlwaysDark, true, "titlebaralwaysdark"},
    {SettingsManager::Setting::QuitOnLastWindow, false, "quitonlastwindow"},
    {SettingsManager::Setting::MenuBarEnabled, false, "menubarenabled"},
    {SettingsManager::Setting::FullScreenDetails, false, "fullscreendetails"},
    // Image settings
    {SettingsManager::Setting::FilteringEnabled, true, "filteringenabled"},
    {SettingsManager::Setting::ScalingEnabled, true, "scalingenabled"},
    {SettingsManager::Setting::ScalingTwoEnabled, true, "scalingtwoenabled"},
    {SettingsManager::Setting::ScaleFactor, 25, "scalefactor"},
    {SettingsManager::Setting::ScrollZoom, 1, "scrollzoom"},
    {SettingsManager::Setting::FractionalZoom, false, "fractionalzoom"},
    {SettingsManager::Setting::CursorZoom, true, "cursorzoom"},
    {SettingsManager::Setting::CropMode, 0, "cropmode"},
    {SettingsManager::Setting::PastActualSizeEnabled, true, "pastactualsizeenabled"},
    {SettingsManager::Setting::ColorSpaceConversion, 1, "colorspaceconversion"},
    // Miscellaneous settings
    {SettingsManager::Setting::Language, "system", "language"},
    {SettingsManager::Setting::SortMode, 0, "sortmode"},
    {SettingsManager::Setting::SortDescending, false, "sortdescending"},
    {SettingsManager::Setting::PreloadingMode, 1, "preloadingmode"},
    {SettingsManager::Setting::LoopFoldersEnabled, true, "loopfoldersenabled"},
    {SettingsManager::Setting::SlideshowReversed, false, "slideshowreversed"},
    {SettingsManager::Setting::SlideshowTimer, 5, "slideshowtimer"},
    {SettingsManager::Setting::AfterDelete, 2, "afterdelete"},
    {SettingsManager::Setting::AskDelete, true, "askdelete"},
    {SettingsManager::Setting::AllowMimeContentDetection, false, "allowmimecontentdetection"},
    {SettingsManager::Setting::SaveRecents, true, "saverecents"},
    {SettingsManager::Setting::UpdateNotifications, false, "updatenotifications"},
    {SettingsManager::Setting::SkipHidden, true, "skiphidden"}
};

// settingKeys is a file-static variable, it doesn't need to be a member
static QVector<QString> settingKeys;

SettingsManager::SettingsManager(QObject *parent) : QObject(parent)
{
    initializeSettingDataCache();
    loadSettings();
    loadTranslation();
}

QString SettingsManager::getSystemLanguage() const
{
    auto entries = QDir(":/i18n/").entryList();
    entries.prepend("qview_en.ts");
    const auto centries = entries;

    const auto languages = QLocale::system().uiLanguages();
    for (auto language : languages)
    {
        language.replace('-', '_');
        const auto countryless = language.left(2);

        for (auto entry : centries)
        {
            entry.remove(0, 6);
            entry.remove(entry.length()-3, 3);

            if (entry == language)
                return language;

            if (entry == countryless)
                return countryless;
        }
    }
    return "en";
}

bool SettingsManager::loadTranslation() const
{
    QString lang = getString("language");
    if (lang == "system")
        lang = getSystemLanguage();

    if (lang == "en")
        return true;

    QTranslator *translator = new QTranslator();
    bool success = translator->load("qview_" + lang + ".qm", QLatin1String(":/i18n"));
    if (success)
    {
        qInfo() << "Loaded translation" << lang;
        QCoreApplication::installTranslator(translator);
    }
    return success;
}

void SettingsManager::loadSettings()
{
    QSettings settings;
    settings.beginGroup("options");
    bool changed = false;

    // Get the meta enum for Setting
    QMetaEnum metaEnum = QMetaEnum::fromType<Setting>();
    Q_ASSERT(metaEnum.keyCount() == settingDataCache.size());

    // Loop through all enum values and load from file
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        Setting setting = static_cast<Setting>(metaEnum.value(i));
        SettingData &data = settingDataCache[i];
        const QString &key = getSettingKey(setting);

        QVariant newValue = settings.value(key, data.defaultValue);
        if (data.value != newValue) {
            changed = true;
            data.value = newValue;
        }
    }

    if (changed)
        emit settingsUpdated();
}

// Legacy string-based API - find setting by key and delegate to enum-based method
const QVariant SettingsManager::getSetting(const QString &key, bool defaults) const
{
    // Find the setting enum that matches this key
    for (const auto& def : settingDefinitions) {
        if (QString(def.key) == key) {
            return getSetting(def.setting, defaults);
        }
    }
    
    qWarning() << "Error: Invalid settings key: " + key;
    return QVariant();
}

bool SettingsManager::getBool(const QString &key, bool defaults) const
{
    auto value = getSetting(key, defaults);
    if (value.canConvert<bool>())
        return value.toBool();
    return false;
}

int SettingsManager::getInt(const QString &key, bool defaults) const
{
    auto value = getSetting(key, defaults);
    if (value.canConvert<int>())
        return value.toInt();
    return 0;
}

double SettingsManager::getDouble(const QString &key, bool defaults) const
{
    auto value = getSetting(key, defaults);
    if (value.canConvert<double>())
        return value.toDouble();
    return 0.0;
}

const QString SettingsManager::getString(const QString &key, bool defaults) const
{
    auto value = getSetting(key, defaults);
    if (value.canConvert<QString>())
        return value.toString();
    return "";
}

bool SettingsManager::isDefault(const QString &key) const
{
    // Find the setting enum that matches this key
    for (const auto& def : settingDefinitions) {
        if (QString(def.key) == key) {
            return isDefault(def.setting);
        }
    }
    return true; // Unknown keys are considered "default"
}

// Enum-based access methods
const SettingsManager::SettingData &SettingsManager::getSettingData(Setting setting) const
{
    const int index = static_cast<int>(setting);
    Q_ASSERT(index >= 0 && index < settingDataCache.size());
    return settingDataCache[index];
}

const QString &SettingsManager::getSettingKey(Setting setting)
{
    static const QString empty;
    int index = static_cast<int>(setting);
    if (index >= 0 && index < settingKeys.size()) {
        return settingKeys[index];
    }
    return empty;
}

const QVariant SettingsManager::getSetting(Setting setting, bool defaults) const
{
    const SettingData &data = getSettingData(setting);
    if (!defaults && !data.value.isNull())
        return data.value;
    return data.defaultValue;
}

bool SettingsManager::getBool(Setting setting, bool defaults) const
{
    auto value = getSetting(setting, defaults);
    if (value.canConvert<bool>())
        return value.toBool();
    return false;
}

int SettingsManager::getInt(Setting setting, bool defaults) const
{
    auto value = getSetting(setting, defaults);
    if (value.canConvert<int>())
        return value.toInt();
    return 0;
}

double SettingsManager::getDouble(Setting setting, bool defaults) const
{
    auto value = getSetting(setting, defaults);
    if (value.canConvert<double>())
        return value.toDouble();
    return 0.0;
}

const QString SettingsManager::getString(Setting setting, bool defaults) const
{
    auto value = getSetting(setting, defaults);
    if (value.canConvert<QString>())
        return value.toString();
    return "";
}

bool SettingsManager::isDefault(Setting setting) const
{
    const SettingData &data = getSettingData(setting);
    return data.value.isNull() || data.value == data.defaultValue;
}

void SettingsManager::initializeSettingDataCache() {
    QMetaEnum metaEnum = QMetaEnum::fromType<SettingsManager::Setting>();
    int enumCount = metaEnum.keyCount();

    settingDataCache.resize(enumCount);
    settingKeys.resize(enumCount);

    // Fill vectors based on definitions
    for (const auto& def : settingDefinitions) {
        int index = static_cast<int>(def.setting);
        settingDataCache[index] = {def.defaultValue, {}};
        settingKeys[index] = def.key;
    }
}
