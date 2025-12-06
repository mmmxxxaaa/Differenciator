#include "tree_base.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

TreeErrorType TreeCtor(Tree* tree)
{
    if (tree == NULL)
        return TREE_ERROR_NULL_PTR;

    tree->root = NULL;
    tree->size = 0;
    tree->file_buffer = NULL;

    return TREE_ERROR_NO;
}

void FreeNode(Node* node)
{
    if (node == NULL)
        return;

    FreeNode(node->left);
    FreeNode(node->right);

    if (node -> type == NODE_VAR && node->data.var_definition.name != NULL)
    {
        free(node->data.var_definition.name);
        node->data.var_definition.name = NULL;
    }

    free(node);
}

TreeErrorType TreeDtor(Tree* tree)
{
    if (tree == NULL)
        return TREE_ERROR_NULL_PTR;

    FreeNode(tree->root);
    tree->root = NULL;
    tree->size = 0;

    if (tree->file_buffer != NULL)
    {
        free(tree->file_buffer);
        tree->file_buffer = NULL;
    }

    return TREE_ERROR_NO;
}


unsigned int ComputeHash(const char* str) //djb2
{
    unsigned int hash = 5381;
    int c = 0;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + (unsigned char)c; //умножаем на 33 без умножения
    return hash;
}
