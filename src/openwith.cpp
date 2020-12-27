#include "mainwindow.h"
#include "openwith.h"
#include "qvcocoafunctions.h"
#include "ui_qvopenwithdialog.h"

#include "ShlObj_core.h"
#include "winuser.h"

#include <QCollator>
#include <QDir>
#include <QFileDialog>
#include <QProcess>
#include <QStandardPaths>
#include <QMimeDatabase>
#include <QSettings>
#include <QVersionNumber>

#include <QDebug>

const QList<OpenWith::OpenWithItem> OpenWith::getOpenWithItems(const QString &filePath)
{
    QList<OpenWithItem> listOfOpenWithItems;

#ifdef Q_OS_MACOS
    listOfOpenWithItems = QVCocoaFunctions::getOpenWithItems(filePath);
#elif defined Q_OS_WIN
    QFileInfo info(filePath);

    IEnumAssocHandlers *assocHandlers = 0;
    HRESULT result = SHAssocEnumHandlers(qUtf16Printable("." + info.suffix()), ASSOC_FILTER_RECOMMENDED, &assocHandlers);
    if (!SUCCEEDED(result))
        qDebug() << "win32 failed point 1";

    ULONG retrieved = 0;
    HRESULT nextResult = S_OK;
    while (nextResult == S_OK)
    {
        IAssocHandler *handlers = 0;
        nextResult = assocHandlers->Next(1, &handlers, &retrieved);
        if (!SUCCEEDED(nextResult) || retrieved < 1)
        {
            qDebug() << "win32 failed point 2";
            break;
        }
        WCHAR *uiName = 0;
        result = handlers[0].GetUIName(&uiName);
        if (!SUCCEEDED(result))
        {
            qDebug() << "win32 failed point 3";
            continue;
        }

        WCHAR *name = 0;
        result = handlers[0].GetName(&name);
        if (!SUCCEEDED(result))
        {
            qDebug() << "win32 failed point 4";
            continue;
        }

        WCHAR *icon = 0;
        int index = 0;
        result = handlers[0].GetIconLocation(&icon, &index);
        if (!SUCCEEDED(result))
        {
            qDebug() << "win32 failed point 5";
            continue;
        }

        QString title = QString::fromWCharArray(uiName);
        QString path = QString::fromWCharArray(name);
        QString iconLocation = QString::fromWCharArray(icon);
        if (title == path) // If it's either invalid or a windows store app
        {
            qDebug() << QSysInfo::kernelVersion();
            if (QVersionNumber::fromString(QSysInfo::kernelVersion()) >= QVersionNumber(11) && iconLocation.contains("ms-resource")) // If it's a windows store app
            {
                QStringList split = iconLocation.split('?');
                QString packageFullName = split.first().remove(0, 2);
                qDebug() << packageFullName;
            }
            else
            {
                continue;
            }
        }
        else
        {
            // Remove items that have a path that does not exist
            QFile file(path);
            QFile iconFile(iconLocation);
            if (!file.exists() || !iconFile.exists())
            {
                qDebug() << title << "openwith item file does not exist";
                continue;
            }
        }

        // Don't include qView in open with menu
        if (title == "qView")
            continue;

        qDebug() << title << path << iconLocation;

        OpenWithItem openWithItem;
        openWithItem.name = title;
        openWithItem.exec = path;

        listOfOpenWithItems.append(openWithItem);
    }

    // Natural/alphabetic sort
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(listOfOpenWithItems.begin(),
              listOfOpenWithItems.end(),
              [&collator](const OpenWithItem &item0, const OpenWithItem &item1)
    {
            return collator.compare(item0.name, item1.name) < 0;
    });

    // add default program to the beginning after sorting
//    if (!defaultOpenWithItem.name.isEmpty())
//        listOfOpenWithItems.prepend(defaultOpenWithItem);

#else
    QString mimeName;
    if (!filePath.isEmpty())
    {
        QMimeDatabase mimedb;
        QMimeType mime = mimedb.mimeTypeForFile(filePath, QMimeDatabase::MatchContent);
        mimeName = mime.name();
    }

    // This should probably be async dude
    QProcess process;
    process.start("xdg-mime", {"query", "default", mimeName});
    process.waitForFinished();
    QString defaultApplication = process.readAllStandardOutput().trimmed();

    OpenWithItem defaultOpenWithItem;

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

            if (!fileInfo.fileName().endsWith(".desktop"))
                continue;

            OpenWithItem openWithItem;
            // additional info to hold
            QString mimeTypes;
            bool noDisplay = false;

            QFile file(fileInfo.absoluteFilePath());
            file.open(QIODevice::ReadOnly);
            QTextStream in(&file);
            QString line;
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
                else if (line.startsWith("Categories=", Qt::CaseInsensitive) && openWithItem.categories.isEmpty())
                {
                    line.remove("Categories=", Qt::CaseInsensitive);
                    openWithItem.categories = line.split(";");
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
                else if (line.startsWith("MimeType=", Qt::CaseInsensitive) && mimeTypes.isEmpty())
                {
                    line.remove("MimeType=", Qt::CaseInsensitive);
                    mimeTypes = line;
                }
                else if (line.startsWith("NoDisplay=", Qt::CaseInsensitive))
                {
                    line.remove("NoDisplay=");
                    if (!line.compare("true", Qt::CaseInsensitive))
                    {
                        noDisplay = true;
                    }
                }
                else if (line.startsWith("Hidden=", Qt::CaseInsensitive))
                {
                    line.remove("Hidden=");
                    if (!line.compare("true", Qt::CaseInsensitive))
                    {
                        noDisplay = true;
                    }
                }
            }
            if ((mimeTypes.contains(mimeName, Qt::CaseInsensitive) || mimeName.isEmpty()) && !noDisplay)
            {
                // If the program is the default program, save it to add to the beginning after sorting
                if (fileInfo.fileName() == defaultApplication)
                    defaultOpenWithItem = openWithItem;
                else
                    listOfOpenWithItems.append(openWithItem);
            }
        }
    }

    // Natural/alphabetic sort
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(listOfOpenWithItems.begin(),
              listOfOpenWithItems.end(),
              [&collator](const OpenWithItem &item0, const OpenWithItem &item1)
    {
            return collator.compare(item0.name, item1.name) < 0;
    });

    // add default program to the beginning after sorting
    if (!defaultOpenWithItem.name.isEmpty())
        listOfOpenWithItems.prepend(defaultOpenWithItem);


