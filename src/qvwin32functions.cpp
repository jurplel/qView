#include "qvwin32functions.h"

#include "ShlObj_core.h"
#include "winuser.h"
#include "Objbase.h"
#include "appmodel.h"
#include "Shlwapi.h"

#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QCollator>
#include <QVersionNumber>
#include <QXmlStreamReader>
#include <QWindow>

#include <QDebug>

// All this win32 code is currently likely full of memory leaks (hooray!)
// It's also kinda slow and if I recall correctly is meant to be called on another thread.


QList<OpenWith::OpenWithItem> QVWin32Functions::getOpenWithItems(const QString &filePath)
{
    QList<OpenWith::OpenWithItem> listOfOpenWithItems;

    OpenWith::OpenWithItem defaultOpenWithItem;

    bool isWin10 = QVersionNumber::fromString(QSysInfo::kernelVersion()) >= QVersionNumber(10);

    QFileInfo info(filePath);
    QString extension = "." + info.suffix();

    // Get default program first
    WCHAR assocString[MAX_PATH];
    DWORD assocStringSize = MAX_PATH;
    AssocQueryStringW(0, ASSOCSTR_FRIENDLYAPPNAME, qUtf16Printable(extension), L"open", assocString, &assocStringSize);
    QString defaultProgramName = QString::fromWCharArray(assocString);
    qDebug() << "default!" << defaultProgramName;

    IEnumAssocHandlers *assocHandlers = 0;
    HRESULT result = SHAssocEnumHandlers(qUtf16Printable(extension), ASSOC_FILTER_RECOMMENDED, &assocHandlers);
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
        int iconIndex = 0;
        result = handlers[0].GetIconLocation(&icon, &iconIndex);
        if (!SUCCEEDED(result))
        {
            qDebug() << "win32 failed point 5";
            continue;
        }

        OpenWith::OpenWithItem openWithItem;
        openWithItem.name = QString::fromWCharArray(uiName);
        openWithItem.exec = QString::fromWCharArray(name);
        QString iconLocation = QString::fromWCharArray(icon);
        if (openWithItem.name == openWithItem.exec) // If it's either invalid or a windows store app
        {
            if (isWin10 && iconLocation.contains("ms-resource")) // If it's a windows store app
            {
                QString firstHalf = iconLocation.mid(0, iconLocation.indexOf("?"));
                QString packageFullName = firstHalf.remove(0, 2);

                // Set exec to application user model id so we can run it using native win32 functions
                openWithItem.exec = getAumid(packageFullName);
                openWithItem.isWindowsStore = true;

                WCHAR realIconPath[MAX_PATH];
                SHLoadIndirectString(icon, realIconPath, MAX_PATH, NULL);
                qDebug() << QString::fromWCharArray(realIconPath);
                openWithItem.icon = QIcon(QString::fromWCharArray(realIconPath));

            }
            else // skip if invalid
            {
                qDebug() << "Skipping" << openWithItem.name;
                continue;
            }
        }
        else
        {
            // Remove items that have a path that does not exist
            QFile file(openWithItem.exec);
            QFile iconFile(iconLocation);
            if (!file.exists() || !iconFile.exists())
            {
                qDebug() << openWithItem.name << "openwith item file does not exist";
                continue;
            }

            QFileIconProvider iconProvider;
            openWithItem.icon = iconProvider.icon(QFileInfo(iconLocation));
        }

        // Don't include qView in open with menu
        if (openWithItem.name == "qView")
            continue;

        // Replace ampersands with escaped ampersands for menu items
        openWithItem.name.replace("&", "&&");

        qDebug() << openWithItem.name << openWithItem.exec << iconLocation;

        if (openWithItem.name == defaultProgramName)
        {
            openWithItem.isDefault = true;
            defaultOpenWithItem = openWithItem;
        }
        else
        {
            listOfOpenWithItems.append(openWithItem);
        }
    }

    // Natural/alphabetic sort
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(listOfOpenWithItems.begin(),
              listOfOpenWithItems.end(),
              [&collator](const OpenWith::OpenWithItem &item0, const OpenWith::OpenWithItem &item1)
    {
            return collator.compare(item0.name, item1.name) < 0;
    });

    // add default program to the beginning after sorting
    if (!defaultOpenWithItem.name.isEmpty())
        listOfOpenWithItems.prepend(defaultOpenWithItem);

    return listOfOpenWithItems;
}



