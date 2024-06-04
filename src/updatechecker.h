#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QtNetwork>

class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(QObject *parent = nullptr);

    void check(bool isStartupCheck);

    void openDialog();

    double getLatestVersionNum() const { return latestVersionNum; }

signals:
    void checkedUpdates();

protected:
    void sendRequest(const QUrl &url);

    void readReply(QNetworkReply *reply);

    QDateTime getLastCheckTime() const;

    void setLastCheckTime(QDateTime value);

private:
    const QString UPDATE_URL = "https://api.github.com/repos/jurplel/qview/releases";
    const QString DOWNLOAD_URL = "https://interversehq.com/qview/download/";
    // If update checking is enabled, this rate limits the auto-check that happens at startup
    const int STARTUP_CHECK_INTERVAL_HOURS = 4;

    double latestVersionNum;

    QString changelog;

    QDateTime releaseDate;

    QNetworkAccessManager netAccessManager;
};

#endif // UPDATECHECKER_H
