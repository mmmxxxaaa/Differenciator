#include "tree_base.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

TreeErrorType TreeCtor(Tree* tree)
{
    if (tree == NULL)
        return TREE_ERROR_NULL_PTR;

    tree->root = NULL;
    tree->size = 0;
    tree->file_buffer = NULL;

    return TREE_ERROR_NO;
}

TreeErrorType TreeDtor(Tree* tree)
{
    if (tree == NULL)
        return TREE_ERROR_NULL_PTR;

    TreeDestroyWithDataRecursive(tree->root);

    if (tree->file_buffer != NULL)
    {
        free(tree->file_buffer);
        tree->file_buffer = NULL;
    }

    tree->root = NULL;
    tree->size = 0;

    return TREE_ERROR_NO;
}

TreeErrorType TreeDestroyWithDataRecursive(Node* node)
{
    if (node == NULL)
        return TREE_ERROR_NO;

    TreeDestroyWithDataRecursive(node->left);
    TreeDestroyWithDataRecursive(node->right);

    if (node->type == NODE_VAR && node->data.var_definition.name != NULL)
        free(node->data.var_definition.name);

    free(node);
    return TREE_ERROR_NO;
}

bool IsLeaf(Node* node)
{
    if (node == NULL)
        return false;

    return (node->left == NULL && node->right == NULL);
}

//СИГМА СКИБИДИ
// функция для создания узла из токена
Node* CreateNodeFromToken(const char* token, Node* parent)
{
    Node* node = (Node*)calloc(1, sizeof(Node));
    if (node == NULL)
        return NULL;

    node->parent = parent;

    if (strcmp(token, "+") == 0)
    {
        node->type = NODE_OP;
        node->data.op_value = OP_ADD;
    }

    else if (strcmp(token, "-") == 0)
    {
        node->type = NODE_OP;
        node->data.op_value = OP_SUB;
    }
    else if (strcmp(token, "*") == 0)
    {
        node->type = NODE_OP;
        node->data.op_value = OP_MUL;
    }
    else if (strcmp(token, "/") == 0)
    {
        node->type = NODE_OP;
        node->data.op_value = OP_DIV;
    }
    else if (strcmp(token, "sin") == 0)
    {
        node->type = NODE_OP;
        node->data.op_value = OP_SIN;
    }
    else if (strcmp(token, "cos") == 0)
    {
        node->type = NODE_OP;
        node->data.op_value = OP_COS;
    }
    else if (isdigit(token[0]) || (token[0] == '-' && isdigit(token[1])))
    {
        node->type = NODE_NUM;
        node->data.num_value = atof(token);
    }
    else
    {
        node->type = NODE_VAR;
        node->data.var_definition.hash = ComputeHash(token);
        node->data.var_definition.name = strdup(token); //СИГМА СКИБИДИ
        if (node->data.var_definition.name == NULL)
        {
            free(node);
            return NULL;
        }
    }

    node->left = NULL;
    node->right = NULL;

    return node;
}

unsigned int ComputeHash(const char* str) //djb2
{
    unsigned int hash = 5381;
    int c = 0;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + (unsigned char)c; //умножаем на 33 без умножения //FIXME в конст
    return hash;
}
