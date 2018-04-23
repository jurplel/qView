#include "qvaboutdialog.h"
#include "ui_qvaboutdialog.h"
#include <QFontDatabase>
#include <QDate>
#include <QDebug>
#include <QJsonDocument>

QVAboutDialog::QVAboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVAboutDialog)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // add fonts
    QFontDatabase fontDatabase;
    fontDatabase.addApplicationFont(":/fonts/fonts/Lato-Light.ttf");
    fontDatabase.addApplicationFont(":/fonts/fonts/Lato-Regular.ttf");

    //set main title font
    #if (defined Q_OS_MACX) || (defined Q_OS_WIN)
    const QFont font1 = QFont("Lato", 72, QFont::Light);
    #else
    const QFont font1 = QFont("Lato Light", 72, QFont::Light);
    #endif
    ui->logoLabel->setFont(font1);

    //set subtitle font & text
    const QFont font2 = QFont("Lato", 18, QFont::Normal);
    const QString subtitleText = QString("version %1").arg(VERSION);
    ui->subtitleLabel->setFont(font2);
    ui->subtitleLabel->setText(subtitleText);

    //set update font & text
    const QFont font3 = QFont("Lato", 10, QFont::Normal);
    const QString updateText = QString("Checking for updates...");
    ui->updateLabel->setFont(font3);
    ui->updateLabel->setText(updateText);

    //set infolabel2 font, text, and properties
    const QFont font5 = QFont("Lato", 8, QFont::Normal);
    const QString labelText2 = QString("Built with Qt %2<br>Source code available under GPLv3 at <a style=\"color: #03A9F4; text-decoration:none;\" href=\"https://github.com/jeep70/qView\">Github</a><br>Icon glyph created by Guilhem from the Noun Project<br>Copyright © 2018-%1, jurplel and qView contributors").arg(QDate::currentDate().year()).arg(QT_VERSION_STR);
    ui->infoLabel2->setFont(font5);
    ui->infoLabel2->setText(labelText2);

    ui->infoLabel2->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->infoLabel2->setOpenExternalLinks(true);

    requestUpdates();
}
QVAboutDialog::~QVAboutDialog()
{
    delete ui;
}

void QVAboutDialog::requestUpdates()
{
    // send network request for update check
    QUrl url("https://api.github.com/repos/jeep70/qview/releases");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(checkUpdates(QNetworkReply*)));

    networkManager->get(request);
}

void QVAboutDialog::checkUpdates(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        ui->updateLabel->setText("Error checking for updates");
        return;
    }

    QByteArray byteArray = reply->readAll();
    QJsonDocument json = QJsonDocument::fromJson(byteArray);

    if (json.isNull())
    {
        ui->updateLabel->setText("Error checking for updates");
        return;
    }

    float latestVersionNum = json.array().first().toObject().value("tag_name").toString("0.0").toFloat();

    if (latestVersionNum == 0.0)
    {
        ui->updateLabel->setText("Error checking for updates");
        return;
    }
    if (latestVersionNum > VERSION)
    {
        const QString text = QString("<a style=\"color: #03A9F4; text-decoration:none;\" href=\"https://github.com/jeep70/qView/releases\">%1 update available!</a>").arg(latestVersionNum);
        ui->updateLabel->setText(text);
        ui->updateLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        ui->updateLabel->setOpenExternalLinks(true);
        ui->updateLabel->setEnabled(true);
    }
    else
    {
        ui->updateLabel->setText("No updates available");
    }
}
