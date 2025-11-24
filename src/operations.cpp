#include "operations.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "tree_error_types.h"
#include "tree_common.h"
#include "variable_parse.h"
#include "logic_functions.h"
#include "tree_base.h"

static TreeErrorType EvaluateTreeRecursive(Node* node, VariableTable* var_table, double* result)
{
    if (node == NULL || result == NULL)
        return TREE_ERROR_NULL_PTR;

    switch (node->type)
    {
        case NODE_NUM:
            *result = node->data.num_value;
            return TREE_ERROR_NO;

        case NODE_OP:
            {
                double left_result = 0, right_result = 0;
                TreeErrorType error = TREE_ERROR_NO;

                // для всех операций проверяем правый аргумент
                if (node->right == NULL)
                {
                    return TREE_ERROR_NULL_PTR;
                }

                // для бинарных операций вычисляем оба аргумента
                if (node->data.op_value != OP_SIN && node->data.op_value != OP_COS)
                {
                    if (node->left == NULL)
                    {
                        return TREE_ERROR_NULL_PTR;
                    }
                    error = EvaluateTreeRecursive(node->left, var_table, &left_result);
                    if (error != TREE_ERROR_NO)
                        return error;
                }

                // для всех операций проверяем правый аргумент
                if (node->right == NULL)
                {
                    return TREE_ERROR_NULL_PTR;
                }

                error = EvaluateTreeRecursive(node->right, var_table, &right_result);
                if (error != TREE_ERROR_NO) return error;

                switch (node->data.op_value)
                {
                    case OP_ADD:
                        *result = left_result + right_result;
                        break;
                    case OP_SUB:
                        *result = left_result - right_result;
                        break;
                    case OP_MUL:
                        *result = left_result * right_result;
                        break;
                    case OP_DIV:
                        if (is_zero(right_result))
                            return TREE_ERROR_DIVISION_BY_ZERO;
                        *result = left_result / right_result;
                        break;
                    case OP_SIN:
                        *result = sin(right_result);
                        break;
                    case OP_COS:
                        *result = cos(right_result);
                        break;
                    default:
                        return TREE_ERROR_UNKNOWN_OPERATION;
                }
                return TREE_ERROR_NO;
            }

        case NODE_VAR: //FIXME в отдельную функцию
            {
                if (node->data.var_definition.name == NULL)
                    return TREE_ERROR_VARIABLE_NOT_FOUND;

                char* var_name = node->data.var_definition.name;
                double value = 0.0;
                TreeErrorType error = GetVariableValue(var_table, var_name, &value);

                if (error == TREE_ERROR_VARIABLE_NOT_FOUND) //
                {
                    // если переменная не найдена, то добавляем ее
                    error = AddVariable(var_table, var_name);
                    if (error != TREE_ERROR_NO)
                        return error;

                    // запрашиваем значение у пользователя
                    printf("Переменная '%s' не определена.\n", var_name);
                    error = RequestVariableValue(var_table, var_name);
                    if (error != TREE_ERROR_NO) return error;

                    // повторно получаем значение
                    error = GetVariableValue(var_table, var_name, &value);
                    if (error != TREE_ERROR_NO) return error;
                }
                else if (error == TREE_ERROR_VARIABLE_UNDEFINED)
                {
                    // переменная есть, но значение не определено
                    printf("Переменная '%s' не определена.\n", var_name);
                    error = RequestVariableValue(var_table, var_name);
                    if (error != TREE_ERROR_NO) return error;

                    error = GetVariableValue(var_table, var_name, &value);
                    if (error != TREE_ERROR_NO) return error;
                }
                else if (error != TREE_ERROR_NO)
                {
                    return error;
                }

                *result = value;
                return TREE_ERROR_NO;
            }

        default:
            return TREE_ERROR_UNKNOWN_OPERATION;
    }
}

TreeErrorType EvaluateTree(Tree* tree, VariableTable* var_table, double* result)
{
    if (tree == NULL || var_table == NULL || result == NULL)
        return TREE_ERROR_NULL_PTR;

    if (tree->root == NULL)
        return TREE_ERROR_NULL_PTR;

    return EvaluateTreeRecursive(tree->root, var_table, result);
}

