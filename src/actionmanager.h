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
    static QStringList keyBindingsToStringList(QKeySequence::StandardKey sequence)
    {
        auto seqList = QKeySequence::keyBindings(sequence);
        QStringList strings;
        foreach (QKeySequence seq, seqList)
        {
            strings << seq.toString();
        }
        return strings;
    }

    static QList<QKeySequence> stringListToKeySequenceList(QStringList stringList)
    {

        QList<QKeySequence> keySequences;
        foreach (QString string, stringList)
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

    static QVariantList recentsListToVariantList(QList<SRecent> recentsList)
    {
        QVariantList variantList;
        foreach (auto recent, recentsList)
        {
            QStringList stringList = {recent.fileName, recent.filePath};
            variantList.append(QVariant(stringList));
        }
        return variantList;
    }

    static QList<SRecent> variantListToRecentsList(QVariantList variantList)
    {
        QList<SRecent> recentsList;
        foreach (auto variant, variantList)
        {
            auto stringList = variant.value<QStringList>();
            recentsList.append({stringList.value(0), stringList.value(1)});
        }
        return recentsList;
    }

    explicit ActionManager(QObject *parent = nullptr);

    QAction *cloneAction(QString key);

    QAction *getAction(QString key) const;

    QList<QAction*> getCloneActions(QString key) const;

    QList<QAction*> getAllInstancesOfAction(QString key) const;

    QMenuBar *buildMenuBar(QWidget *parent = nullptr);

    QMenu *buildGifMenu(QWidget *parent = nullptr);

    QMenu *buildViewMenu(bool withFullscreen = true, QWidget *parent = nullptr);

    QMenu *buildToolsMenu(QWidget *parent = nullptr);

    QMenu *buildHelpMenu(QWidget *parent = nullptr);

    void loadRecentsList();

    void saveRecentsList();

    void addFileToRecentsList(QFileInfo file);

    void auditRecentsList();

    void clearRecentsList();

    void updateRecentsMenu();

    void actionTriggered(QAction *triggeredAction, bool useEmptyWindow = false) const;

    void actionTriggered(QAction *triggeredAction, MainWindow *relevantWindow) const;

    void updateShortcuts();

    void loadSettings();

    QMenu *getRecentsMenu() const { return recentsMenu; }

    const QList<SRecent> &getRecentsList() const { return recentsList; }

    const QList<QAction*> &getRecentsActionList() const { return recentsActionList; }

    const QHash<QString, QAction*> &getActionLibrary() const { return actionLibrary; }

    const QList<SShortcut> &getShortcutsList() const { return shortcutsList; }

signals:
    void recentsMenuUpdated();

protected:
    void initializeRecentsMenu();

    void initializeActionLibrary();

    void initializeShortcutsList();

private:
    QList<SRecent> recentsList;

    QList<QAction*> recentsActionList;

    QHash<QString, QAction*> actionLibrary;

    QMultiHash<QString, QAction*> actionCloneLibrary;

    QList<SShortcut> shortcutsList;

    QMenu *recentsMenu;

    QTimer *recentsSaveTimer;

    bool isSaveRecentsEnabled;
};

#endif // MENUBUILDER_H
