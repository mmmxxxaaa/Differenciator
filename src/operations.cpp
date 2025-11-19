#include "operations.h"
#include <stdio.h>
#include <math.h>
#include "tree_error_types.h"
#include "tree_common.h"
#include "variable_parse.h"

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
                        if (fabs(right_result) < 1e-10) //FIXME в функцию is_zero
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
