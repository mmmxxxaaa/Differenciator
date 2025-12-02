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

bool is_minus_one(double number)
{
    return fabs(number + 1) < 1e-10;
}

bool is_unary(OperationType op)
{
    return (op == OP_SIN || op == OP_COS || op == OP_LN || op == OP_EXP);
}

bool is_binary(OperationType op)
{
    return (op == OP_ADD || op == OP_SUB || op == OP_MUL || op == OP_DIV || op == OP_POW);
}
