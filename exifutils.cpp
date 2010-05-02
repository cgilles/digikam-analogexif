#include "exifutils.h"
#include <cmath>
#include <limits>

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

void ExifUtils::doubleToFraction(double value, int* numerator, int* denom)
{
	getFraction(abs(value), numerator, denom, 10);
	if(value < 0)
		*numerator = -*numerator;
}