Node* CreateNode(NodeType type, ValueOfTreeElement data, Node* left, Node* right) //СИГМА СКИБИДИ
{
    Node* node = (Node*)calloc(1, sizeof(Node));
    if (!node)
        return NULL;

    node->type = type;
    node->left = left;
    node->right = right;
    node->parent = NULL;

    node->data = data; //FIXME

    if (type == NODE_VAR && data.var_definition.name != NULL)
    {
        node->data.var_definition.name = data.var_definition.name;
        node->data.var_definition.hash = ComputeHash(data.var_definition.name);
    }

    if (left)
        left->parent = node;
    if (right)
        right->parent = node;

    return node;
}

static Node* CopyNode(Node* original)
{
    if (original == NULL)
        return NULL;

    ValueOfTreeElement data = {};

    switch (original->type)
    {
        case NODE_NUM:
            data.num_value = original->data.num_value;
            return CreateNode(NODE_NUM, data, NULL, NULL);

        case NODE_VAR:
            data.var_definition.hash = original->data.var_definition.hash;
            if (original->data.var_definition.name != NULL)
            {
                data.var_definition.name = strdup(original->data.var_definition.name);
            }
            else
            {
                data.var_definition.name = NULL;
            }            return CreateNode(NODE_VAR, data, NULL, NULL);

        case NODE_OP:
            data.op_value = original->data.op_value;
            return CreateNode(NODE_OP, data, CopyNode(original->left), CopyNode(original->right));
        default:
            return NULL;
    }
}

static Node* DifferentiateNode(Node* node, const char* variable_name)
{
    if (node == NULL)
        return NULL;

    ValueOfTreeElement data = {};
    //FIXME раскидать по функциям
    switch (node->type)
    {
        case NODE_NUM:
            data.num_value = 0.0;
            return CreateNode(NODE_NUM, data, NULL, NULL);

        case NODE_VAR:
            if (node->data.var_definition.name && strcmp(node->data.var_definition.name, variable_name) == 0)
            {
                // d(x)/dx = 1
                data.num_value = 1.0;

                return CreateNode(NODE_NUM, data, NULL, NULL);
            }
            else
            {
                // d(y)/dx = 0 (если y != x)
                data.num_value = 0.0;

                return CreateNode(NODE_NUM, data, NULL, NULL);
            }

        case NODE_OP:
            switch (node->data.op_value)
            {
                case OP_ADD:
                case OP_SUB:
                {
                    // d(u +- v)/dx = du/dx +- dv/dx
                    Node* left_deriv = DifferentiateNode(node->left, variable_name);
                    Node* right_deriv = DifferentiateNode(node->right, variable_name);
                    data.op_value = node->data.op_value;

                    return CreateNode(NODE_OP, data, left_deriv, right_deriv);
                }

                case OP_MUL:
                {
                    // d(u*v)/dx = u*dv/dx + v*du/dx
                    Node* u = CopyNode(node->left);
                    Node* v = CopyNode(node->right);
                    Node* du_dx = DifferentiateNode(node->left, variable_name);
                    Node* dv_dx = DifferentiateNode(node->right, variable_name);

                    data.op_value = OP_MUL;
                    Node* term1 = CreateNode(NODE_OP, data, u, dv_dx);
                    Node* term2 = CreateNode(NODE_OP, data, v, du_dx);

                    data.op_value = OP_ADD;

                    return CreateNode(NODE_OP, data, term1, term2);
                }

                case OP_DIV:
                {
                    // d(u/v)/dx = (v*du/dx - u*dv/dx) / (v^2)
                    Node* u = CopyNode(node->left);
                    Node* v = CopyNode(node->right);
                    Node* du_dx = DifferentiateNode(node->left, variable_name);
                    Node* dv_dx = DifferentiateNode(node->right, variable_name);

                    data.op_value = OP_MUL;
                    Node* numerator_term1 = CreateNode(NODE_OP, data, CopyNode(v), du_dx);
                    Node* numerator_term2 = CreateNode(NODE_OP, data, CopyNode(u), dv_dx);

                    data.op_value = OP_SUB;
                    Node* numerator = CreateNode(NODE_OP, data, numerator_term1, numerator_term2);

                    data.op_value = OP_MUL;
                    Node* v_squared = CreateNode(NODE_OP, data, CopyNode(v), CopyNode(v));

                    data.op_value = OP_DIV;

                    return CreateNode(NODE_OP, data, numerator, v_squared);
                }

                case OP_SIN:
                {
                    // d(sin(u))/dx = cos(u) * du/dx
                    Node* u = CopyNode(node->right);
                    Node* du_dx = DifferentiateNode(node->right, variable_name);

                    data.op_value = OP_COS;
                    Node* cos_u = CreateNode(NODE_OP, data, NULL, u);

                    data.op_value = OP_MUL;
                    return CreateNode(NODE_OP, data, cos_u, du_dx);
                }

                case OP_COS:
                {
                    // d(cos(u))/dx = -sin(u) * du/dx
                    Node* u = CopyNode(node->right);
                    Node* du_dx = DifferentiateNode(node->right, variable_name);

                    data.op_value = OP_SIN;
                    Node* sin_u = CreateNode(NODE_OP, data, NULL, u);

                    data.num_value = -1.0;
                    Node* minus_one = CreateNode(NODE_NUM, data, NULL, NULL);

                    data.op_value = OP_MUL;
                    Node* minus_sin_u = CreateNode(NODE_OP, data, minus_one, sin_u);

                    data.op_value = OP_MUL;
                    return CreateNode(NODE_OP, data, minus_sin_u, du_dx);
                }

                default:
                    data.num_value = 0.0;
                    return CreateNode(NODE_NUM, data, NULL, NULL);
            }

        default:
            data.num_value = 0.0;
            return CreateNode(NODE_NUM, data, NULL, NULL);
    }
}

