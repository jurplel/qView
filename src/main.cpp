#include "mainwindow.h"
#include "qvapplication.h"
#include <QCommandLineParser>
#include <QObject>

int main(int argc, char *argv[])
{
    QVApplication app(argc, argv);
    QCoreApplication::setOrganizationName("qView");
    QCoreApplication::setApplicationName(QString("qView"));
    QCoreApplication::setApplicationVersion(QString::number(VERSION));

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QObject::tr("file"), QObject::tr("The file to open."));
    parser.process(app);

    auto window = new MainWindow();
    if (!parser.positionalArguments().isEmpty())
    {
        window->setJustLaunchedWithImage(true);
        window->openFile(parser.positionalArguments().constFirst());
    }
    window->show();

    return QApplication::exec();
}
