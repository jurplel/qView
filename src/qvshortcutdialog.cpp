#include "qvshortcutdialog.h"
#include "ui_qvshortcutdialog.h"
#include "qvapplication.h"

#include <QMessageBox>

#include <QDebug>

QVShortcutDialog::QVShortcutDialog(int index, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QVShortcutDialog)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint | Qt::CustomizeWindowHint));

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &QVShortcutDialog::buttonBoxClicked);

    shortcutObject = qvApp->getShortcutManager().getShortcutsList().value(index);
    this->index = index;
    ui->keySequenceEdit->setKeySequence(shortcutObject.shortcuts.join(", "));
}

QVShortcutDialog::~QVShortcutDialog()
{
    delete ui;
}

void QVShortcutDialog::registerGetTransientShortcutCallback(const GetTransientShortcutCallback callback)
{
    getTransientShortcutCallback = callback;
}

void QVShortcutDialog::done(int r)
{
    if (r == QDialog::Accepted)
    {
        return;
    }

    QDialog::done(r);
}

void QVShortcutDialog::buttonBoxClicked(QAbstractButton *button)
{
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole)
    {
        QStringList shortcutsStringList = ui->keySequenceEdit->keySequence().toString().split(", ");
        const auto sequenceList = ShortcutManager::stringListToKeySequenceList(shortcutsStringList);

        for (const auto &sequence : sequenceList)
        {
            auto conflictingShortcut = shortcutAlreadyBound(sequence, shortcutObject.name);
            if (!conflictingShortcut.isEmpty())
            {
                QString nativeShortcutString = sequence.toString(QKeySequence::NativeText);
                QMessageBox::warning(this, tr("Shortcut Already Used"), tr("\"%1\" is already bound to \"%2\"").arg(nativeShortcutString, conflictingShortcut));
                return;
            }
        }

        acceptValidated();

        emit shortcutsListChanged(index, shortcutsStringList);
    }
    else if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole)
    {
        ui->keySequenceEdit->setKeySequence(shortcutObject.defaultShortcuts.join(", "));
    }
}

QString QVShortcutDialog::shortcutAlreadyBound(const QKeySequence &chosenSequence, const QString &exemptShortcut)
{
    if (chosenSequence.isEmpty())
        return "";

    const auto &shortcutsList = qvApp->getShortcutManager().getShortcutsList();
    for (int i = 0; i < shortcutsList.length(); i++)
    {
        const auto &shortcut = shortcutsList.value(i);
        const auto shortcuts = getTransientShortcutCallback ? getTransientShortcutCallback(i) : shortcut.shortcuts;
        const auto sequenceList = ShortcutManager::stringListToKeySequenceList(shortcuts);

        if (sequenceList.contains(chosenSequence) && shortcut.name != exemptShortcut)
            return shortcut.readableName;
    }
    return "";
}

void QVShortcutDialog::acceptValidated()
{
    QDialog::done(1);
}
