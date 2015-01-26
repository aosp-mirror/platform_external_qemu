/********************************************************************************
** Form generated from reading UI file 'emulator-window.ui'
**
** Created by: Qt User Interface Compiler version 5.4.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EMULATOR_2D_WINDOW_H
#define UI_EMULATOR_2D_WINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>

QT_BEGIN_NAMESPACE

class Ui_EmulatorWindow
{
public:
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QFrame *uiFrame;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer_2;
    QPushButton *rotate;
    QSpacerItem *horizontalSpacer_3;

    void setupUi(QFrame *EmulatorWindow)
    {
        if (EmulatorWindow->objectName().isEmpty())
            EmulatorWindow->setObjectName(QStringLiteral("EmulatorWindow"));
        EmulatorWindow->resize(589, 503);
        horizontalLayout = new QHBoxLayout(EmulatorWindow);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        uiFrame = new QFrame(EmulatorWindow);
        uiFrame->setObjectName(QStringLiteral("uiFrame"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(uiFrame->sizePolicy().hasHeightForWidth());
        uiFrame->setSizePolicy(sizePolicy);
        uiFrame->setMinimumSize(QSize(200, 0));
        uiFrame->setMaximumSize(QSize(200, 16777215));
        uiFrame->setAutoFillBackground(true);
        uiFrame->setFrameShape(QFrame::StyledPanel);
        uiFrame->setFrameShadow(QFrame::Raised);
        horizontalLayout_2 = new QHBoxLayout(uiFrame);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);

        rotate = new QPushButton(uiFrame);
        rotate->setObjectName(QStringLiteral("rotate"));
        QIcon icon;
        icon.addFile(QStringLiteral(":/images/rotate-right.png"), QSize(), QIcon::Normal, QIcon::Off);
        rotate->setIcon(icon);

        horizontalLayout_2->addWidget(rotate);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_3);


        horizontalLayout->addWidget(uiFrame);


        retranslateUi(EmulatorWindow);

        QMetaObject::connectSlotsByName(EmulatorWindow);
    } // setupUi

    void retranslateUi(QFrame *EmulatorWindow)
    {
        EmulatorWindow->setWindowTitle(QApplication::translate("EmulatorWindow", "Form", 0));
        rotate->setText(QApplication::translate("EmulatorWindow", "Rotate", 0));
    } // retranslateUi

};

namespace Ui {
    class EmulatorWindow: public Ui_EmulatorWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EMULATOR_2D_WINDOW_H
