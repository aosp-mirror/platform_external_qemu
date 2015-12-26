#pragma once

#include "android/skin/qt/shortcut-key-store.h"
#include "android/skin/qt/qt-ui-commands.h"
#include <QWidget>
#include "ui_help-page.h"

class HelpPage : public QWidget
{
    Q_OBJECT

public:
    explicit HelpPage(QWidget *parent = 0);
    void initializeKeyboardShortcutList(const ShortcutKeyStore<QtUICommand>* key_store);

private slots:
    void on_help_docs_clicked();
    void on_help_fileBug_clicked();
    void on_help_sendFeedback_clicked();

private:
    std::unique_ptr<Ui::HelpPage> mUi;
};

class LatestVersionLoadTask : public QObject {
    Q_OBJECT

public slots:
    void run();

signals:
    void finished(QString version);
};
