#ifndef VIPSREADER_H
#define VIPSREADER_H

#include <QByteArray>
#include <QImage>
#include <QString>

class VipsReader
{
public:
    struct ReadResult
    {
        QImage image;
        QByteArray colorProfile;
        QString error;
    };

    static ReadResult read(const QString &fileName);

    static void init();
    static void shutdown();
};

#endif // VIPSREADER_H