void QVWin32Functions::openWithAppx(const QString &filePath, const QString &amuid)
{
    const QString &nativeFilePath = QDir::toNativeSeparators(filePath);
    IShellItem *shellItem = 0;
    HRESULT result = SHCreateItemFromParsingName(qUtf16Printable(nativeFilePath), 0, IID_IShellItem, (void**)&shellItem);
    if (!SUCCEEDED(result))
        qDebug() << "win32 openwith failed point 1";

    IApplicationActivationManager *activationManager = 0;
    result = CoCreateInstance(CLSID_ApplicationActivationManager, 0, CLSCTX_INPROC_SERVER, IID_IApplicationActivationManager, (void**)&activationManager);
    if (!SUCCEEDED(result))
        qDebug() << "win32 openwith failed point 2";

    IShellItemArray *shellItemArray = 0;
    result = SHCreateShellItemArrayFromShellItem(shellItem, IID_IShellItemArray, (void**)&shellItemArray);
    if (!SUCCEEDED(result))
        qDebug() << "win32 openwith failed point 3";

    DWORD pid = 0;
    activationManager->ActivateForFile(qUtf16Printable(amuid), shellItemArray, L"Open", &pid);

}

QString QVWin32Functions::getAumid(const QString &packageFullName)
{
    QString packageFamilyName = packageFullName;
    packageFamilyName.remove(packageFullName.indexOf("_"), packageFullName.lastIndexOf("_")-packageFullName.indexOf("_"));
    PACKAGE_INFO_REFERENCE info = 0;
    OpenPackageInfoByFullName(qUtf16Printable(packageFullName), 0, &info);
    UINT32 bufferLen = 0;
    UINT32 bufferAmount = 0;
    LONG error = GetPackageInfo(info, PACKAGE_INFORMATION_BASIC, &bufferLen, 0, &bufferAmount);
    if (error)
        qDebug() << "win32 failed point 6" << error;

    BYTE *buffer = (BYTE*)malloc(bufferLen);
    error = GetPackageInfo(info, PACKAGE_INFORMATION_BASIC, &bufferLen, buffer, &bufferAmount);
    if (error)
        qDebug() << "win32 failed point 7" << error;

    auto *packageInfoArray = reinterpret_cast<PACKAGE_INFO*>(buffer);
    QString appDirectory = QString::fromWCharArray(packageInfoArray[0].path);
    QFile manifest(appDirectory + "/AppxManifest.xml");
    if (!manifest.exists())
        qDebug() << "manifest doesn't exist?!";

    if (!manifest.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << "cant open manifest" << manifest.errorString();
    }

    QString praid;
    QXmlStreamReader manifestReader(&manifest);
    if (manifestReader.readNextStartElement()) {
        if (manifestReader.name() == "Package") {
            while (manifestReader.readNextStartElement()) {
                if (manifestReader.name() == "Applications") {
                    while (manifestReader.readNextStartElement()) {
                        if (manifestReader.name() == "Application") {
                            praid = manifestReader.attributes().value("Id").toString();
                            break;
                        }
                        else
                            manifestReader.skipCurrentElement();
                    }
                }
                else
                    manifestReader.skipCurrentElement();
            }
        }
    }
    QString aumid = packageFamilyName + "!" + praid;
    return aumid;
}

void QVWin32Functions::showOpenWithDialog(const QString &filePath, const QWindow *parent)
{
    const QString &nativeFilePath = QDir::toNativeSeparators(filePath);
    const OPENASINFO info = {
        qUtf16Printable(nativeFilePath),
        0,
        OAIF_EXEC
    };
    HWND winId = reinterpret_cast<HWND>(parent->winId());
    HRESULT result = SHOpenWithDialog(winId, &info);
    if (!SUCCEEDED(result))
        qDebug() << "win32 openwithdialog failed point 1";
}
