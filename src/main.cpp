#include "mainwindow.h"
#include "qvapplication.h"
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QObject>
#include <QMenu>

int main(int argc, char *argv[])
{
    QVApplication a(argc, argv);
    a.setOrganizationName("qView");
    a.setApplicationName(QString("qView"));
    a.setApplicationVersion(QString::number(VERSION));
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QObject::tr("file"), QObject::tr("The file to open."));
    parser.process(a);

    MainWindow w;
    w.show();

    if (!parser.positionalArguments().isEmpty())
        w.openFile(parser.positionalArguments().first());

    return a.exec();
}
