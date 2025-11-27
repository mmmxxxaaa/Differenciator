#ifndef DIFF_OPERATIONS
#define DIFF_OPERATIONS

#include <stdio.h>
#include "tree_error_types.h"
#include "tree_common.h"
#include "variable_parse.h"

size_t CountTreeNodes(Node* node);
TreeErrorType EvaluateTree(Tree* tree, VariableTable* var_table, double* result);
TreeErrorType DifferentiateTree(Tree* tree, const char* variable_name, Tree* result_tree);
Node* CreateNode(NodeType type, ValueOfTreeElement data, Node* left, Node* right);
Node* CreateNodeFromToken(const char* token, Node* parent);
TreeErrorType OptimizeTreeWithDump(Tree* tree, FILE* tex_file, VariableTable* var_table);

#endif // DIFF_OPERATIONS
