#include "libm.h"

double nm__math_uflow(uint32_t sign)
{
	return nm__math_xflow(sign, 0x1p-767);
}
