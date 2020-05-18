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

signals:

protected:
    void sendRequest(const QUrl &url);

    void readReply(QNetworkReply *reply);

    bool showSystemNotification();

private:
    const QString UPDATE_URL = "https://api.github.com/repos/jurplel/qview/releases";

    double latestVersionNum;

    QString changelog;

    QDateTime releaseDate;
};

#endif // UPDATECHECKER_H
