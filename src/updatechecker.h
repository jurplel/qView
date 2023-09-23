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

    void check();

    void openDialog(QWidget *parent, bool showDisableButton);

    bool getIsChecking() const { return isChecking; }

    bool getHasChecked() const { return hasChecked; }

    CheckResult getCheckResult() const { return checkResult; }

signals:
    void checkedUpdates();

protected:
    void readReply(QNetworkReply *reply);

    void onError(QString msg);

    static double parseVersion(QString str);

    static bool isVersionConsideredUpdate(double v);

private:
    const QString API_BASE_URL = "https://api.github.com/repos/jdpurcell/qView/releases";
    const QString DOWNLOAD_URL = "https://github.com/jdpurcell/qView/releases";

    bool isChecking;
    bool hasChecked;
    CheckResult checkResult;

    QNetworkAccessManager netAccessManager;
};

#endif // UPDATECHECKER_H
