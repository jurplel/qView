#include "qvaboutdialog.h"
#include "ui_qvaboutdialog.h"

#include "qvapplication.h"

#include <QFontDatabase>
#include <QJsonDocument>

QVAboutDialog::QVAboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVAboutDialog)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint | Qt::CustomizeWindowHint));

    connect(ui->checkForUpdatesButton, &QPushButton::clicked, this, &QVAboutDialog::checkForUpdatesButtonClicked);

    // Application modal on mac, window modal everywhere else
#ifdef Q_OS_MACOS
    setWindowModality(Qt::ApplicationModal);
#else
    setWindowModality(Qt::WindowModal);
#endif

    // add fonts
    QFontDatabase::addApplicationFont(":/fonts/Lato-Light.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-Regular.ttf");

    int modifier = 0;
    //set main title font
#ifdef Q_OS_MACOS
    const QFont font1 = QFont("Lato", 96, QFont::Light);
    modifier = 4;
#else
    const QFont font1 = QFont("Lato", 72, QFont::Light);
#endif
    ui->logoLabel->setFont(font1);

    //set subtitle font & text
    QFont font2 = QFont("Lato", 18 + modifier);
    font2.setStyleName("Regular");
    QString subtitleText = tr("Unofficial Fork (jdpurcell)<br>");
#ifdef NIGHTLY
    subtitleText += tr("Version %1").arg(QT_STRINGIFY(NIGHTLY));
#else
    //this fork has no formal releases, just "nightly" builds promoted to releases
    subtitleText += tr("Unspecified Version");
#endif
    ui->subtitleLabel->setFont(font2);
    ui->subtitleLabel->setText(subtitleText);

    //set infolabel2 font, text, and properties
    QFont font4 = QFont("Lato", 8 + modifier);
    font4.setStyleName("Regular");
    const QString labelText2 = tr("Built with Qt %1 (%2)<br>"
                                  "Licensed under the GNU GPLv3<br>"
                                  R"(Derivative of official qView: <a style="color: #03A9F4; text-decoration:none;" href="https://interversehq.com/qview/">Website</a>, <a style="color: #03A9F4; text-decoration:none;" href="https://github.com/jurplel/qView">GitHub</a><br>)"
                                  "Icon glyph created by Guilhem from the Noun Project<br>"
                                  "Copyright © %3 jurplel, jdpurcell, and qView contributors")
                                  .arg(QT_VERSION_STR, QSysInfo::buildCpuArchitecture(), "2018-2023");

    ui->infoLabel2->setFont(font4);
    ui->infoLabel2->setText(labelText2);

    ui->infoLabel2->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->infoLabel2->setOpenExternalLinks(true);

    updateCheckForUpdatesButtonState();
}

QVAboutDialog::~QVAboutDialog()
{
    delete ui;
}

void QVAboutDialog::updateCheckForUpdatesButtonState()
{
    ui->checkForUpdatesButton->setEnabled(!qvApp->getUpdateChecker().getIsChecking());
}

void QVAboutDialog::checkForUpdatesButtonClicked()
{
    qvApp->getUpdateChecker().check(true);
    updateCheckForUpdatesButtonState();
}
