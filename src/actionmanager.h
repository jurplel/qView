#ifndef MENUBUILDER_H
#define MENUBUILDER_H

#include "mainwindow.h"

#include <QObject>
#include <QMenuBar>
#include <QMultiHash>
#include <QFileInfo>

class ActionManager : public QObject
{
    Q_OBJECT
public:
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

    static QStringList keyBindingsToStringList(QKeySequence::StandardKey sequence)
    {
        const auto seqList = QKeySequence::keyBindings(sequence);
        QStringList strings;
        for (const auto &seq : seqList)
        {
            strings << seq.toString();
        }
        return strings;
    }

    static QList<QKeySequence> stringListToKeySequenceList(const QStringList &stringList)
    {

        QList<QKeySequence> keySequences;
        for (const auto &string : stringList)
        {
            keySequences << QKeySequence::fromString(string);
        }
        return keySequences;
    }

    static QString stringListToReadableString(QStringList stringList)
    {
        return QKeySequence::fromString(stringList.join(", ")).toString(QKeySequence::NativeText);
    }

    static QStringList readableStringToStringList(QString shortcutString)
    {
        return QKeySequence::fromString(shortcutString, QKeySequence::NativeText).toString().split(", ");
    }


    struct SShortcut {
        QString readableName;
        QString name;
        QStringList defaultShortcuts;
        QStringList shortcuts;
    };


    struct SRecent {
        QString fileName;
        QString filePath;

        bool operator==(const SRecent other) const { return (fileName == other.fileName && filePath == other.filePath);}

        operator QString() const { return "SRecent(" + fileName + ", " + filePath + ")"; }
    };

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

    QAction *cloneAction(const QString &key);

    QAction *getAction(const QString &key) const;

    QList<QAction*> getCloneActions(const QString &key) const;

    QList<QAction*> getAllInstancesOfAction(const QString &key) const;

    void untrackClonedActions(const QList<QAction*> &actions);

    void untrackClonedActions(const QMenu *menu);

    void untrackClonedActions(const QMenuBar *menuBar);

    QMenuBar *buildMenuBar(QWidget *parent = nullptr);

    QMenu *buildGifMenu(QWidget *parent = nullptr);

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

    void actionTriggered(QAction *triggeredAction) const;

    void actionTriggered(QAction *triggeredAction, MainWindow *relevantWindow) const;

    void updateShortcuts();

    void loadSettings();

    const QList<SRecent> &getRecentsList() const { return recentsList; }

    const QHash<QString, QAction*> &getActionLibrary() const { return actionLibrary; }

    const QList<SShortcut> &getShortcutsList() const { return shortcutsList; }

signals:
    void recentsMenuUpdated();

protected:
    void initializeActionLibrary();

    void initializeShortcutsList();

private:
    QList<SRecent> recentsList;

    QHash<QString, QAction*> actionLibrary;

    QMultiHash<QString, QAction*> actionCloneLibrary;

    QMultiHash<QString, QMenu*> menuCloneLibrary;

    QList<SShortcut> shortcutsList;

    QMenu *windowMenu;

    QTimer *recentsSaveTimer;

    bool isSaveRecentsEnabled;

    int recentsListMaxLength;
};

#endif // MENUBUILDER_H
