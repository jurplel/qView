#ifndef MENUBUILDER_H
#define MENUBUILDER_H

#include <QObject>
#include <QMenuBar>
#include <QHash>

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

    struct SShortcut {
        QString readableName;
        QString name;
        QStringList defaultShortcuts;
        QStringList shortcuts;
    };

    explicit ActionManager(QObject *parent = nullptr);

    QAction *getAction(QString key) const;

    QMenuBar *buildMenuBar() const;

    QMenu *buildGifMenu() const;

    QMenu *buildViewMenu(bool withFullscreen = true) const;

    QMenu *buildToolsMenu() const;

    QMenu *buildHelpMenu() const;

    void updateRecentsList();

    void clearRecentsList();

    void initializeRecentsMenu();

    void initializeRecentsList();

    void initializeActionLibrary();

    void initializeShortcutsList();

    void assignShortcuts();

    QMenu *getRecentsMenu() const { return recentsMenu; }

signals:

private:
    QList<QAction*> recentsList;

    QHash<QString, QAction*> actionLibrary; 

    QList<SShortcut> shortcutsList;

    QMenu *recentsMenu;
};

#endif // MENUBUILDER_H