#endif
    return listOfOpenWithItems;
}

void OpenWith::showOpenWithDialog(QWidget *parent)
{
#ifdef Q_OS_MACOS
    auto openWithDialog = new QFileDialog(parent);
    openWithDialog->setNameFilters({QT_TR_NOOP("All Applications (*.app)")});
    openWithDialog->setDirectory("/Applications");
    openWithDialog->open();
#elif defined Q_OS_WIN
    auto openWithDialog = new QFileDialog(parent);
    openWithDialog->setWindowTitle("Open with...");
    openWithDialog->setNameFilters({QT_TR_NOOP("Programs (*.exe *.pif *.com *.bat *.cmd)"), QT_TR_NOOP("All Files (*)")});
    openWithDialog->setDirectory(QProcessEnvironment::systemEnvironment().value("PROGRAMFILES", "C:\\"));
    openWithDialog->open();
#else
    auto openWithDialog = new QVOpenWithDialog(parent);
    openWithDialog->open();
#endif
}

// OpenWithDialog (for linux)
QVOpenWithDialog::QVOpenWithDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVOpenWithDialog)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint | Qt::CustomizeWindowHint));

    connect(this, &QDialog::accepted, this, &QVOpenWithDialog::triggeredOpen);
    connect(ui->treeView, &QTreeView::doubleClicked, this, &QVOpenWithDialog::triggeredOpen);

    model = new QStandardItemModel();

    ui->treeView->setModel(model);
    populateTreeView();
}

void QVOpenWithDialog::populateTreeView()
{
    auto listOfAllApps = OpenWith::getOpenWithItems("");

    for (const auto &category : categories)
    {
        auto *categoryItem = new QStandardItem(QIcon::fromTheme(category.iconName), category.readableName);

        QMutableListIterator<OpenWith::OpenWithItem> i(listOfAllApps);
        while (i.hasNext()) {
            auto app = i.next();
            if (app.categories.contains(category.name, Qt::CaseInsensitive) || category.name.isEmpty())
            {
                auto *item = new QStandardItem(app.icon, app.name);
                item->setData(app.exec, Qt::UserRole);
                categoryItem->setChild(categoryItem->rowCount(), item);
                i.remove();
            }
        }
        if (categoryItem->rowCount())
            model->appendRow(categoryItem);
    }
}

void QVOpenWithDialog::triggeredOpen()
{
    if (auto *window = qobject_cast<MainWindow*>(parent()))
    {
        auto selectedIndexes = ui->treeView->selectionModel()->selectedIndexes();
        if (selectedIndexes.isEmpty())
            return;

        QString exec = selectedIndexes.first().data(Qt::UserRole).toString();
        window->openWith(exec);
    }
}

QVOpenWithDialog::~QVOpenWithDialog()
{
    delete ui;
}

