#include "android/skin/qt/error-dialog.h"
#include <QErrorMessage>

void showErrorDialog(const QString& message, const QString& title)
{
    QErrorMessage *err = QErrorMessage::qtHandler();
    err->setWindowTitle(title);
    err->showMessage(message);
}