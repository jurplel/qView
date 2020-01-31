#ifndef MENUBUILDER_H
#define MENUBUILDER_H

#include <QObject>
#include <QMenuBar>
#include <QHash>

class ActionManager : public QObject
{
    Q_OBJECT
public:
    explicit ActionManager(QObject *parent = nullptr);

    QAction *getAction(QString key);

    QMenuBar *buildMenuBar();

    QMenu *buildGifMenu();

    void initializeActionLibrary();

signals:

private:
    QHash<QString, QAction*> actionLibrary;

};

#endif // MENUBUILDER_H