static size_t CountTreeNodes(Node* node)
{
    if (node == NULL)
        return 0;

    return 1 + CountTreeNodes(node->left) + CountTreeNodes(node->right);
}

TreeErrorType DifferentiateTree(Tree* tree, const char* variable_name, Tree* result_tree)
{
    if (tree == NULL || variable_name == NULL || result_tree == NULL)
        return TREE_ERROR_NULL_PTR;

    if (tree->root == NULL)
        return TREE_ERROR_NULL_PTR;

    Node* derivative_root = DifferentiateNode(tree->root, variable_name);
    if (derivative_root == NULL)
        return TREE_ERROR_ALLOCATION;

    result_tree->root = derivative_root;
    result_tree->size = CountTreeNodes(derivative_root); //норм/хуйня? //FIXME

    return TREE_ERROR_NO;
}


// создавать переменные type и data
// выстраивать их
// после этого вызывать обычный CreateNode
Node* CreateNodeFromToken(const char* token, Node* parent)
{
    NodeType type = NODE_NUM; //FIXME надо инициализировать
    ValueOfTreeElement data = {0};

    static unsigned int op_hashes[6] = {0}; //FIXME
    static bool hashes_initialized = false;

    if (!hashes_initialized)
    {
        op_hashes[0] = ComputeHash("+");
        op_hashes[1] = ComputeHash("-");
        op_hashes[2] = ComputeHash("*");
        op_hashes[3] = ComputeHash("/");
        op_hashes[4] = ComputeHash("sin");
        op_hashes[5] = ComputeHash("cos");
        hashes_initialized = true;
    }

    unsigned int token_hash = ComputeHash(token);

    if (token_hash == op_hashes[0])
    {
        type = NODE_OP;
        data.op_value = OP_ADD;
    }
    else if (token_hash == op_hashes[1])
    {
        type = NODE_OP;
        data.op_value = OP_SUB;
    }
    else if (token_hash == op_hashes[2])
    {
        type = NODE_OP;
        data.op_value = OP_MUL;
    }
    else if (token_hash == op_hashes[3])
    {
        type = NODE_OP;
        data.op_value = OP_DIV;
    }
    else if (token_hash == op_hashes[4])
    {
        type = NODE_OP;
        data.op_value = OP_SIN;
    }
    else if (token_hash == op_hashes[5])
    {
        type = NODE_OP;
        data.op_value = OP_COS;
    }
    else if (isdigit(token[0]) || (token[0] == '-' && isdigit(token[1])))
    {
        type = NODE_NUM;
        data.num_value = atof(token);
    }
    else
    {
        type = NODE_VAR;
        data.var_definition.hash = token_hash;
        data.var_definition.name = strdup(token);
    }

    Node* node = CreateNode(type, data, NULL, NULL);
    if (node != NULL)
    {
        node->parent = parent;
    }
    else
    {
        if (type == NODE_VAR && data.var_definition.name != NULL)
            free(data.var_definition.name);
    }

    return node;
}
