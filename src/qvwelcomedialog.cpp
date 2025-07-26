#include "qvwelcomedialog.h"
#include "ui_qvwelcomedialog.h"

#include "qvapplication.h"

#include <QFontDatabase>
#include <QSettings>

QVWelcomeDialog::QVWelcomeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVWelcomeDialog)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint | Qt::CustomizeWindowHint));

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
    const QFont font1 = QFont("Lato", 72, QFont::Light);
    modifier = 4;
#else
    QFont font1 = QFont("Lato", 54, QFont::Light);
#endif
    ui->logoLabel->setFont(font1);

    //set subtitle font & text
    QFont font2 = QFont("Lato", 14 + modifier);
    font2.setStyleName("Regular");
    const QString subtitleText = tr("Thank you for downloading qView.<br>Here's a few tips to get you started:");
    ui->subtitleLabel->setFont(font2);
    ui->subtitleLabel->setText(subtitleText);

    //set info font & text
    QFont font3 = QFont("Lato", 12 + modifier);
    font3.setStyleName("Regular");
    const QString updateText = tr("<ul><li>Right click to access the main menu</li><li>Drag the image to reposition it</li><li>Scroll to zoom in and out</li><li>Use arrow keys to switch files</li></ul>");
    ui->infoLabel->setFont(font3);
    ui->infoLabel->setText(updateText);

    ui->infoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->infoLabel->setOpenExternalLinks(true);

#ifdef QV_DISABLE_ONLINE_VERSION_CHECK
    ui->updateCheckBox->hide();
#else
    ui->updateCheckBox->setChecked(qvApp->getSettingsManager().getBool("updatenotifications"));
    connect(ui->updateCheckBox, &QCheckBox::stateChanged, qvApp, [](int state){
        QSettings settings;
        settings.beginGroup("options");
        settings.setValue("updatenotifications", state > 0);
        qvApp->getSettingsManager().loadSettings();
    });
#endif //QV_DISABLE_ONLINE_VERSION_CHECK
}

QVWelcomeDialog::~QVWelcomeDialog()
{
    delete ui;
}
