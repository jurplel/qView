#include "mainwindow.h"
#include "qvapplication.h"
#include "qvwin32functions.h"

#include <QCommandLineParser>
#include <QTextStream>

QStringList expandStandardInputPaths(const QStringList &paths)
{
    QStringList expandedPaths;
    QTextStream inputStream(stdin, QIODevice::ReadOnly);
    QTextStream errorStream(stderr);

    for (const QString &path : paths) {
        if (path != "-") {
            expandedPaths.append(path);
            continue;
        }

        while (!inputStream.atEnd()) {
            const QString line = inputStream.readLine();
            if (!line.isEmpty())
                expandedPaths.append(line);
        }

        if (inputStream.status() != QTextStream::Ok)
            errorStream << "qView: Failed to read paths from standard input" << Qt::endl;
    }

    return expandedPaths;
}

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QCoreApplication::setOrganizationName("qView");
    QCoreApplication::setApplicationName("qView");
    QCoreApplication::setApplicationVersion(QString::number(VERSION));
    QVApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QObject::tr("file"),
                                 QObject::tr("The file(s) or directories to open. Use - to read "
                                             "line-delimited paths from standard input."),
                                 QObject::tr("[file ...]"));
#if defined Q_OS_WIN && WIN32_LOADED && QT_VERSION < QT_VERSION_CHECK(6, 7, 2)
    // Workaround for unicode characters getting mangled in certain cases. To support unicode
    // arguments on Windows, QCoreApplication normally ignores argv and gets them from the Windows
    // API instead. But this only happens if it thinks argv hasn't been modified prior to being
    // passed into QCoreApplication's constructor. Certain characters like U+2033 (double prime) get
    // converted differently in argv versus the value Qt is comparing with (__argv). This makes Qt
    // incorrectly think the data was changed, and it skips fetching unicode arguments from the API.
    // https://bugreports.qt.io/browse/QTBUG-125380
    parser.process(QVWin32Functions::getCommandLineArgs());
#else
    parser.process(app);
#endif

    auto *window = QVApplication::newWindow();
    const QStringList inputPaths = expandStandardInputPaths(parser.positionalArguments());
    if (!inputPaths.isEmpty())
        QVApplication::openFileSequence(window, inputPaths, true);

    return QApplication::exec();
}
