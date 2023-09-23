#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QtNetwork>

class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(QObject *parent = nullptr);

    struct CheckResult
    {
        bool wasSuccessful;
        QString errorMessage;
        double latestVersionNum;
        QString releaseName;
        QString changelog;

        bool isConsideredUpdate() const { return isVersionConsideredUpdate(latestVersionNum); }
    };

    void check(bool isManualCheck = false);

    void openDialog(QWidget *parent, bool showDisableButton);

    bool getIsChecking() const { return isChecking; }

    bool getHasChecked() const { return hasChecked; }

    CheckResult getCheckResult() const { return checkResult; }

signals:
    void checkedUpdates();

protected:
    void readReply(QNetworkReply *reply);

    void onError(QString msg);

    QDateTime getLastCheckTime() const;

    void setLastCheckTime(QDateTime value);

    static double parseVersion(QString str);

    static bool isVersionConsideredUpdate(double v);

private:
    const QString API_BASE_URL = "https://api.github.com/repos/jdpurcell/qView/releases";
    const QString DOWNLOAD_URL = "https://github.com/jdpurcell/qView/releases";
    // Auto-check happens only at startup (if enabled); this is to rate limit across launches
    const int AUTO_CHECK_INTERVAL_HOURS = 4;

    bool isChecking;
    bool hasChecked;
    CheckResult checkResult;

    QNetworkAccessManager netAccessManager;
};

#endif // UPDATECHECKER_H
