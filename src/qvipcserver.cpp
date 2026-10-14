#include "qvipcserver.h"
#include "qvapplication.h"

#include <QDir>
#include <QJsonDocument>
#include <QLocalSocket>

#ifdef Q_OS_UNIX
#  include <unistd.h>
#endif

namespace {
const char *bufferPropertyName = "qviewIpcBuffer";

QString fallbackUserId()
{
#ifdef Q_OS_UNIX
    return QString::number(getuid());
#else
    const QString username = qEnvironmentVariable("USERNAME");
    return username.isEmpty() ? QStringLiteral("user") : username;
#endif
}
}

QVIPCServer::QVIPCServer(QVApplication *app) : QObject(app), application(app)
{
    connect(&localServer, &QLocalServer::newConnection, this, &QVIPCServer::acceptConnections);
}

bool QVIPCServer::listen(const QString &serverName)
{
    const QString name = serverName.isEmpty() ? defaultServerName() : serverName;
    if (localServer.listen(name))
        return true;

    if (localServer.serverError() != QAbstractSocket::AddressInUseError)
        return false;

    QLocalSocket probe;
    probe.connectToServer(name);
    if (probe.waitForConnected(100)) {
        probe.disconnectFromServer();
        return false;
    }

    QLocalServer::removeServer(name);
    return localServer.listen(name);
}

QString QVIPCServer::serverName() const
{
    return localServer.serverName();
}

QString QVIPCServer::defaultServerName()
{
#ifdef Q_OS_UNIX
    QString socketDir = qEnvironmentVariable("TMPDIR");
    if (socketDir.isEmpty())
        socketDir = QDir::tempPath();
    if (socketDir.isEmpty())
        socketDir = QStringLiteral("/tmp");

    return QDir(socketDir).absoluteFilePath(QStringLiteral("qview-%1.sock").arg(fallbackUserId()));
#else
    return QStringLiteral("qview-%1").arg(fallbackUserId());
#endif
}

QVIPCServer::ServerOption QVIPCServer::extractServerOption(QStringList *arguments)
{
    ServerOption option;
    if (!arguments || arguments->isEmpty())
        return option;

    for (int i = 1; i < arguments->size(); ++i) {
        const QString argument = arguments->at(i);
        if (argument == QStringLiteral("--ipc-server")) {
            option.enabled = true;
            arguments->removeAt(i);

            if (i < arguments->size() && !arguments->at(i).startsWith(QLatin1Char('-'))) {
                option.serverName = arguments->takeAt(i);
                --i;
            } else {
                option.serverName = defaultServerName();
                --i;
            }
        } else if (argument.startsWith(QStringLiteral("--ipc-server="))) {
            option.enabled = true;
            option.serverName = argument.mid(QStringLiteral("--ipc-server=").size());
            if (option.serverName.isEmpty())
                option.serverName = defaultServerName();
            arguments->removeAt(i);
            --i;
        }
    }

    return option;
}

QJsonObject QVIPCServer::handleMessage(const QJsonObject &message, const QString &currentFilePath)
{
    const QString method = message.value(QStringLiteral("method")).toString();
    if (method == QStringLiteral("currentFilePath")) {
        if (currentFilePath.isEmpty()) {
            return { { QStringLiteral("ok"), false },
                     { QStringLiteral("error"), QStringLiteral("no_current_file") } };
        }

        return { { QStringLiteral("ok"), true }, { QStringLiteral("path"), currentFilePath } };
    }

    return { { QStringLiteral("ok"), false },
             { QStringLiteral("error"), QStringLiteral("unknown_method") } };
}

void QVIPCServer::acceptConnections()
{
    while (auto *socket = localServer.nextPendingConnection()) {
        socket->setParent(this);
        connect(socket, &QLocalSocket::readyRead, this,
                [this, socket]() { handleSocketData(socket); });
        connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
    }
}

void QVIPCServer::handleSocketData(QLocalSocket *socket)
{
    QByteArray buffer = socket->property(bufferPropertyName).toByteArray();
    buffer.append(socket->readAll());

    int newlineIndex = buffer.indexOf('\n');
    while (newlineIndex != -1) {
        const QByteArray line = buffer.left(newlineIndex).trimmed();
        buffer.remove(0, newlineIndex + 1);

        if (!line.isEmpty()) {
            QJsonParseError parseError;
            const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);
            if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                sendResponse(socket,
                             { { QStringLiteral("ok"), false },
                               { QStringLiteral("error"), QStringLiteral("invalid_json") } });
            } else {
                sendResponse(socket,
                             handleMessage(document.object(), application->currentFilePath()));
            }
        }

        newlineIndex = buffer.indexOf('\n');
    }

    socket->setProperty(bufferPropertyName, buffer);
}

void QVIPCServer::sendResponse(QLocalSocket *socket, const QJsonObject &response)
{
    socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact));
    socket->write("\n");
    socket->flush();
}
