// Copyright (C) 2015 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/angle-input-widget.h"
#include <QtMath>

// Helper function for AngleInputWidget ctor
static void setUpLineEdit(QLineEdit* editor, const QValidator* validator) {
    editor->setValidator(validator);
    editor->setProperty("class", "EditableValue");
}

AngleInputWidget::AngleInputWidget(QWidget* parent) :
    QWidget(parent),
    mDecimalModeFrame(this),
    mSexagesimalModeFrame(this),
    mDecimalValueEditor(&mDecimalModeFrame),
    mDegreesValueEditor(&mSexagesimalModeFrame),
    mMinutesValueEditor(&mSexagesimalModeFrame),
    mSecondsValueEditor(&mSexagesimalModeFrame),
    mDecimalValue(0.0),
    mCurrentInputMode(InputMode::Decimal) {

    // Set up validators for the editors.
    mDecimalDegreeValidator.setDecimals(100);
    mDecimalDegreeValidator.setBottom(-180.0);
    mDecimalDegreeValidator.setTop(180.0);
    mIntegerDegreeValidator.setBottom(-180);
    mIntegerDegreeValidator.setTop(180);
    mMinValidator.setBottom(0);
    mMinValidator.setTop(60);
    mSecValidator.setBottom(0.0);
    mSecValidator.setTop(60.0);

    // Set up the editors.
    setUpLineEdit(&mDecimalValueEditor, &mDecimalDegreeValidator);
    setUpLineEdit(&mDegreesValueEditor, &mIntegerDegreeValidator);
    setUpLineEdit(&mMinutesValueEditor, &mMinValidator);
    setUpLineEdit(&mSecondsValueEditor, &mSecValidator);

    // Lay out the subwidgets horizontally.
    mDecimalFrameLayout.addWidget(&mDecimalValueEditor);
    mDecimalModeFrame.setLayout(&mDecimalFrameLayout);
    mSexagesimalFrameLayout.addWidget(&mDegreesValueEditor);
    mSexagesimalFrameLayout.addWidget(&mMinutesValueEditor);
    mSexagesimalFrameLayout.addWidget(&mSecondsValueEditor);
    mSexagesimalModeFrame.setLayout(&mSexagesimalFrameLayout);
    mLayout.addWidget(&mDecimalModeFrame);
    mLayout.addWidget(&mSexagesimalModeFrame);
    setLayout(&mLayout);

    // Default input mode is "decimal".
    setInputMode(InputMode::Decimal);

    // Ensure the internal value is updated when changes are made in the editor subwidgets.
    connect(&mDecimalValueEditor, SIGNAL(editingFinished()),
            this, SLOT(updateValueFromDecimalInput()));
    connect(&mDegreesValueEditor, SIGNAL(editingFinished()),
            this, SLOT(updateValueFromSexagesimalInput()));
    connect(&mMinutesValueEditor, SIGNAL(editingFinished()),
            this, SLOT(updateValueFromSexagesimalInput()));
    connect(&mSecondsValueEditor, SIGNAL(editingFinished()),
            this, SLOT(updateValueFromSexagesimalInput()));

    // The line edit controls must not be squished by the spacing
    // introduced by additional box layouts. The spacing is removed
    // by setting margins to 0.
    setContentsMargins(0, 0, 0, 0);
    mDecimalModeFrame.setContentsMargins(0, 0, 0, 0);
    mSexagesimalModeFrame.setContentsMargins(0, 0, 0, 0);
    mSexagesimalFrameLayout.setContentsMargins(0, 0, 0, 0);
    mDecimalFrameLayout.setContentsMargins(0, 0, 0, 0);
    mLayout.setContentsMargins(0, 0, 0, 0);
}

const int MINUTES_IN_DEGREE = 60;
const int SECONDS_IN_MINUTE = 60;
const int SECONDS_IN_DEGREE = MINUTES_IN_DEGREE * SECONDS_IN_MINUTE;

static int sgn(double x) {
    return (x > 0.0) - (x < 0.0);
}

void AngleInputWidget::setInputMode(AngleInputWidget::InputMode mode) {
    mCurrentInputMode = mode;
    switch(mode) {
    case InputMode::Decimal:
        // Update the visible sub-widgets.
        mDecimalModeFrame.show();
        mSexagesimalModeFrame.hide();

        // Ensure the correct value is displayed in the editor.
        mDecimalValueEditor.setText(QString::number(mDecimalValue));
        break;

    case InputMode::Sexagesimal:
        // Update the visible sub-widgets.
        mDecimalModeFrame.hide();
        mSexagesimalModeFrame.show();

        // Convert the decimal value to sexagesimal representation.
        int sign = sgn(mDecimalValue);
        double seconds = qFabs(mDecimalValue) * SECONDS_IN_DEGREE;
        int whole_seconds = qFloor(seconds);
        double remainder_seconds = seconds - whole_seconds;

        int degrees = sign * whole_seconds / SECONDS_IN_DEGREE;
        whole_seconds = whole_seconds % SECONDS_IN_DEGREE;
        int minutes = whole_seconds / SECONDS_IN_MINUTE;
        seconds = whole_seconds % SECONDS_IN_MINUTE + remainder_seconds;

        // Ensure the correct value is displayed in the editor.
        mDegreesValueEditor.setText(
                QString(degrees == 0 && sign == -1 ? "-" : "") +
                QString::number(degrees));
        mMinutesValueEditor.setText(QString::number(minutes));
        mSecondsValueEditor.setText(QString::number(seconds));
        break;
    }
}

void AngleInputWidget::updateValueFromDecimalInput() {
    mDecimalValue = mDecimalValueEditor.text().toDouble();
    emit(valueChanged(mDecimalValue));
}

void AngleInputWidget::updateValueFromSexagesimalInput() {
    int degrees = mDegreesValueEditor.text().toInt();
    int minutes = mMinutesValueEditor.text().toInt();
    double seconds = mSecondsValueEditor.text().toDouble();
    int sign =
        degrees == 0 && mDegreesValueEditor.text()[0] == '-'
            ? -1
            : sgn(degrees);
    mDecimalValue =
        (sign == 0 ? 1 : sign) * (abs(degrees) +
                minutes / static_cast<double>(MINUTES_IN_DEGREE) +
                seconds / static_cast<double>(SECONDS_IN_DEGREE));

    emit(valueChanged(mDecimalValue));
}
