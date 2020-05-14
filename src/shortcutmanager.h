#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <QObject>
#include <QKeySequence>

class ShortcutManager : public QObject
{
    Q_OBJECT
public:
    struct SShortcut {
        QString readableName;
        QString name;
        QStringList defaultShortcuts;
        QStringList shortcuts;
    };

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

    explicit ShortcutManager(QObject *parent = nullptr);

    void updateShortcuts();

    const QList<SShortcut> &getShortcutsList() const { return shortcutsList; }

signals:
    void shortcutsUpdated();

protected:
    void initializeShortcutsList();

private:
    QList<SShortcut> shortcutsList;
};

#endif // SHORTCUTMANAGER_H
