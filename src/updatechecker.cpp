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

void UpdateChecker::check(bool isStartupCheck)
{
    if (isStartupCheck)
    {
        QDateTime lastCheckTime = getLastCheckTime();
        if (lastCheckTime.isValid() && QDateTime::currentDateTimeUtc() < lastCheckTime.addSecs(STARTUP_CHECK_INTERVAL_HOURS * 3600))
            return;
    }

    sendRequest(UPDATE_URL + "/latest");
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

    QJsonObject object = json.object();

    latestVersionNum = object.value("tag_name").toString("0.0").toDouble();

    static const QRegularExpression newLineRegEx("\r?\n");
    QStringList changelogList = object.value("body").toString().split(newLineRegEx);
    // remove "changelog" heading
    changelogList.removeFirst();
    // remove additional newline if present
    if (!changelogList.isEmpty() && changelogList.first().isEmpty())
        changelogList.removeFirst();
    changelog = changelogList.join("\n");

    releaseDate = QDateTime::fromString(object.value("published_at").toString(), Qt::ISODate);
    releaseDate = releaseDate.toTimeSpec(Qt::LocalTime);

    setLastCheckTime(QDateTime::currentDateTimeUtc());

    emit checkedUpdates();
}

QDateTime UpdateChecker::getLastCheckTime() const
{
    qint64 secsSinceEpoch = QSettings().value("lastupdatecheck").toLongLong();
    return secsSinceEpoch == 0 ? QDateTime() : QDateTime::fromSecsSinceEpoch(secsSinceEpoch, Qt::UTC);
}

void UpdateChecker::setLastCheckTime(QDateTime value)
{
    QSettings().setValue("lastupdatecheck", value.toSecsSinceEpoch());
}

void UpdateChecker::openDialog()
{
    QLocale locale;
    auto *downloadButton = new QPushButton(QIcon::fromTheme("edit-download", QIcon::fromTheme("document-save")), tr("Download"));

    auto *msgBox = new QMessageBox();
    msgBox->setWindowTitle(tr("qView Update Available"));
    msgBox->setText(tr("qView %1 is available to download.").arg(QString::number(latestVersionNum, 'f', 1))
                    + "\n\n" + releaseDate.toString(locale.dateFormat()) + "\n\n" + changelog);
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
