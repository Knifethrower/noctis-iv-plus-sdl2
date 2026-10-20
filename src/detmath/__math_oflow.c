#include "libm.h"

double nm__math_oflow(uint32_t sign)
{
	return nm__math_xflow(sign, 0x1p769);
}
