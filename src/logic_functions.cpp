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
    return (op == OP_SIN    || op == OP_COS    || op == OP_TAN    || op == OP_COT    ||
            op == OP_ARCSIN || op == OP_ARCCOS || op == OP_ARCTAN || op == OP_ARCCOT ||
            op == OP_SINH   || op == OP_COSH   || op == OP_TANH   || op == OP_COTH   ||
            op == OP_LN     || op == OP_EXP);
}

bool is_binary(OperationType op)
{
    return (op == OP_ADD || op == OP_SUB || op == OP_MUL ||
            op == OP_DIV || op == OP_POW);
}

bool IsNodeType(Node* node, NodeType type)
{
    return (node != NULL) && (node->type == type);
}

bool IsNodeOp(Node* node, OperationType op_type)
{
    return IsNodeType(node, NODE_OP) && (node->data.op_value == op_type);
}
