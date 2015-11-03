// Copyright (C) 2015 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QIntValidator>
#include <QWidget>

// This is a widget for specifying angle values.
// It has two input modes - decimal and sexagesimal.
// In decimal mode, it allows the user to specify a single
// decimal value in degrees, i.e. 10.5 degrees
// In sexagesimal mode, it allows the user to specify the value
// in terms of degrees, minutes and seconds, i.e. 10 degrees and
// 30 minutes.
class AngleInputWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double value READ value NOTIFY valueChanged USER true);

public:
    enum class InputMode {
        Decimal,
        Sexagesimal
    };

    explicit AngleInputWidget(QWidget* parent = nullptr);

    // Returns the decimal measure of the angle, in degrees.
    double value() const { return mDecimalValue; }

    // Indicates which input mode the widget is currently in.
    InputMode currentMode() const { return mCurrentInputMode; }

public slots:
    // Sets the widget's current input mode to either decimal or sexagesimal.
    void setInputMode(InputMode);

signals:
    // Emitted when the user changes the value.
    // Regardless of the current input mode, new_value will always be a decimal
    // value specified in degrees, i.e. 10.5 degrees.
    void valueChanged(double new_value);

private slots:
    // Updates the internal value based on the decimal editor.
    void updateValueFromDecimalInput();

    // Updates the internal value based on the sexagesimal editor.
    void updateValueFromSexagesimalInput();

private:
    QFrame mDecimalModeFrame;
    QFrame mSexagesimalModeFrame;
    QHBoxLayout mDecimalFrameLayout;
    QHBoxLayout mSexagesimalFrameLayout;
    QLineEdit mDecimalValueEditor;
    QLineEdit mDegreesValueEditor;
    QLineEdit mMinutesValueEditor;
    QLineEdit mSecondsValueEditor;
    QDoubleValidator mDecimalDegreeValidator;
    QIntValidator mIntegerDegreeValidator;
    QIntValidator mMinSecValidator;
    QHBoxLayout mLayout;
    double mDecimalValue;
    InputMode mCurrentInputMode;
};
