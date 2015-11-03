#include "android/skin/qt/angle-input-widget.h"
#include <QtMath>

AngleInputWidget::AngleInputWidget(QWidget* parent) :
    QWidget (parent),
    mDecimalValueEditor(this),
    mDegreesValueEditor(this),
    mMinutesValueEditor(this),
    mSecondsValueEditor(this),
    mDecimalValue(0.0),
    mCurrentInputMode(AngleInputWidget::InputMode::DECIMAL) {

    // Set up validators for the editors.
    mDecimalDegreeValidator.setDecimals(100);
    mDecimalDegreeValidator.setBottom(-180.0);
    mDecimalDegreeValidator.setTop(180.0);
    mIntegerDegreeValidator.setBottom(-180);
    mIntegerDegreeValidator.setTop(180);
    mMinSecValidator.setBottom(0);
    mMinSecValidator.setTop(60);

    // Set up the editors.
    mDecimalValueEditor.setValidator(&mDecimalDegreeValidator);
    mDecimalValueEditor.setProperty("class", "EditableValue");
    mDegreesValueEditor.setValidator(&mIntegerDegreeValidator);
    mDegreesValueEditor.setProperty("class", "EditableValue");
    mMinutesValueEditor.setValidator(&mMinSecValidator);
    mMinutesValueEditor.setProperty("class", "EditableValue");
    mSecondsValueEditor.setValidator(&mMinSecValidator);
    mSecondsValueEditor.setProperty("class", "EditableValue");
    
    // Lay out the subwidgets horizontally.
    mLayout.addWidget(&mDecimalValueEditor);
    mLayout.addWidget(&mDegreesValueEditor);
    mLayout.addWidget(&mMinutesValueEditor);
    mLayout.addWidget(&mSecondsValueEditor);
    setLayout(&mLayout);

    // Default input mode is "decimal".
    setInputMode(InputMode::DECIMAL);

    // Ensure the internal value is updated when changes are made in the editor subwidgets.
    connect(&mDecimalValueEditor, SIGNAL(editingFinished()), this, SLOT(updateValueFromDecimalInput()));
    connect(&mDegreesValueEditor, SIGNAL(editingFinished()), this, SLOT(updateValueFromSexagesimalInput()));
    connect(&mMinutesValueEditor, SIGNAL(editingFinished()), this, SLOT(updateValueFromSexagesimalInput()));
    connect(&mSecondsValueEditor, SIGNAL(editingFinished()), this, SLOT(updateValueFromSexagesimalInput()));
}

const int MINUTES_IN_DEGREE = 60;
const int SECONDS_IN_MINUTE = 60;
const int SECONDS_IN_DEGREE = MINUTES_IN_DEGREE * SECONDS_IN_MINUTE;

void AngleInputWidget::setInputMode(AngleInputWidget::InputMode mode) {
    mCurrentInputMode = mode;
    switch(mode) {
    case InputMode::DECIMAL:
        // Update the visible sub-widgets.
        mDecimalValueEditor.show();
        mDegreesValueEditor.hide();
        mMinutesValueEditor.hide();
        mSecondsValueEditor.hide();

        // Ensure the correct value is displayed in the editor.
        mDecimalValueEditor.setText(QString::number(mDecimalValue));
        break;

    case InputMode::SEXAGESIMAL:
        // Update the visible sub-widgets.
        mDecimalValueEditor.hide();
        mDegreesValueEditor.show();
        mMinutesValueEditor.show();
        mSecondsValueEditor.show();
        
        // Convert the decimal value to sexagesimal representation.
        int sign = qFabs(mDecimalValue) / mDecimalValue;
        int seconds = qFloor(qFabs(mDecimalValue) * SECONDS_IN_DEGREE);
        int degrees = sign * seconds / SECONDS_IN_DEGREE;
        seconds = seconds % SECONDS_IN_DEGREE;
        int minutes = seconds / SECONDS_IN_MINUTE;
        seconds = seconds % SECONDS_IN_MINUTE;
        
        // Ensure the correct value is displayed in the editor.
        mDegreesValueEditor.setText(QString::number(degrees));
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
    int seconds = mSecondsValueEditor.text().toInt();
    int sign = abs(degrees)/degrees;
    mDecimalValue =
        sign * (abs(degrees) +
                minutes / static_cast<double>(MINUTES_IN_DEGREE) +
                seconds / static_cast<double>(SECONDS_IN_DEGREE));

    emit(valueChanged(mDecimalValue));
}
