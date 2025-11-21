#include "logic_functions.h"
#include <math.h>

bool is_zero(double number)
{
    return fabs(number) < 1e-10;
}
