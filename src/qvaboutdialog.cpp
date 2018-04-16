#include "qvaboutdialog.h"
#include "ui_qvaboutdialog.h"
#include <QFontDatabase>
#include <QDate>
#include <QDebug>

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
    #if defined(Q_OS_MACX) || (Q_OS_WIN)
    const QFont font1 = QFont("Lato", 96, QFont::Light);
    #else
    const QFont font1 = QFont("Lato Light", 96, QFont::Light);
    #endif
    ui->logoLabel->setFont(font1);

    //set subtitle font & text
    const QFont font2 = QFont("Lato", 24, QFont::Normal);
    ui->subtitleLabel->setFont(font2);

    QString subtitleText = QString("version %1").arg(VERSION);
    ui->subtitleLabel->setText(subtitleText);

    //set label1 font & text
    const QFont font3 = QFont("Lato", 14, QFont::Normal);
    ui->infoLabel->setFont(font3);

    QString labelText = QString("Built with Qt %1").arg(QT_VERSION_STR);
    ui->infoLabel->setText(labelText);

    //set label2 font, text, and properties
    const QFont font4 = QFont("Lato", 10, QFont::Normal);
    ui->infoLabel2->setFont(font4);

    QString labelText2 = QString("Source code available under GPLv3 at <a href=\"https://github.com/jeep70/qView\">Github</a><br>Icon glyph created by Guilhem from the Noun Project<br>Copyright © 2018-%1, jurplel and qView contributors").arg(QDate::currentDate().year());

    ui->infoLabel2->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->infoLabel2->setOpenExternalLinks(true);
    ui->infoLabel2->setText(labelText2);
}
QVAboutDialog::~QVAboutDialog()
{
    delete ui;
}
