#ifndef QVINFODIALOG_H
#define QVINFODIALOG_H

#include <QDialog>
#include <QFileInfo>
#include <QLocale>

namespace Ui {
class QVInfoDialog;
}

class QVInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QVInfoDialog(QWidget *parent = nullptr);
    ~QVInfoDialog();

    void setInfo(const QFileInfo &value, const int &value2, const int &value3, const int &value4);

    void updateInfo();

private:
    Ui::QVInfoDialog *ui;

    QFileInfo selectedFileInfo;
    int width;
    int height;

    int frameCount;

public:
    static QString formatBytes(qint64 bytes)
    {
        QString sizeString;
        #if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
        QLocale locale;
        sizeString = locale.formattedDataSize(bytes);
        #else

        double size = bytes;

        int reductionAmount = 0;
        double newSize = size/1024;
        while (newSize > 1024)
        {
            newSize /= 1024;
            reductionAmount++;
            if (reductionAmount > 2)
                break;
        }

        QString unit;
        switch (reductionAmount)
        {
        case 0: {
            unit = " KiB";
            break;
        }
        case 1: {
            unit = " MiB";
            break;
        }
        case 2: {
            unit = " GiB";
            break;
        }
        case 3: {
            unit = " TiB";
            break;
        }
        }

        sizeString = QString::number(newSize, 'f', 2) + unit;
        #endif
        return sizeString;
    }
};

#endif // QVINFODIALOG_H
