#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QtNetwork>

class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(QObject *parent = nullptr);

    void check();

    void openDialog();

    double getLatestVersionNum() const { return latestVersionNum; }

    static bool isVersionConsideredUpdate(double v);

signals:
    void checkedUpdates();

protected:
    void readReply(QNetworkReply *reply);

    bool showSystemNotification();

    static double parseVersion(QString str);

private:
    const QString API_BASE_URL = "https://api.github.com/repos/jdpurcell/qView/releases";
    const QString DOWNLOAD_URL = "https://github.com/jdpurcell/qView/releases";

    double latestVersionNum;

    QString changelog;

    QDateTime releaseDate;

    QNetworkAccessManager netAccessManager;
};

#endif // UPDATECHECKER_H
