#ifndef OPENWITH_H
#define OPENWITH_H

#include <QIcon>
#include <QDialog>
#include <QAbstractButton>
#include <QStandardItemModel>

class OpenWith : public QObject
{
    Q_OBJECT
public:
    struct OpenWithItem {
        QIcon icon;
        QString iconName;
        QString name;
        QString exec;
        QStringList args;
        QStringList categories; // this is only used on linux
        bool isDefault = false;
        void *winAssocHandler = nullptr;
    };

    static const QList<OpenWithItem> getOpenWithItems(const QString &filePath);

    static void showOpenWithDialog(QWidget *parent);

    static void openWithExecutable(const QString &executablePath, const QString &filePath);

    static void openWithExecutable(const QString &executablePath, const QStringList &args, const QString &filePath);

    static void openWith(const QString &filePath, const OpenWithItem &openWithItem);

    static QList<OpenWithItem> getOpenWithItemsFromDesktopFiles(const QString &filePath);
};
Q_DECLARE_METATYPE(OpenWith::OpenWithItem);

namespace Ui {
class QVOpenWithDialog;
}

class QVOpenWithDialog : public QDialog
{
    Q_OBJECT

public:
    struct Category {
      QString name;
      QString readableName;
      QString iconName;
    };
    explicit QVOpenWithDialog(QWidget *parent = nullptr);

    void populateTreeView();

    void triggeredOpen();

    ~QVOpenWithDialog();

signals:
    void selected(const QString exec, const QStringList args);
private:
    Ui::QVOpenWithDialog *ui;

    QStandardItemModel *model;

    const QList<Category> categories = {{"Development", QT_TR_NOOP("Development"), "applications-development"},
                                        {"Education", QT_TR_NOOP("Education"), "applications-education"},
                                        {"Game", QT_TR_NOOP("Games"), "applications-games"},
                                        {"Graphics", QT_TR_NOOP("Graphics"), "applications-graphics"},
                                        {"Network", QT_TR_NOOP("Internet"), "applications-internet"},
                                        {"AudioVideo", QT_TR_NOOP("Multimedia"), "applications-multimedia"},
                                        {"Office", QT_TR_NOOP("Office"), "applications-office"},
                                        {"Science", QT_TR_NOOP("Science"), "applications-science"},
                                        {"Settings", QT_TR_NOOP("Settings"), "preferences-system"},
                                        {"System", QT_TR_NOOP("System"), "applications-system"},
                                        {"Utility", QT_TR_NOOP("Utilities"), "applications-utilities"},
                                        {"", QT_TR_NOOP("Other"), "applications-other"}};
};

#endif // OPENWITH_H
