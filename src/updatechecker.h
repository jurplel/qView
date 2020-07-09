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

signals:
    void checkedUpdates();

protected:
    void sendRequest(const QUrl &url);

    void readReply(QNetworkReply *reply);

    bool showSystemNotification();

private:
    const QString UPDATE_URL = "https://api.github.com/repos/jurplel/qview/releases";
    const QString DOWNLOAD_URL = "https://interversehq.com/qview/download/";

    double latestVersionNum;

    QString changelog;

    QDateTime releaseDate;
};

#endif // UPDATECHECKER_H
