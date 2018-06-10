#include "qvwelcomedialog.h"
#include "ui_qvwelcomedialog.h"
#include <QFontDatabase>

QVWelcomeDialog::QVWelcomeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVWelcomeDialog)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // add fonts
    QFontDatabase fontDatabase;
    fontDatabase.addApplicationFont(":/fonts/fonts/Lato-Light.ttf");
    fontDatabase.addApplicationFont(":/fonts/fonts/Lato-Regular.ttf");

    int modifier = 0;
    //set main title font
    #if (defined Q_OS_WIN)
    const QFont font1 = QFont("Lato", 54, QFont::Light);
    #elif (defined Q_OS_MACX)
    const QFont font1 = QFont("Lato", 72, QFont::Light);
    modifier = 4;
    #else
    const QFont font1 = QFont("Lato Light", 54, QFont::Light);
    #endif
    ui->logoLabel->setFont(font1);

    //set subtitle font & text
    const QFont font2 = QFont("Lato", 14 + modifier, QFont::Normal);
    const QString subtitleText = tr("Thank you for downloading qView.<br>Here's a few tips to get you started:");
    ui->subtitleLabel->setFont(font2);
    ui->subtitleLabel->setText(subtitleText);

    //set info font & text
    const QFont font3 = QFont("Lato", 12 + modifier, QFont::Normal);
    const QString updateText = tr("<ul><li>Right click to access the main menu<li>Use arrow keys to switch files<li>Drag the image to reposition it<li>Scroll to zoom in and out</ul>");
    ui->infoLabel->setFont(font3);
    ui->infoLabel->setText(updateText);
}

QVWelcomeDialog::~QVWelcomeDialog()
{
    delete ui;
}
