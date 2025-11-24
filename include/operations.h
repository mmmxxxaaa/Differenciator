#ifndef DIFF_OPERATIONS
#define DIFF_OPERATIONS

#include "tree_error_types.h"
#include "tree_common.h"
#include "variable_parse.h"

TreeErrorType EvaluateTree(Tree* tree, VariableTable* var_table, double* result);
TreeErrorType DifferentiateTree(Tree* tree, const char* variable_name, Tree* result_tree);
Node* CreateNode(NodeType type, ValueOfTreeElement data, Node* left, Node* right);
Node* CreateNodeFromToken(const char* token, Node* parent); //FIXME

#endif // DIFF_OPERATIONS
