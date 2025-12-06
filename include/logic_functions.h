#ifndef LOGIC_FUNCTIONS_H_
#define LOGIC_FUNCTIONS_H_

#include <stdbool.h>
#include "tree_common.h"

bool is_zero     (double number);
bool is_one      (double number);
bool is_minus_one(double number);
bool is_unary    (OperationType op);
bool is_binary   (OperationType op);
bool IsNodeType(Node* node, NodeType type);
bool IsNodeOp(Node* node, OperationType op_type);



#endif // LOGIC_FUNCTIONS_H_
