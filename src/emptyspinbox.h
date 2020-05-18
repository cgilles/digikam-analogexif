/*
    Copyright (C) 2010 C-41 Bytes <contact@c41bytes.com>

    This file is part of AnalogExif.

    AnalogExif is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    AnalogExif is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with AnalogExif.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _EMPTYSPINBOX_H_
#define _EMPTYSPINBOX_H_

#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>

class EmptyQSpinBox : public QSpinBox 
{
    Q_OBJECT

public:

    EmptyQSpinBox(QWidget* parent) : QSpinBox(parent) { }

    void fixup(QString & input) const
    {
        if(input != "")
            QSpinBox::fixup(input);
    }

    bool isEmpty() const
    {
        return (text() == "");
    }

};

class EmptyQDoubleSpinBox : public QDoubleSpinBox 
{
    Q_OBJECT

public:

    EmptyQDoubleSpinBox(QWidget* parent) : QDoubleSpinBox(parent) { }

    void fixup(QString & input) const
    {
        if(input != "")
            QDoubleSpinBox::fixup(input);
    }

    bool isEmpty() const
    {
        return (text() == "");
    }

};

#endif // _EMPTYSPINBOX_H_