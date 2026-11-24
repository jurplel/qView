#include "qvexifpanel.h"

#include <exiv2/exiv2.hpp>

#include <QFile>
#include <QHeaderView>
#include <QLabel>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QtEndian>

#include <sstream>
#include <stdexcept>

namespace {
QString fromStdString(const std::string &value)
{
    return QString::fromUtf8(value.data(), static_cast<int>(value.size()));
}

Exiv2::ExifData readPngExif(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error(file.errorString().toStdString());

    const QByteArray pngSignature = QByteArray::fromRawData("\x89PNG\r\n\x1a\n", 8);
    if (file.read(pngSignature.size()) != pngSignature)
        throw std::runtime_error("Invalid PNG signature");

    Exiv2::ExifData exifData;
    while (file.pos() + 12 <= file.size()) {
        const QByteArray chunkHeader = file.read(8);
        if (chunkHeader.size() != 8)
            break;

        const quint32 chunkLength =
                qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(chunkHeader.constData()));
        const QByteArray chunkType = chunkHeader.mid(4, 4);
        if (chunkLength > static_cast<quint64>(file.size() - file.pos() - 4))
            throw std::runtime_error("Invalid PNG chunk length");

        if (chunkType == "eXIf") {
            const QByteArray exifBytes = file.read(static_cast<qint64>(chunkLength));
            if (exifBytes.size() != static_cast<qint64>(chunkLength))
                throw std::runtime_error("Incomplete PNG EXIF chunk");

            const auto *payload = reinterpret_cast<const Exiv2::byte *>(exifBytes.constData());
            Exiv2::ExifParser::decode(exifData, payload, static_cast<size_t>(exifBytes.size()));
            return exifData;
        }

        if (!file.seek(file.pos() + chunkLength + 4))
            throw std::runtime_error("Unable to seek past PNG chunk");
        if (chunkType == "IEND")
            break;
    }
    return exifData;
}

Exiv2::ExifData readExifData(const QString &filePath)
{
    if (filePath.endsWith(".png", Qt::CaseInsensitive)
        || filePath.endsWith(".apng", Qt::CaseInsensitive)) {
        return readPngExif(filePath);
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error(file.errorString().toStdString());

    const QByteArray fileData = file.readAll();
    const auto *payload = reinterpret_cast<const Exiv2::byte *>(fileData.constData());
    auto image = Exiv2::ImageFactory::open(payload, static_cast<size_t>(fileData.size()));
    if (!image)
        throw std::runtime_error("Unable to open image metadata");

    image->readMetadata();
    return image->exifData();
}
} // namespace

QVExifPanel::QVExifPanel(QWidget *parent) : QWidget(parent)
{
    setObjectName("exifPanel");
    setMinimumWidth(240);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    auto *titleLabel = new QLabel(tr("EXIF"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    statusLabel = new QLabel(this);
    statusLabel->setObjectName("exifStatusLabel");
    statusLabel->setWordWrap(true);
    layout->addWidget(statusLabel);

    treeWidget = new QTreeWidget(this);
    treeWidget->setObjectName("exifTreeWidget");
    treeWidget->setColumnCount(2);
    treeWidget->setHeaderLabels({ tr("Property"), tr("Value") });
    treeWidget->setRootIsDecorated(false);
    treeWidget->setUniformRowHeights(true);
    treeWidget->setItemsExpandable(false);
    treeWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    treeWidget->setFocusPolicy(Qt::NoFocus);
    treeWidget->setTextElideMode(Qt::ElideNone);
    treeWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    treeWidget->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    layout->addWidget(treeWidget, 1);

    statusLabel->setText(tr("Open an image to view EXIF information."));
    treeWidget->hide();
}

void QVExifPanel::setFilePath(const QString &filePath)
{
    treeWidget->setUpdatesEnabled(false);
    treeWidget->clear();

    const auto showMessage = [this](const QString &message) {
        treeWidget->hide();
        statusLabel->setText(message);
    };

    if (filePath.isEmpty()) {
        showMessage(tr("Open an image to view EXIF information."));
        treeWidget->setUpdatesEnabled(true);
        return;
    }

    try {
        const Exiv2::ExifData exifData = readExifData(filePath);
        if (exifData.empty()) {
            showMessage(tr("No EXIF information found."));
            treeWidget->setUpdatesEnabled(true);
            return;
        }

        for (const auto &datum : exifData) {
            std::ostringstream valueStream;
            datum.write(valueStream, &exifData);

            const QString key = fromStdString(datum.key());
            const QString value = fromStdString(valueStream.str());
            auto *item = new QTreeWidgetItem(treeWidget, { key, value });
            item->setData(0, Qt::UserRole, key);
            item->setToolTip(0, key);
            item->setToolTip(1, value);
        }

        statusLabel->setText(tr("%1 EXIF fields").arg(exifData.count()));
        treeWidget->show();
    } catch (const std::exception &error) {
        showMessage(tr("Unable to read EXIF information: %1").arg(QString::fromUtf8(error.what())));
    }

    treeWidget->setUpdatesEnabled(true);
    treeWidget->viewport()->update();
}
