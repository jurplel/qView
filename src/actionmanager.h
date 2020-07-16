#ifndef MENUBUILDER_H
#define MENUBUILDER_H

#include "mainwindow.h"

#include <QMenuBar>
#include <QMultiHash>

class ActionManager : public QObject
{
    Q_OBJECT
public:
    struct SRecent {
        QString fileName;
        QString filePath;

        bool operator==(const SRecent other) const { return (fileName == other.fileName && filePath == other.filePath);}

        operator QString() const { return "SRecent(" + fileName + ", " + filePath + ")"; }
    };

    static QList<QAction*> getAllNestedActions(const QList<QAction*> &givenActionList)
    {
        QList<QAction*> totalActionList;
        for (const auto &action : givenActionList)
        {
            if (action->isSeparator())
                continue;

            if (action->menu())
                totalActionList.append(getAllNestedActions(action->menu()->actions()));

            totalActionList << action;
        }
        return totalActionList;
    }

    static QList<QAction*> getAllActionsOfType(const QList<QAction*> &givenActionList, const QString &type)
    {
        QList<QAction*> allActionsOfType;

        auto allNestedActions = getAllNestedActions(givenActionList);
        for (const auto &action : allNestedActions)
        {
            if (action->data() == type)
                allActionsOfType << action;
        }

        return allActionsOfType;
    }

    static QVariantList recentsListToVariantList(const QList<SRecent> &recentsList)
    {
        QVariantList variantList;
        for (const auto &recent : recentsList)
        {
            QStringList stringList = {recent.fileName, recent.filePath};
            variantList.append(QVariant(stringList));
        }
        return variantList;
    }

    static QList<SRecent> variantListToRecentsList(const QVariantList &variantList)
    {
        QList<SRecent> recentsList;
        for (const auto &variant : variantList)
        {
            auto stringList = variant.toStringList();
            recentsList.append({stringList.value(0), stringList.value(1)});
        }
        return recentsList;
    }

    explicit ActionManager(QObject *parent = nullptr);

    void settingsUpdated();

    QAction *cloneAction(const QString &key);

    QAction *getAction(const QString &key) const;

    QList<QAction*> getAllInstancesOfAction(const QString &key) const;

    QList<QAction*> getAllInstancesOfAction(const QString &key, QWidget *parent) const;

    void untrackClonedActions(const QList<QAction*> &actions);

    void untrackClonedActions(const QMenu *menu);

    void untrackClonedActions(const QMenuBar *menuBar);

    QMenuBar *buildMenuBar(QWidget *parent = nullptr);

    QMenu *buildViewMenu(bool addIcon = true, QWidget *parent = nullptr);

    QMenu *buildToolsMenu(bool addIcon = true, QWidget *parent = nullptr);

    QMenu *buildHelpMenu(bool addIcon = true, QWidget *parent = nullptr);

    QMenu *buildRecentsMenu(bool includeClearAction = true, QWidget *parent = nullptr);

    void loadRecentsList();

    void saveRecentsList();

    void addFileToRecentsList(const QFileInfo &file);

    void auditRecentsList();

    void clearRecentsList();

    void updateRecentsMenu();

    static void actionTriggered(QAction *triggeredAction);

    static void actionTriggered(QAction *triggeredAction, MainWindow *relevantWindow);

    const QList<SRecent> &getRecentsList() const { return recentsList; }

    const QHash<QString, QAction*> &getActionLibrary() const { return actionLibrary; }

    int getRecentsListMaxLength() const { return recentsListMaxLength; };

signals:
    void recentsMenuUpdated();

protected:
    void initializeActionLibrary();

private:
    QHash<QString, QAction*> actionLibrary;

    QMultiHash<QString, QAction*> actionCloneLibrary;

    QMultiHash<QString, QMenu*> menuCloneLibrary;

    QMenu *windowMenu;

    QList<SRecent> recentsList;

    QTimer *recentsSaveTimer;

    // Settings
    bool isSaveRecentsEnabled;
    int recentsListMaxLength;
};

#endif // MENUBUILDER_H
