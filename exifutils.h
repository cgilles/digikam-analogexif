#ifndef EXIFUTILS_H
#define EXIFUTILS_H

class ExifUtils
{
public:
	static void doubleToFraction(double value, int* numerator, int* denom);
private:
	static double fractionPart(double value);
	static void getFraction(double value, int* num, int* denom, int depth);
};

#endif // EXIFUTILS_H
