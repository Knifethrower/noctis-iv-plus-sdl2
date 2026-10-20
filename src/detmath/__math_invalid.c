#include "libm.h"

double nm__math_invalid(double x)
{
	return (x - x) / (x - x);
}
