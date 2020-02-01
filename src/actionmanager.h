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

    QAction *getAction(QString key) const;

    QMenuBar *buildMenuBar() const;

    QMenu *buildGifMenu() const;

    QMenu *buildViewMenu(bool withFullscreen = true) const;

    QMenu *buildToolsMenu() const;

    QMenu *buildHelpMenu() const;

    QMenu *buildRecentsMenu() const;

    void updateRecentsList();

    void clearRecentsList();

    void initializeRecentsList();

    void initializeActionLibrary();

signals:

private:
    QHash<QString, QAction*> actionLibrary;

    QList<QAction*> recentsList;
};

#endif // MENUBUILDER_H
