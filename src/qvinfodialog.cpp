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

        auto getGpsCoordinates = [&](const char* coordKey, const char* refKey) -> QString {
            auto coordIt = exifData.findKey(Exiv2::ExifKey(coordKey));
            auto refIt = exifData.findKey(Exiv2::ExifKey(refKey));

            if (coordIt != exifData.end() && refIt != exifData.end()) {
                QString ref = QString::fromStdString(refIt->toString());
                if (ref.isEmpty()) return "N/A";

                const auto value = coordIt->getValue();
                if (value->count() < 3) return "N/A";

                Exiv2::Rational deg = value->toRational(0);
                Exiv2::Rational min = value->toRational(1);
                Exiv2::Rational sec = value->toRational(2);

                double degrees = static_cast<double>(deg.first) / deg.second;
                double minutes = static_cast<double>(min.first) / min.second;
                double seconds = static_cast<double>(sec.first) / sec.second;
                int secPrecision = (sec.second > 1) ? static_cast<int>(std::ceil(std::log10(sec.second))) : 0;

                auto padSeconds = [](double seconds, int precision) -> QString {
                    int intPart = static_cast<int>(seconds);
                    double fracPart = seconds - intPart;

                    QString intStr = QString("%1").arg(intPart, 2, 10, QChar('0'));
                    QString fracStr = QString::number(fracPart, 'f', precision).mid(1);

                    return intStr + fracStr;
                };

                return QString("%1° %2′ %3″ %4")
                    .arg(degrees, 0, 'f', 0)
                    .arg(minutes, 2, 'f', 0, '0')
                    .arg(padSeconds(seconds, secPrecision))
                    .arg(ref);
            }
            return "N/A";
        };

        auto getGpsAltitude = [&]() -> QString {
            auto altIt = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSAltitude"));
            auto refIt = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSAltitudeRef"));

            if (altIt != exifData.end() && refIt != exifData.end()) {
                Exiv2::Rational alt = altIt->toRational();
                double altitude = static_cast<double>(alt.first) / alt.second;
                int altPrecision = (alt.second > 1) ? static_cast<int>(std::ceil(std::log10(alt.second))) : 0;

                QString ref = QString::fromStdString(refIt->toString());
                if (ref == "1") {
                    altitude = -altitude;
                }

                return QString::number(altitude, 'f', altPrecision) + " m";
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
        exifOutput += "GPS Latitude: " + getGpsCoordinates("Exif.GPSInfo.GPSLatitude", "Exif.GPSInfo.GPSLatitudeRef") + "\n";
        exifOutput += "GPS Longitude: " + getGpsCoordinates("Exif.GPSInfo.GPSLongitude", "Exif.GPSInfo.GPSLongitudeRef") + "\n";
        exifOutput += "GPS Altitude: " + getGpsAltitude() + "\n";
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
