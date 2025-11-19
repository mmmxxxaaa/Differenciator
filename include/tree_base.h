#ifndef TREE_BASE_H_
#define TREE_BASE_H_

#include <stdbool.h>
#include "tree_common.h"
#include "tree_error_types.h"

TreeErrorType TreeCtor(Tree* tree);
TreeErrorType TreeDtor(Tree* tree);
Node* CreateNodeFromToken(const char* token, Node* parent); //FIXME

Node* CreateNode(ValueOfTreeElement value, Node* parent); //По сути уже не нужен is_dynamic + передавать тип (Op/Var/Num) + указатели на левое и правое поддерево
bool IsLeaf(Node* node);
TreeErrorType TreeDestroyWithDataRecursive(Node* node);
unsigned int ComputeHash(const char* str);

#endif // TREE_BASE_H_
