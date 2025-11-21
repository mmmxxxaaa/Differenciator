#include "operations.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
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

//ХУЙНЯ у меня 2 функции разных для создания узла
Node* CreateNode(NodeType type, void* data, Node* left, Node* right) //СИГМА СКИБИДИ пизды за void* дадут
{
    Node* node = (Node*)calloc(1, sizeof(Node));
    if (!node)
        return NULL;

    node->type = type;
    node->left = left;
    node->right = right;
    node->parent = NULL;

    switch (type)
    {
        case NODE_NUM:
            if (data)
                node->data.num_value = *(double*)data;
            else
                node->data.num_value = 0.0;
            break;

        case NODE_VAR:
            if (data)
            {
                const char* name = (const char*)data;
                node->data.var_definition.name = strdup(name);
                node->data.var_definition.hash = ComputeHash(name);
            }
            break;

        case NODE_OP:
            if (data)
                node->data.op_value = *(OperationType*)data;
            break;

        default:
            break;
    }

    if (left)
        left->parent = node;
    if (right)
        right->parent = node;

    return node;
}

//FIXME мб это как-то через DSL
static Node* CreateNumberNode(double value)
{
    return CreateNode(NODE_NUM, &value, NULL, NULL);
}

static Node* CreateVariableNode(const char* name)
{
    return CreateNode(NODE_VAR, (void*)name, NULL, NULL);
}

static Node* CreateOperationNode(OperationType op, Node* left, Node* right)
{
    return CreateNode(NODE_OP, &op, left, right);
}

static Node* CopyNode(Node* original)
{
    if (original == NULL)
        return NULL;

    switch (original->type)
    {
        case NODE_NUM:
            return CreateNumberNode(original->data.num_value);

        case NODE_VAR:
            return CreateVariableNode(original->data.var_definition.name);

        case NODE_OP:
            return CreateOperationNode(original->data.op_value, CopyNode(original->left), CopyNode(original->right));
        default:
            return NULL;
    }
}

static Node* DifferentiateNode(Node* node, const char* variable_name)
{
    if (node == NULL)
        return NULL;
    //FIXME раскидать по функциям
    switch (node->type)
    {
        case NODE_NUM:
            return CreateNumberNode(0.0);

        case NODE_VAR:
            if (node->data.var_definition.name && strcmp(node->data.var_definition.name, variable_name) == 0)
            {
                // d(x)/dx = 1
                return CreateNumberNode(1.0);
            }
            else
            {
                // d(y)/dx = 0 (если y != x)
                return CreateNumberNode(0.0);
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
                    return CreateOperationNode(node->data.op_value, left_deriv, right_deriv);
                }

                case OP_MUL:
                {
                    // d(u*v)/dx = u*dv/dx + v*du/dx
                    Node* u = CopyNode(node->left);
                    Node* v = CopyNode(node->right);
                    Node* du_dx = DifferentiateNode(node->left, variable_name);
                    Node* dv_dx = DifferentiateNode(node->right, variable_name);

                    Node* term1 = CreateOperationNode(OP_MUL, u, dv_dx);
                    Node* term2 = CreateOperationNode(OP_MUL, v, du_dx);

                    return CreateOperationNode(OP_ADD, term1, term2);
                }

                case OP_DIV:
                {
                    // d(u/v)/dx = (v*du/dx - u*dv/dx) / (v^2)
                    Node* u = CopyNode(node->left);
                    Node* v = CopyNode(node->right);
                    Node* du_dx = DifferentiateNode(node->left, variable_name);
                    Node* dv_dx = DifferentiateNode(node->right, variable_name);

                    Node* numerator_term1 = CreateOperationNode(OP_MUL, v, du_dx);
                    Node* numerator_term2 = CreateOperationNode(OP_MUL, u, dv_dx);
                    Node* numerator = CreateOperationNode(OP_SUB, numerator_term1, numerator_term2);

                    Node* v_squared = CreateOperationNode(OP_MUL, CopyNode(v), CopyNode(v));

                    return CreateOperationNode(OP_DIV, numerator, v_squared);
                }

                case OP_SIN:
                {
                    // d(sin(u))/dx = cos(u) * du/dx
                    Node* u = CopyNode(node->right);
                    Node* du_dx = DifferentiateNode(node->right, variable_name);
                    Node* cos_u = CreateOperationNode(OP_COS, NULL, u);

                    return CreateOperationNode(OP_MUL, cos_u, du_dx);
                }

                case OP_COS:
                {
                    // d(cos(u))/dx = -sin(u) * du/dx
                    Node* u = CopyNode(node->right);
                    Node* du_dx = DifferentiateNode(node->right, variable_name);
                    Node* sin_u = CreateOperationNode(OP_SIN, NULL, u);
                    Node* minus_one = CreateNumberNode(-1.0);
                    Node* minus_sin_u = CreateOperationNode(OP_MUL, minus_one, sin_u);

                    return CreateOperationNode(OP_MUL, minus_sin_u, du_dx);
                }

                default:
                    return CreateNumberNode(0.0);
            }

        default:
            return CreateNumberNode(0.0);
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
