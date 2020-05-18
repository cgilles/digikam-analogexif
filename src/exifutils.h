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

#ifndef EXIFUTILS_H
#define EXIFUTILS_H

#include <QString>

class ExifUtils
{
public:
    static void doubleToFraction(double value, int* numerator, int* denom);
    static void doubleToDegrees(double val, int& deg, int& min, double& sec);
    static QString fancyPrintDouble(double val);
    static bool containsNonAscii(const QString& str);

    static const int DoublePrecision = 2;

private:
#ifdef __GNUC__
    // don't use clever double to fraction algorithm under gcc (bug 3035568)
    static constexpr double DoubleDenominator = 10000.0;
#else
    static double fractionPart(double value);
    static void getFraction(double value, int* num, int* denom, int depth);
#endif // __GNUC__
};

#endif // EXIFUTILS_H
