#ifndef TREE_BASE_H_
#define TREE_BASE_H_

#include <stdbool.h>
#include "tree_common.h"
#include "tree_error_types.h"

TreeErrorType TreeCtor(Tree* tree);
TreeErrorType TreeDtor(Tree* tree);
void FreeNode(Node* node);

bool IsLeaf(Node* node);

unsigned int ComputeHash(const char* str);

#endif // TREE_BASE_H_
