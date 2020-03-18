#ifndef MENUBUILDER_H
#define MENUBUILDER_H

#include <QObject>
#include <QMenuBar>
#include <QHash>
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

    QAction *getAction(QString key) const;

    QMenuBar *buildMenuBar() const;

    QMenu *buildGifMenu() const;

    QMenu *buildViewMenu(bool withFullscreen = true) const;

    QMenu *buildToolsMenu() const;

    QMenu *buildHelpMenu() const;

    void loadRecentsList();

    void saveRecentsList();

    void addFileToRecentsList(QFileInfo file);

    void auditRecentsList();

    void clearRecentsList();

    void updateRecentsMenu();

    void initializeRecentsMenu();

    void initializeActionLibrary();

    void initializeShortcutsList();

    void updateShortcuts();

    void loadSettings();

    QMenu *getRecentsMenu() const { return recentsMenu; }

    const QList<SShortcut> &getShortcutsList() const { return shortcutsList; }

    const QList<SRecent> &getRecentsList() const { return recentsList; }

signals:

private:
    QList<SRecent> recentsList;

    QList<QAction*> recentsActionList;

    QHash<QString, QAction*> actionLibrary;

    QList<SShortcut> shortcutsList;

    QMenu *recentsMenu;

    bool isSaveRecentsEnabled;
};

#endif // MENUBUILDER_H
