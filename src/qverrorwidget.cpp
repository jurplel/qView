#include "qvapplication.h"
#include "qverrorwidget.h"
#include "ui_qverrorwidget.h"

QVErrorWidget::QVErrorWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QVErrorWidget)
{
    ui->setupUi(this);

    setAutoFillBackground(true);

    // Connect to settings signal
    connect(&qvApp->getSettingsManager(), &SettingsManager::settingsUpdated, this, &QVErrorWidget::settingsUpdated);
    settingsUpdated();
}

QVErrorWidget::~QVErrorWidget()
{
    delete ui;
}

void QVErrorWidget::setError(int errorNum, const QString &errorString, const QString &fileName)
{
    auto errorMessage = QStringList
    {
        tr("Error occurred opening"),
        fileName,
        tr("%1 (Error %2)").arg(errorString).arg(errorNum)
    };
    ui->errorMessageLabel->setText(errorMessage.join("\n"));
}

void QVErrorWidget::settingsUpdated()
{
    updateBackgroundColor();
    updateTextColor();
}

void QVErrorWidget::updateBackgroundColor()
{
    auto &settingsManager = qvApp->getSettingsManager();

    QBrush newBrush;
    if (!settingsManager.getBoolean("bgcolorenabled"))
    {
        newBrush.setStyle(Qt::NoBrush);
    }
    else
    {
        newBrush.setStyle(Qt::SolidPattern);
        newBrush.setColor(QColor(settingsManager.getString("bgcolor")));
    }

    auto newPalette = palette();
    newPalette.setBrush(QPalette::Window, newBrush);
    setPalette(newPalette);
}

void QVErrorWidget::updateTextColor()
{
    auto &settingsManager = qvApp->getSettingsManager();

    const auto bgColor = QColor(settingsManager.getString("bgcolor"));
    double bgLuminance = (0.299 * bgColor.red() + 0.587 * bgColor.green() + 0.114 * bgColor.blue())/255;

    QColor newColor;
    if (bgLuminance > 0.5 || !settingsManager.getBoolean("bgcolorenabled"))
    {
        newColor.setRgb(32, 32, 32);
    }
    else
    {
        newColor.setRgb(223, 223, 223);
    }

    auto newPalette = palette();
    newPalette.setBrush(QPalette::WindowText, newColor);
    setPalette(newPalette);
}
