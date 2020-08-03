#include "openwith.h"

#include <QDir>
#include <QProcess>
#include <QStandardPaths>

#include <QDebug>

const QList<OpenWith::OpenWithItem> OpenWith::getOpenWithItems(const QString &mimeName)
{
    QProcess process;
    process.start("xdg-mime", {"query", "default", mimeName});
    process.waitForFinished();
    QString defaultApplication = process.readAllStandardOutput().trimmed();

    QList<OpenWithItem> listOfOpenWithItems;

    QList<QMap<QString, QString>> programList;

    const QStringList &applicationLocations = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    for (const auto &location : applicationLocations)
    {
        auto dir = QDir(location);
        const auto &entryInfoList = dir.entryInfoList();
        for(const auto &fileInfo : entryInfoList)
        {
            // Don't add qView to the open with menu!
            if (fileInfo.fileName() == "qView.desktop")
                continue;

            OpenWithItem openWithItem;

            QFile file(fileInfo.absoluteFilePath());
            file.open(QIODevice::ReadOnly);
            QTextStream in(&file);
            QString line;
            bool addToList = false;
            while (in.readLineInto(&line))
            {
                if (line.startsWith("Name=", Qt::CaseInsensitive) && openWithItem.name.isEmpty())
                {
                    line.remove("Name=", Qt::CaseInsensitive);
                    openWithItem.name = line;
                }
                else if (line.startsWith("Icon=", Qt::CaseInsensitive) && openWithItem.icon.isNull())
                {
                    line.remove("Icon=", Qt::CaseInsensitive);
                    openWithItem.icon = QIcon::fromTheme(line);
                }
                else if (line.startsWith("Exec=", Qt::CaseInsensitive) && openWithItem.exec.isEmpty())
                {
                    line.remove("Exec=", Qt::CaseInsensitive);
                    QRegExp regExp;
                    regExp.setPatternSyntax(QRegExp::Wildcard);
                    regExp.setPattern("%?");
                    line.remove(regExp);
                    openWithItem.exec = line;
                }
                else if (line.startsWith("MimeType=", Qt::CaseInsensitive))
                {
                    if (line.contains(mimeName, Qt::CaseInsensitive))
                        addToList = true;
                }
            }
            if (addToList)
            {
                // Add the default program to the beginning of the menu
                if (fileInfo.fileName() == defaultApplication)
                {
                    listOfOpenWithItems.prepend(openWithItem);
                }
                else
                {
                    listOfOpenWithItems.append(openWithItem);
                }
            }
        }
    }

    return listOfOpenWithItems;
}

