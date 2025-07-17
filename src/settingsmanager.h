#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QVariant>

class SettingsManager : public QObject
{
    Q_OBJECT
public:
    enum class Setting
    {
        // Window settings
        BgColorEnabled = 0,
        BgColor,
        TitleBarMode,
        WindowResizeMode,
        MinWindowResizedPercentage,
        MaxWindowResizedPercentage,
        TitleBarAlwaysDark,
        QuitOnLastWindow,
        MenuBarEnabled,
        FullScreenDetails,
        // Image settings
        FilteringEnabled,
        ScalingEnabled,
        ScalingTwoEnabled,
        ScaleFactor,
        ScrollZoom,
        FractionalZoom,
        CursorZoom,
        CropMode,
        PastActualSizeEnabled,
        ColorSpaceConversion,
        // Miscellaneous settings
        Language,
        SortMode,
        SortDescending,
        PreloadingMode,
        LoopFoldersEnabled,
        SlideshowReversed,
        SlideshowTimer,
        AfterDelete,
        AskDelete,
        AllowMimeContentDetection,
        SaveRecents,
        UpdateNotifications,
        SkipHidden
    };
    Q_ENUM(Setting)

    struct SettingData {
        QVariant defaultValue;
        QVariant value;
    };

    explicit SettingsManager(QObject *parent = nullptr);

    QString getSystemLanguage() const;

    bool loadTranslation() const;

    void loadSettings();

    // String-based access (legacy)
    const QVariant getSetting(const QString &key, bool defaults = false) const;
    bool getBool(const QString &key, bool defaults = false) const;
    int getInt(const QString &key, bool defaults = false) const;
    double getDouble(const QString &key, bool defaults = false) const;
    const QString getString(const QString &key, bool defaults = false) const;
    bool isDefault(const QString &key) const;

    // Enum-based access (optimized)
    const QVariant getSetting(Setting setting, bool defaults = false) const;
    bool getBool(Setting setting, bool defaults = false) const;
    int getInt(Setting setting, bool defaults = false) const;
    double getDouble(Setting setting, bool defaults = false) const;
    const QString getString(Setting setting, bool defaults = false) const;
    bool isDefault(Setting setting) const;

    // Direct enum-based access to setting data
    const SettingData &getSettingData(Setting setting) const;

signals:
    void settingsUpdated();

private:
    void initializeSettingDataCache();
    QVector<SettingData> settingDataCache;
    static const QString &getSettingKey(Setting setting);

};

#endif // SETTINGSMANAGER_H
