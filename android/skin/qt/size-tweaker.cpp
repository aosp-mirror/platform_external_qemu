#include "android/skin/qt/size-tweaker.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QScreen>
#include <QStyle>
#include <QTableWidget>

#include <limits>
#include <sstream>

const qreal SizeTweaker::BaselineDpi = 96.0;

SizeTweaker::SizeTweaker(QWidget* widget) : 
        mSubject(widget),
        mCurrentScaleFactor(1.0, 1.0) {
#ifndef Q_OS_MAC
    if (mSubject) {
        mSubject->installEventFilter(this);
    }
#endif
}

bool SizeTweaker::eventFilter(QObject* o, QEvent* event) {
    if (event->type() == QEvent::Show || event->type() == QEvent::ScreenChangeInternal) {
        adjustSizesAndPositions();
    }
    return QObject::eventFilter(o, event);
}

static QString getTweakedStylesheet(const QVector2D& scale_factor) {
    // KLUDGE: We have customized several controls via QSS.
    // Their stylesheets need to be adjusted to properly react to DPI
    // changes.

    // Unfortunately, there is no way to read those baseline values from the
    // stylesheet without actually parsing it., so we hardcode them here.
    static const int custom_radiobtn_baseline_size = 24;
    static const int custom_checkbox_baseline_width = 36;
    static const int custom_checkbox_baseline_height = 24;
    static const int custom_headerview_baseline_padding = 16;

    std::ostringstream custom_ctrl_stylesheet;
    custom_ctrl_stylesheet << "\nQRadioButton::indicator { "
                           << "width: " << custom_radiobtn_baseline_size * scale_factor.x() << "; "
                           << "height: " << custom_radiobtn_baseline_size * scale_factor.y() << ";"
                           << "}\n"
                           << "QCheckBox::indicator { "
                           << "width: " << custom_checkbox_baseline_width * scale_factor.x() << "; "
                           << "height: " << custom_checkbox_baseline_height * scale_factor.y() << ";"
                           << "}\n"
                           << "QComboBox::down-arrow { "
                           << "width: " << custom_radiobtn_baseline_size * scale_factor.x() << "; "
                           << "height: " << custom_radiobtn_baseline_size * scale_factor.y() << ";"
                           << "}\n"
                           << "QHeaderView::section { padding: "
                           << custom_headerview_baseline_padding * scale_factor.y() << "; "
                           << "}\n";
    return QString::fromStdString(custom_ctrl_stylesheet.str());
}

static void scaleWidgetBy(QWidget* widget, const QVector2D& scale_by) {
    // First, calculate the expected new size (using QWidget::width and
    // QWidget::height after fiddling with min/max sizes is unpredictable
    // since changing min/max sizes may actually resize the widget).
    QVector2D new_size {widget->width() * scale_by.x(),
                        widget->height() * scale_by.y()};
    // For size-limited widgets, set new limits. The check for QWIDGETSIZE_MAX
    // is to avoid potential LOTS of logspam.
    widget->setMinimumWidth(widget->minimumWidth() * scale_by.x());
    widget->setMinimumHeight(widget->minimumHeight() * scale_by.y());
    int new_max_width = widget->maximumWidth() * scale_by.x();
    if (new_max_width < QWIDGETSIZE_MAX) {
        widget->setMaximumWidth(widget->maximumWidth() * scale_by.x());
    }
    int new_max_height = widget->maximumHeight() * scale_by.y();
    if (new_max_height < QWIDGETSIZE_MAX) {
        widget->setMaximumHeight(widget->maximumHeight() * scale_by.y());
    }
    // Set the new size of the widget!
    widget->resize(new_size.x(), new_size.y());

    // Fonts need to be adjusted too.
    if (widget->font().pixelSize() != -1) {
        QFont f = widget->font();
        f.setPixelSize(widget->font().pixelSize() * scale_by.y());
        widget->setFont(f);
    } else if (widget->font().pointSize() != -1) {
        QFont f = widget->font();
        f.setPointSize(widget->font().pointSize() * scale_by.y());
        widget->setFont(f);
    }
}

void SizeTweaker::adjustSizesAndPositions() {
#ifndef Q_OS_MAC
    if (!mSubject) {
        return;
    }

    int screen_number = QApplication::desktop()->screenNumber(mSubject.data());
    if (screen_number <= -1) {
        return;
    }
    QScreen* screen = QApplication::screens().at(screen_number);
    if (!screen) {
        return;
    }

    // Calculate the scale factors relative to baseline DPI.
    QVector2D scale_factor =
        QVector2D(screen->logicalDotsPerInchX() / BaselineDpi,
                screen->logicalDotsPerInchY() / BaselineDpi);

    // Calculate how much the size needs to be changed. For example,
    // if we're going from 192 DPI to 96 DPI, the sizes should be changed
    // by 0.5. If we're going from 96v to 192, sizes should be changed by
    // 2.0.
    QVector2D scale_by = scale_factor / mCurrentScaleFactor;

    if (fabs(scale_by.x() - 1.0) < std::numeric_limits<qreal>::epsilon() &&
        fabs(scale_by.y() - 1.0) < std::numeric_limits<qreal>::epsilon()) {
        // No DPI change, sizes don't need to be adjusted.
        return;
    }

    // Update scale factors.
    mCurrentScaleFactor = scale_factor;

    // Resize the widget itself.
    scaleWidgetBy(mSubject.data(), scale_by);

    // Generate a stylesheet tweaked for this DPI.
    QString tweaked_stylesheet = getTweakedStylesheet(scale_factor);

    // Scale and reposition child widgets.
    for (QWidget* w : mSubject->findChildren<QWidget*>()) {
        // Apply QCheckbox / QRadioButton styles if needed.
        if (dynamic_cast<QCheckBox*>(w) ||
            dynamic_cast<QRadioButton*>(w) ||
            dynamic_cast<QComboBox*>(w) ||
            dynamic_cast<QTableWidget*>(w)) {
            w->setStyleSheet(tweaked_stylesheet);

            // Ensure new stylesheet is applied.
            w->style()->unpolish(w);
            w->style()->polish(w);
        }
        if (QAbstractButton* button = dynamic_cast<QAbstractButton*>(w)) {
            // Make sure to use large icons on buttons.
            button->setIconSize(QSize(button->iconSize().width() * scale_by.x(),
                                      button->iconSize().height() * scale_by.y()));
        }
        scaleWidgetBy(w, scale_by);
        w->move(w->x() * scale_by.x(), w->y() * scale_by.y());
    }
#endif
}

