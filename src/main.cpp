#include "mainwindow.h"
#include "qvapplication.h"
#include <QCommandLineParser>
#include <QObject>

int main(int argc, char *argv[])
{
    QVApplication a(argc, argv);
    a.setOrganizationName("qView");
    a.setApplicationName(QString("qView"));
    a.setApplicationVersion(QString::number(VERSION));

    MainWindow w;
    w.show();

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QObject::tr("file"), QObject::tr("The file to open."));
    parser.process(a);


    if (!parser.positionalArguments().isEmpty())
    {
        w.setJustLaunchedWithImage(true);
        w.openFile(parser.positionalArguments().first());
    }

    return a.exec();
}
