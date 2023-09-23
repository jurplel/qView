#include "updatechecker.h"

#include "qvapplication.h"

#include <QMessageBox>
#include <QPushButton>
#include <QDateTime>
#include <QRegularExpression>
#include <QDesktopServices>

UpdateChecker::UpdateChecker(QObject *parent) : QObject(parent)
{
    isChecking = false;
    hasChecked = false;

    connect(&netAccessManager, &QNetworkAccessManager::finished, this, &UpdateChecker::readReply);
}

void UpdateChecker::check()
{
    if (isChecking)
        return;

#ifndef NIGHTLY
    // This fork uses only the nightly build number for update versioning
    onError(tr("This build is not configured for update checking."));
    return;
#endif

    isChecking = true;
    QNetworkRequest request(API_BASE_URL + "/latest");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    netAccessManager.get(request);
}

void UpdateChecker::readReply(QNetworkReply *reply)
{
    isChecking = false;
    hasChecked = true;

    if (reply->error() != QNetworkReply::NoError)
    {
        onError(reply->errorString());
        return;
    }

    QJsonDocument json = QJsonDocument::fromJson(reply->readAll());

    if (json.isNull())
    {
        onError(tr("Received null JSON."));
        return;
    }

    QJsonObject object = json.object();

    checkResult = {
        true,
        {},
        parseVersion(object.value("tag_name").toString()),
        object.value("name").toString(),
        object.value("body").toString()
    };

    emit checkedUpdates();
}

void UpdateChecker::onError(QString msg)
{
    checkResult = {
        false,
        msg,
        {},
        {},
        {}
    };

    emit checkedUpdates();
}

double UpdateChecker::parseVersion(QString str)
{
    return str.remove(QRegularExpression("[^0-9]")).left(8).toDouble();
}

bool UpdateChecker::isVersionConsideredUpdate(double v)
{
#ifndef NIGHTLY
    // This fork uses only the nightly build number for update versioning
    return false;
#endif

    return v > 0 && v > parseVersion(QT_STRINGIFY(NIGHTLY));
}

void UpdateChecker::openDialog(QWidget *parent, bool showDisableButton)
{
    if (!(hasChecked && checkResult.wasSuccessful && checkResult.isConsideredUpdate()))
        return;

    QLocale locale;
    auto *downloadButton = new QPushButton(QIcon::fromTheme("edit-download", QIcon::fromTheme("document-save")), tr("Download"));

    auto *msgBox = new QMessageBox(parent);
    msgBox->setWindowTitle(tr("qView Update Available"));
    msgBox->setText(tr("A newer version is available to download.")
                    + "\n\n" + checkResult.releaseName + ":\n" + checkResult.changelog);
    msgBox->setWindowModality(Qt::ApplicationModal);
    msgBox->setStandardButtons(QMessageBox::Close | (showDisableButton ? QMessageBox::Reset : QMessageBox::NoButton));
    msgBox->addButton(downloadButton, QMessageBox::ActionRole);
    connect(downloadButton, &QAbstractButton::clicked, this, [this]{
        QDesktopServices::openUrl(DOWNLOAD_URL);
    });
    if (showDisableButton)
    {
        msgBox->button(QMessageBox::Reset)->setText(tr("&Disable Update Checking"));
        connect(msgBox->button(QMessageBox::Reset), &QAbstractButton::clicked, qvApp, []{
            QSettings settings;
            settings.beginGroup("options");
            settings.setValue("updatenotifications", false);
            qvApp->getSettingsManager().loadSettings();
            QMessageBox::information(nullptr, tr("qView Update Checking Disabled"), tr("Update notifications on startup have been disabled.\nYou can reenable them in the options dialog."), QMessageBox::Ok);
        });
    }
    msgBox->open();
    msgBox->setDefaultButton(QMessageBox::Close);
}
