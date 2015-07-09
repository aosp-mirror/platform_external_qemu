/********************************************************************************
** Form generated from reading UI file 'tool-window.ui'
**
** Created by: Qt User Interface Compiler version 5.4.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TOOL_2D_WINDOW_H
#define UI_TOOL_2D_WINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ToolWindow
{
public:
    QWidget *verticalLayoutWidget;
    QGridLayout *gridLayout;
    QToolButton *toolButton;
    QToolButton *toolButton_2;

    void setupUi(QFrame *ToolWindow)
    {
        if (ToolWindow->objectName().isEmpty())
            ToolWindow->setObjectName(QStringLiteral("ToolWindow"));
        ToolWindow->resize(37, 400);
        ToolWindow->setFrameShape(QFrame::StyledPanel);
        ToolWindow->setFrameShadow(QFrame::Raised);
        verticalLayoutWidget = new QWidget(ToolWindow);
        verticalLayoutWidget->setObjectName(QStringLiteral("verticalLayoutWidget"));
        verticalLayoutWidget->setGeometry(QRect(0, 0, 51, 116));
        gridLayout = new QGridLayout(verticalLayoutWidget);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);
        toolButton = new QToolButton(verticalLayoutWidget);
        toolButton->setObjectName(QStringLiteral("toolButton"));
        QIcon icon;
        icon.addFile(QStringLiteral(":/images/power.png"), QSize(), QIcon::Normal, QIcon::Off);
        toolButton->setIcon(icon);

        gridLayout->addWidget(toolButton, 0, 0, 1, 1);

        toolButton_2 = new QToolButton(verticalLayoutWidget);
        toolButton_2->setObjectName(QStringLiteral("toolButton_2"));
        QFont font;
        font.setPointSize(9);
        toolButton_2->setFont(font);

        gridLayout->addWidget(toolButton_2, 1, 0, 1, 1);


        retranslateUi(ToolWindow);

        QMetaObject::connectSlotsByName(ToolWindow);
    } // setupUi

    void retranslateUi(QFrame *ToolWindow)
    {
        ToolWindow->setWindowTitle(QApplication::translate("ToolWindow", "Frame", 0));
        toolButton->setText(QString());
        toolButton_2->setText(QApplication::translate("ToolWindow", "Rotate", 0));
    } // retranslateUi

};

namespace Ui {
    class ToolWindow: public Ui_ToolWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TOOL_2D_WINDOW_H
