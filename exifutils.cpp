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

#include "exifutils.h"
#include <cmath>
#include <limits>

#ifndef __GNUC__
double ExifUtils::fractionPart(double value)
{
	return abs(abs(value) - (int)abs(value));
}

void ExifUtils::getFraction(double value, int* num, int* denom, int depth)
{
	double inverse = 1 / value;
	double fracPart = fractionPart(inverse);

	if((depth == 0)||(fracPart < 1e-10))
	{
		*num = 1;
		*denom = (int)inverse;
		return;
	}

	int num1, denom1;
	getFraction(fracPart, &num1, &denom1, depth - 1);

	*denom = (int)inverse * denom1 + num1;
	*num = denom1;
}
#endif

void ExifUtils::doubleToFraction(double value, int* numerator, int* denom)
{
#ifdef __GNUC__
	// don't use clever double to fraction algorithm under gcc (bug 3035568)
	*numerator = value * DoubleDenominator;
	*denom = DoubleDenominator;
#else
	getFraction(abs(value), numerator, denom, 10);
	if(value < 0)
		*numerator = -*numerator;
#endif
}

void ExifUtils::doubleToDegrees(double val, int& deg, int& min, double& sec)
{
	double dec, frac;

	frac = modf(val, &dec);

	deg = dec;

	frac = frac * 60.0;

	frac = modf(frac, &dec);

	min = dec;

	sec = frac * 60.0;
}

QString ExifUtils::fancyPrintDouble(double val)
{
	double dec;

	if(modf(val, &dec) == 0.0)
		return QString::number(val, 'f', 1);

	return QString::number(val, 'f', DoublePrecision);
}

bool ExifUtils::containsNonAscii(const QString& str)
{
	bool result = false;

	foreach(QChar c, str)
	{
		unsigned char chr = (unsigned char)c.toAscii();

		if((chr > 0x80) || (chr == 0x00))
		{
			// character is out of ASCII 7bit
			result = true;
			break;
		}
	}

	return result;
}