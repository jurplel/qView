#include "mainwindow.h"
#include "qvapplication.h"

#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setOrganizationName("qView");
    QCoreApplication::setApplicationName("qView");
    QCoreApplication::setApplicationVersion(QString::number(VERSION));
    QVApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QObject::tr("file"), QObject::tr("The file to open."));
    parser.process(app);

    auto *window = QVApplication::newWindow();

    if (!parser.positionalArguments().isEmpty())
        QVApplication::openFile(window, parser.positionalArguments().constFirst(), true);

    return QApplication::exec();
}
