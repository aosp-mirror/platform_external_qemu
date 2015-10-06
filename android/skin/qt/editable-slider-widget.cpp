#include "android/skin/qt/editable-slider-widget.h"

EditableSliderWidget::EditableSliderWidget(QWidget *parent) :
    QWidget(parent),
    mMinValueLabel(this),
    mMaxValueLabel(this),
    mSlider(Qt::Horizontal, this),
    mLineEdit(this),
    mLineEditValidator(&mLineEdit) {

    // Arrange the child widgets in a layout.
    mSliderLabelsLayout.addWidget(&mMinValueLabel);
    mSliderLabelsLayout.addWidget(&mMaxValueLabel);
    mAnnotatedSliderLayout.setSpacing(0);
    mAnnotatedSliderLayout.addSpacing(4);
    mAnnotatedSliderLayout.addWidget(&mSlider);
    mAnnotatedSliderLayout.addLayout(&mSliderLabelsLayout);
    mEditBoxLayout.addWidget(&mLineEdit);
    mEditBoxLayout.addStretch();
    mMainLayout.addLayout(&mAnnotatedSliderLayout);
    mMainLayout.addSpacing(4);
    mMainLayout.addLayout(&mEditBoxLayout);
    setLayout(&mMainLayout);

    // Set initial values.
    setMinimum(static_cast<int>(mSlider.minimum()/10.0));
    setMaximum(static_cast<int>(mSlider.maximum()/10.0));
    setValue(static_cast<int>(mSlider.value()/10.0));

    // Do some adjustments to child widgets.
    mMinValueLabel.setAlignment(Qt::AlignLeft | Qt::AlignTop);
    mMaxValueLabel.setAlignment(Qt::AlignRight | Qt::AlignTop);
    mMinValueLabel.setProperty("ColorGroup", "SliderLabel");
    mMaxValueLabel.setProperty("ColorGroup", "SliderLabel");
    mLineEditValidator.setDecimals(1);
    mLineEdit.setValidator(&mLineEditValidator);
    mLineEdit.setAlignment(Qt::AlignLeft | Qt::AlignTop);
    mLineEdit.setMaximumWidth(64);
    mLineEdit.setTextMargins(0, 0, 0, 4);
    mLineEdit.setProperty("class", "EditableValue");

    // Handle child widgets' value changes in our own slots.
    connect(&mSlider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
    connect(&mLineEdit, SIGNAL(editingFinished()), this, SLOT(lineEditValueChanged()));
}

void EditableSliderWidget::setValue(double value) {
    // Clip the value to the allowed range.
    mValue =
        value < mMinimum ? mMinimum : (value > mMaximum ? mMaximum : value);

    // Update the values displayed by the slider and the edit box.
    mSlider.blockSignals(true);
    mSlider.setValue(static_cast<int>(mValue * 10.0));
    mSlider.blockSignals(false);
    mLineEdit.setText(QString::number(mValue));

    emit valueChanged(mValue);
    emit valueChanged();
}

void EditableSliderWidget::setMinimum(double minimum) {
    mMinimum = minimum;
    mLineEditValidator.setBottom(mMinimum);
    setValue(mValue); // Force the current value into the new bounds.
    mMinValueLabel.setText(QString::number(mMinimum));
    mSlider.setMinimum(static_cast<int>(mMinimum * 10.0));
}

void EditableSliderWidget::setMaximum(double maximum) {
    mMaximum = maximum;
    mLineEditValidator.setTop(mMaximum);
    setValue(mValue); // Force the current value into the new bounds.
    mMaxValueLabel.setText(QString::number(mMaximum));
    mSlider.setMaximum(static_cast<int>(mMaximum * 10.0));
}

void EditableSliderWidget::sliderValueChanged(int new_value) {
    setValue(static_cast<double>(new_value) / 10.0);
}

void EditableSliderWidget::lineEditValueChanged() {
    QString new_value = mLineEdit.text();
    setValue(new_value.toDouble());
}

