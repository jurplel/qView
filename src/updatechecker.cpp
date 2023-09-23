#include "updatechecker.h"

#include "qvapplication.h"

#include <QMessageBox>
#include <QPushButton>
#include <QDateTime>
#include <QRegularExpression>
#include <QDesktopServices>

UpdateChecker::UpdateChecker(QObject *parent) : QObject(parent)
{
    latestVersionNum = -1.0;

    connect(&netAccessManager, &QNetworkAccessManager::finished, this, &UpdateChecker::readReply);
}

void UpdateChecker::check()
{
#ifndef NIGHTLY
    emit checkedUpdates();
    return;
#endif

    latestVersionNum = 0.0;
    QNetworkRequest request(API_BASE_URL + "/latest");
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

    QJsonObject object = json.object();

    latestVersionNum = parseVersion(object.value("tag_name").toString());

    changelog = object.value("body").toString();

    releaseDate = QDateTime::fromString(object.value("published_at").toString(), Qt::ISODate);
    releaseDate = releaseDate.toTimeSpec(Qt::LocalTime);

    emit checkedUpdates();
}

double UpdateChecker::parseVersion(QString str)
{
    return str.remove(QRegularExpression("[^0-9]")).left(8).toDouble();
}

bool UpdateChecker::isVersionConsideredUpdate(double v)
{
#ifndef NIGHTLY
    return false;
#endif

    return v > 0 && v > parseVersion(QT_STRINGIFY(NIGHTLY));
}

void UpdateChecker::openDialog()
{
    QLocale locale;
    auto *downloadButton = new QPushButton(QIcon::fromTheme("edit-download", QIcon::fromTheme("document-save")), tr("Download"));

    auto *msgBox = new QMessageBox();
    msgBox->setWindowTitle(tr("qView Update Available"));
    msgBox->setText(tr("A newer version is available to download.")
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
