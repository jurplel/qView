#ifndef QVIPCSERVER_H
#define QVIPCSERVER_H

#include <QJsonObject>
#include <QLocalServer>
#include <QObject>
#include <QStringList>

class QVApplication;
class QLocalSocket;

class QVIPCServer : public QObject
{
    Q_OBJECT

public:
    struct ServerOption
    {
        bool enabled = false;
        QString serverName;
    };

    explicit QVIPCServer(QVApplication *app);

    bool listen(const QString &serverName);
    QString serverName() const;

    static QString defaultServerName();
    static ServerOption extractServerOption(QStringList *arguments);
    static QJsonObject handleMessage(const QJsonObject &message, const QString &currentFilePath);

private slots:
    void acceptConnections();

private:
    void handleSocketData(QLocalSocket *socket);
    void sendResponse(QLocalSocket *socket, const QJsonObject &response);

    QVApplication *application;
    QLocalServer localServer;
};

#endif // QVIPCSERVER_H
