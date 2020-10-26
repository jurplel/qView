#include "updatechecker.h"

#include "qvapplication.h"

#include <QMessageBox>
#include <QPushButton>
#include <QDateTime>
#include <QDesktopServices>

UpdateChecker::UpdateChecker(QObject *parent) : QObject(parent)
{
    latestVersionNum = -1.0;

    connect(&netAccessManager, &QNetworkAccessManager::finished, this, &UpdateChecker::readReply);
}

void UpdateChecker::check()
{
    sendRequest(UPDATE_URL);
}

void UpdateChecker::sendRequest(const QUrl &url)
{
    latestVersionNum = 0.0;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    netAccessManager.get(request);
}

void UpdateChecker::readReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << "Error checking for updates: " + reply->errorString();
        latestVersionNum = -1.0;
        emit checkedUpdates();
        return;
    }

    QByteArray byteArray = reply->readAll();
    QJsonDocument json = QJsonDocument::fromJson(byteArray);

    if (json.isNull())
    {
        qWarning() << "Error checking for updates: Received null JSON";
        latestVersionNum = -1.0;
        emit checkedUpdates();
        return;
    }

    QJsonObject object = json.array().first().toObject();

    latestVersionNum = object.value("tag_name").toString("0.0").toDouble();

    QStringList changelogList = object.value("body").toString().split("\n");
    changelogList.removeFirst();
    changelog = changelogList.join("");

    releaseDate = QDateTime::fromString(object.value("published_at").toString(), Qt::ISODate);
    releaseDate = releaseDate.toTimeSpec(Qt::LocalTime);

    emit checkedUpdates();
}

void UpdateChecker::openDialog()
{
    QLocale locale;
    auto *downloadButton = new QPushButton(QIcon::fromTheme("edit-download", QIcon::fromTheme("document-save")), tr("Download"));

    auto *msgBox = new QMessageBox();
    msgBox->setWindowTitle(tr("qView Update Available"));
    msgBox->setText("qView " + QString::number(latestVersionNum, 'f', 1) + tr(" is available to download.")
                    + "\n\n" + releaseDate.toString(locale.dateFormat()) + "\n" + changelog);
    msgBox->setWindowModality(Qt::ApplicationModal);
    msgBox->setStandardButtons(QMessageBox::Close | QMessageBox::Reset);
    msgBox->button(QMessageBox::Reset)->setText(tr("&Disable Update Checking"));
    msgBox->addButton(downloadButton, QMessageBox::ActionRole);
    connect(downloadButton, &QAbstractButton::clicked, this, [this]{
        QDesktopServices::openUrl(DOWNLOAD_URL);
    });
    connect(msgBox->button(QMessageBox::Reset), &QAbstractButton::clicked, qvApp, []{
        QSettings settings;
        settings.beginGroup("options");
        settings.setValue("updatenotifications", false);
        qvApp->getSettingsManager().loadSettings();
        QMessageBox::information(nullptr, tr("qView Update Checking Disabled"), tr("Update notifications on startup have been disabled.\nYou can reenable them in the options dialog."), QMessageBox::Ok);
    });
    msgBox->open();
    msgBox->setDefaultButton(QMessageBox::Close);
}
