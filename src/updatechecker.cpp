#include "updatechecker.h"

#include <QMessageBox>
#include <QPushButton>
#include <QDateTime>

UpdateChecker::UpdateChecker(QObject *parent) : QObject(parent)
{
    latestVersionNum = 0.0;
}

void UpdateChecker::check()
{
    sendRequest(UPDATE_URL);
}

void UpdateChecker::sendRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    auto *netAccessManager = new QNetworkAccessManager;
    connect(netAccessManager, &QNetworkAccessManager::finished, this, &UpdateChecker::readReply);

    netAccessManager->get(request);
}

void UpdateChecker::readReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << "Error checking for updates: " + reply->errorString();
        return;
    }

    QByteArray byteArray = reply->readAll();
    QJsonDocument json = QJsonDocument::fromJson(byteArray);

    if (json.isNull())
    {
        qWarning() << "Error checking for updates: Received null JSON";
        return;
    }

    QJsonObject object = json.array().first().toObject();

    latestVersionNum = object.value("tag_name").toString("0.0").toDouble();

    QStringList changelogList = object.value("body").toString().split("\n");
    changelogList.removeFirst();
    changelog = changelogList.join("");

    releaseDate = QDateTime::fromString(object.value("published_at").toString(), Qt::ISODate);
    releaseDate = releaseDate.toTimeSpec(Qt::LocalTime);

    //if (latestVersionNum > VERSION)
        openDialog();
}

void UpdateChecker::openDialog()
{
    QLocale locale;
    auto *downloadButton = new QPushButton(QIcon::fromTheme("download"), tr("Download"));

    auto *msgBox = new QMessageBox();
    msgBox->setWindowTitle(tr("qView Update Available"));
    msgBox->setText("qView " + QString::number(latestVersionNum, 'f', 1) + tr(" is available to download.")
                    + "\n\n" + releaseDate.toString(locale.dateFormat()) + "\n" + changelog);
    msgBox->setWindowModality(Qt::ApplicationModal);
    msgBox->setStandardButtons(QMessageBox::Close | QMessageBox::Reset);
    msgBox->button(QMessageBox::Reset)->setText(tr("Disable Update Checking"));
    msgBox->addButton(downloadButton, QMessageBox::ActionRole);
    msgBox->open();
}
