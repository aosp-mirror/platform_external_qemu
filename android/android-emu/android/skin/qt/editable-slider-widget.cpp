#include "android/skin/qt/editable-slider-widget.h"

#include <math.h>          // for M_LN10
#include <qcoreevent.h>    // for QEvent (ptr only), QEvent::FocusIn, QEvent...
#include <qmath.h>         // for qCeil, qLn
#include <qnamespace.h>    // for operator|, AlignTop, AlignRight, AlignLeft
#include <qstring.h>       // for operator==
#include <qvalidator.h>    // for QDoubleValidator::StandardNotation, QValid...
#include <QEvent>          // for QEvent
#include <QLocale>         // for QLocale
#include <QSignalBlocker>  // for QSignalBlocker
#include <QStyle>          // for QStyle
#include <QValidator>      // for QValidator
#include <QVariant>        // for QVariant

class QObject;
class QWidget;

EditableSliderWidget::EditableSliderWidget(QWidget* parent)
    : QWidget(parent),
      mMinValueLabel(this),
      mMaxValueLabel(this),
      mSlider(Qt::Horizontal, this),
      mLineEdit(this),
      mLineEditValidator(&mLineEdit) {
    // Arrange the child widgets in a layout.
    mSliderLabelsLayout.setSpacing(0);
    mSliderLabelsLayout.addWidget(&mMinValueLabel);
    mSliderLabelsLayout.addWidget(&mMaxValueLabel);
    mAnnotatedSliderLayout.setSpacing(0);
    mAnnotatedSliderLayout.addWidget(&mSlider);
    mAnnotatedSliderLayout.addSpacing(3);
    mAnnotatedSliderLayout.addLayout(&mSliderLabelsLayout);
    mEditBoxLayout.setSpacing(0);
    mEditBoxLayout.addWidget(&mLineEdit);
    mEditBoxLayout.addStretch();
    mMainLayout.setSpacing(0);
    mMainLayout.setContentsMargins(0, 0, 0, 0);
    mMainLayout.addLayout(&mAnnotatedSliderLayout);
    mMainLayout.addSpacing(16);
    mMainLayout.addLayout(&mEditBoxLayout);
    setLayout(&mMainLayout);

    // Set initial values.
    setMinimum(static_cast<int>(mSlider.minimum() / mSteps));
    setMaximum(static_cast<int>(mSlider.maximum() / mSteps));
    setValue(static_cast<int>(mSlider.value() / mSteps));

    // Do some adjustments to child widgets.
    mSlider.setFocusPolicy(Qt::ClickFocus);
    mMinValueLabel.setAlignment(Qt::AlignLeft | Qt::AlignTop);
    mMaxValueLabel.setAlignment(Qt::AlignRight | Qt::AlignTop);
    mMinValueLabel.setProperty("ColorGroup", "SliderLabel");
    mMaxValueLabel.setProperty("ColorGroup", "SliderLabel");
    mLineEditValidator.setDecimals(1);
    mLineEditValidator.setNotation(QDoubleValidator::StandardNotation);
    mLineEditValidator.setLocale(QLocale::c());
    mLineEdit.setValidator(&mLineEditValidator);
    mLineEdit.setAlignment(Qt::AlignRight | Qt::AlignTop);
    mLineEdit.setMaximumWidth(60);
    mLineEdit.setTextMargins(0, 0, 0, 4);
    mLineEdit.setProperty("class", "EditableValue");
    mLineEdit.installEventFilter(this);

    // Handle child widgets' value changes in our own slots.
    connect(&mSlider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
    connect(&mSlider, SIGNAL(sliderPressed()), this, SIGNAL(sliderPressed()));
    connect(&mSlider, SIGNAL(sliderReleased()), this, SIGNAL(sliderReleased()));
    connect(&mLineEdit, SIGNAL(editingFinished()), this, SLOT(lineEditValueChanged()));
}

void EditableSliderWidget::setSteps(double steps) {
    mPrecision = qCeil(qLn(steps) / M_LN10);
    mSteps = steps;

    // The slider's value is a multiple of the steps, so changing the steps
    // the values has effectively changed.  Reset the range and value to rescale
    // the sliders to the new step value.
    setRange(mMinimum, mMaximum, false);
    setValue(mValue, false);
}

void EditableSliderWidget::setValue(double value, bool emit_signal) {
    // Clip the value to the allowed range.
    mValue =
        value < mMinimum ? mMinimum : (value > mMaximum ? mMaximum : value);

    // Update the values displayed by the slider and the edit box.
    {
        QSignalBlocker blocker(mSlider);
        mSlider.setValue(static_cast<int>(mValue * mSteps));
    }

    if (mLineEditFocused) {
        mValueChangeIgnored = true;
    } else {
        mLineEdit.setText(QString("%1").arg(mValue, 0, 'f', mPrecision, '0'));
    }
    if (emit_signal) {
        emit valueChanged(mValue);
        emit valueChanged();
    }

    updateValidatorStyle("");
}

void EditableSliderWidget::setMinimum(double minimum, bool emit_signal) {
    mMinimum = minimum;
    mLineEditValidator.setBottom(mMinimum);
    setValue(mValue,
             emit_signal);  // Force the current value into the new bounds.
    mMinValueLabel.setText(QString::number(mMinimum));
    {
        QSignalBlocker blocker(mSlider);
        mSlider.setMinimum(static_cast<int>(mMinimum * mSteps));
    }
}

void EditableSliderWidget::setMaximum(double maximum, bool emit_signal) {
    mMaximum = maximum;
    mLineEditValidator.setTop(mMaximum);
    setValue(mValue,
             emit_signal);  // Force the current value into the new bounds.
    mMaxValueLabel.setText(QString::number(mMaximum));
    {
        QSignalBlocker blocker(mSlider);
        mSlider.setMaximum(static_cast<int>(mMaximum * mSteps));
    }
}

void EditableSliderWidget::sliderValueChanged(int new_value) {
    setValue(static_cast<double>(new_value) / mSteps);
}

void EditableSliderWidget::lineEditValueChanged() {
    QString new_value = mLineEdit.text();
    setValue(new_value.toDouble());
    mValueChangeIgnored = false;
}

bool EditableSliderWidget::eventFilter(QObject*, QEvent* e) {
    // This function can be called by the destructor, hence we check null here.
    if (!mLineEdit.validator()) return false;

    if (e->type() == QEvent::FocusIn) {
        // If the edit box was previously highlighted due to an
        // invalid value, un-highlight it.
        updateValidatorStyle("");
        mValueChangeIgnored = false;
        mLineEditFocused = true;
        mLineEditOriginalValue = mLineEdit.text();
    } else if (e->type() == QEvent::FocusOut) {
        mLineEditFocused = false;

        // When the edit box loses focus, highlight it
        // if it contains an invalid value.
        int dummy;
        QString text = mLineEdit.text();
        if (mLineEdit.validator()->validate(text, dummy) != QValidator::Acceptable) {
            updateValidatorStyle("InvalidInput");
        } else if (mLineEdit.text() == mLineEditOriginalValue && mValueChangeIgnored) {
            setValue(mValue);
        }
    }
    return false;
}

void EditableSliderWidget::updateValidatorStyle(QString colorGroup) {
    if (mLineEdit.property("ColorGroup") != colorGroup) {
        mLineEdit.setProperty("ColorGroup", colorGroup);
        mLineEdit.style()->unpolish(&mLineEdit);
        mLineEdit.style()->polish(&mLineEdit);
    }
}

void EditableSliderWidget::setLabelHidden() {
    mMinValueLabel.hide();
    mMaxValueLabel.hide();
}
