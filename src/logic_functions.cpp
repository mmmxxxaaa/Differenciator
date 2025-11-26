#include "logic_functions.h"
#include <math.h>

bool is_zero(double number)
{
    return fabs(number) < 1e-10;
}

bool is_one(double number)
{
    return fabs(number - 1) < 1e-10;
}

