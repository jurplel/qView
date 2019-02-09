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
    QFontDatabase::addApplicationFont(":/fonts/res/Lato-Light.ttf");
    QFontDatabase::addApplicationFont(":/fonts/res/Lato-Regular.ttf");

    int modifier = 0;
    //set main title font
    #if (defined Q_OS_MACX)
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
    const QString updateText = tr("<ul><li>Right click to access the main menu</li><li>Drag the image to reposition it</li><li>Scroll to zoom in and out</li><li>Use arrow keys to switch files</li><li><a style='color: #03A9F4; text-decoration:none;' href='https://github.com/jurplel/qView/wiki/Keyboard-Shortcuts'>View more keyboard shortcuts</a></li></ul>");
    ui->infoLabel->setFont(font3);
    ui->infoLabel->setText(updateText);

    ui->infoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->infoLabel->setOpenExternalLinks(true);
}

QVWelcomeDialog::~QVWelcomeDialog()
{
    delete ui;
}
