#include "qvinfodialog.h"
#include "ui_qvinfodialog.h"
#include <QDateTime>
#include <QMimeDatabase>
#include <QTimer>
#include <exiv2/exiv2.hpp>
#include <QString>

static int getGcd (int a, int b) {
    return (b == 0) ? a : getGcd(b, a%b);
}

QVInfoDialog::QVInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVInfoDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint | Qt::CustomizeWindowHint));

    width = 0;
    height = 0;
    frameCount = 0;
}

QVInfoDialog::~QVInfoDialog()
{
    delete ui;
}

void QVInfoDialog::setInfo(const QFileInfo &value, const int &value2, const int &value3, const int &value4)
{
    selectedFileInfo = value;
    width = value2;
    height = value3;
    frameCount = value4;

    // If the dialog is visible, it means we've just navigated to a new image. Instead of running
    // updateInfo immediately, add it to the event queue. This is a workaround for a (Windows-specific?)
    // delay when calling adjustSize on the window if the font contains certain characters (e.g. Chinese)
    // the first time that happens for a given font. At least on Windows, by making the work happen later
    // in the event loop, it allows the main window to repaint first, giving the appearance of better
    // responsiveness. If the dialog is not visible, however, it means we're preparing to display for an
    // image already opened. In this case there is no urgency to repaint the main window, and we need to
    // process the updates here synchronously to avoid the caller showing the dialog before it's ready
    // (i.e. to avoid showing outdated info or placeholder text).
    if (isVisible())
        QTimer::singleShot(0, this, &QVInfoDialog::updateInfo);
    else
        updateInfo();
}

QString readExifData(const QString &imagePath) {
    try {
        auto image = Exiv2::ImageFactory::open(imagePath.toStdString());
        if (!image.get()) return "Error opening image.";

        image->readMetadata();
        Exiv2::ExifData &exifData = image->exifData();
        if (exifData.empty()) return "No EXIF data found.";

        QString exifOutput;

        auto getStr = [&](const char* key) -> QString {
            auto it = exifData.findKey(Exiv2::ExifKey(key));
            return (it != exifData.end()) ? QString::fromStdString(it->toString()) : "N/A";
        };

        auto getRationalAsFloat = [&](const char* key, int precision = 2) -> QString {
            auto it = exifData.findKey(Exiv2::ExifKey(key));
            if (it != exifData.end()) {
                Exiv2::Rational r = it->toRational();
                if (r.second != 0) {
                    double value = static_cast<double>(r.first) / r.second;
                    return QString::number(value, 'f', precision);
                }
            }
            return "N/A";
        };

        exifOutput += "Camera Make: " + getStr("Exif.Image.Make") + "\n";
        exifOutput += "Camera Model: " + getStr("Exif.Image.Model") + "\n";
        exifOutput += "Software: " + getStr("Exif.Image.Software") + "\n";
        exifOutput += "DateTime: " + getStr("Exif.Image.DateTime") + "\n";
        exifOutput += "Exposure Time: " + getStr("Exif.Photo.ExposureTime") + "s\n";
        exifOutput += "Aperture: F" + getRationalAsFloat("Exif.Photo.FNumber") + "\n";
        exifOutput += "ISO Speed: " + getStr("Exif.Photo.ISOSpeedRatings") + "\n";
        exifOutput += "DateTime Original: " + getStr("Exif.Photo.DateTimeOriginal") + "\n";
        exifOutput += "DateTime Digitized: " + getStr("Exif.Photo.DateTimeDigitized") + "\n";
        exifOutput += "Metering Mode: " + getStr("Exif.Photo.MeteringMode") + "\n";
        exifOutput += "Light Source: " + getStr("Exif.Photo.LightSource") + "\n";
        exifOutput += "Flash: " + getStr("Exif.Photo.Flash") + "\n";
        exifOutput += "Focal Length: " + getRationalAsFloat("Exif.Photo.FocalLength") + " mm\n";
        exifOutput += "GPS Latitude: " + getStr("Exif.GPSInfo.GPSLatitude") + "\n";
        exifOutput += "GPS Longitude: " + getStr("Exif.GPSInfo.GPSLongitude") + "\n";
        return exifOutput;
    } catch (const Exiv2::Error &e) {
        return QString("EXIF Error: ") + e.what();
    }
}

void QVInfoDialog::updateInfo()
{
    QLocale locale = QLocale::system();
    QMimeDatabase mimedb;
    QMimeType mime = mimedb.mimeTypeForFile(selectedFileInfo.absoluteFilePath(), QMimeDatabase::MatchContent);
    //this is just math to figure the megapixels and then round it to the tenths place
    const double megapixels = static_cast<double>(qRound(((static_cast<double>((width*height))))/1000000 * 10 + 0.5)) / 10 ;

    QString exifData = readExifData(selectedFileInfo.absoluteFilePath());

    ui->nameLabel->setText(selectedFileInfo.fileName());
    ui->typeLabel->setText(mime.name());
    ui->locationLabel->setText(selectedFileInfo.path());
    ui->sizeLabel->setText(tr("%1 (%2 bytes)").arg(formatBytes(selectedFileInfo.size()), locale.toString(selectedFileInfo.size())));
    ui->modifiedLabel->setText(selectedFileInfo.lastModified().toString(locale.dateTimeFormat()));
    ui->dimensionsLabel->setText(tr("%1 x %2 (%3 MP)").arg(QString::number(width), QString::number(height), QString::number(megapixels)));
    ui->miscLabel->setText(exifData);

    int gcd = getGcd(width,height);
    if (gcd != 0)
        ui->ratioLabel->setText(QString::number(width/gcd) + ":" + QString::number(height/gcd));
    if (frameCount != 0)
    {
        ui->framesLabel2->show();
        ui->framesLabel->show();
        ui->framesLabel->setText(QString::number(frameCount));
    }
    else
    {
        ui->framesLabel2->hide();
        ui->framesLabel->hide();
    }
    window()->adjustSize();
}
