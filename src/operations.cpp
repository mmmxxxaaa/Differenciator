#include "operations.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "assert.h"
#include "tree_error_types.h"
#include "tree_common.h"
#include "variable_parse.h"
#include "logic_functions.h"
#include "tree_base.h"
#include "latex_dump.h"
#include "dump.h"

//ДЕЛО СДЕЛАНО оглавление
//FIXME главы оформить
//FIXME график функции, производной и Тейлора
//FIXME касательная в точке

// ==================== DSL ДЛЯ СОЗДАНИЯ УЗЛОВ ====================
#define CREATE_NUM(value)          CreateNode(NODE_NUM, (ValueOfTreeElement){.num_value = (value)}, NULL, NULL)
#define CREATE_OP(op, left, right) CreateNode(NODE_OP,  (ValueOfTreeElement){.op_value = (op)}, (left), (right))
#define CREATE_UNARY_OP(op, right) CreateNode(NODE_OP,  (ValueOfTreeElement){.op_value = (op)}, NULL, (right))
#define CREATE_VAR(name)           CreateNode(NODE_VAR, (ValueOfTreeElement){.var_definition = (VariableDefinition){.name = strdup(name), .hash = ComputeHash(name)}}, NULL, NULL)
//DSL которая может копировать поддеревья и диффференцирование
#define CHECK_AND_CREATE(condition, creator) \
    ((condition) ? (creator) : (NULL))

#define RELEASE_IF_NULL(ptr, ...) \
    do { \
        if (!(ptr)) \
        {           \
            Node* nodes[] = {__VA_ARGS__}; \
            for (size_t i = 0; i < sizeof(nodes)/sizeof(nodes[0]); i++) \
                if (nodes[i]) FreeSubtree(nodes[i]); \
        } \
    } while(0)
// ===================================================================

// ==================== ПРОТОТИПЫ ФУНКЦИЙ ====================
static Node* DifferentiateAddSub(Node* node, const char* variable_name, OperationType op);
static Node* DifferentiateMul   (Node* node, const char* variable_name);
static Node* DifferentiateDiv   (Node* node, const char* variable_name);
static Node* DifferentiateSin   (Node* node, const char* variable_name);
static Node* DifferentiateCos   (Node* node, const char* variable_name);
static Node* DifferentiatePow   (Node* node, const char* variable_name);
static Node* DifferentiateLn    (Node* node, const char* variable_name);
static Node* DifferentiateExp   (Node* node, const char* variable_name);
static Node* DifferentiatePowerVarConst(Node* u, Node* v, Node* du_dx, Node* dv_dx);
static Node* DifferentiatePowerConstVar(Node* u, Node* v, Node* du_dx, Node* dv_dx);
static Node* DifferentiatePowerVarVar  (Node* u, Node* v, Node* du_dx, Node* dv_dx);
static Node* DifferentiateNode(Node* node, const char* variable_name);
static void  FreeNodes(int count, ...);
static bool  ContainsVariable(Node* node, const char* variable_name);
static void  ReplaceNode(Node** node_ptr, Node* new_node);


// ==================== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ====================

void FreeSubtree(Node* node)
{
    if (node == NULL)
        return;

    FreeSubtree(node->left);
    FreeSubtree(node->right);

    if (node->type == NODE_VAR && node->data.var_definition.name != NULL)
        free(node->data.var_definition.name);

    free(node);
}

static void FreeNodes(int count, ...)
{
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++)
    {
        Node* node = va_arg(args, Node*);
        if (node) FreeSubtree(node);
    }
    va_end(args);
}

static Node* CreateCheckedOp(OperationType op, Node* left, Node* right)
{
    Node* result = CREATE_OP(op, left, right);
    RELEASE_IF_NULL(result, left, right);

    return result;
}

static Node* CreateCheckedUnaryOp(OperationType op, Node* right)
{
    Node* result = CREATE_UNARY_OP(op, right);
    RELEASE_IF_NULL(result, right);

    return result;
}

static TreeErrorType EvaluateTreeRecursive(Node* node, VariableTable* var_table, double* result, int depth)
{
    if (node == NULL)
        return TREE_ERROR_NULL_PTR;

    if (result == NULL)
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

                if (node->right == NULL)
                    return TREE_ERROR_NULL_PTR;

                if (node->data.op_value != OP_SIN && node->data.op_value != OP_COS &&
                    node->data.op_value != OP_LN && node->data.op_value != OP_EXP)
                {
                    if (node->left == NULL)
                        return TREE_ERROR_NULL_PTR;

                    error = EvaluateTreeRecursive(node->left, var_table, &left_result, depth + 1);
                    if (error != TREE_ERROR_NO)
                        return error;
                }

                error = EvaluateTreeRecursive(node->right, var_table, &right_result, depth + 1);
                if (error != TREE_ERROR_NO)
                    return error;

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
                    case OP_POW:
                        *result = pow(left_result, right_result);
                        break;
                    case OP_LN:
                        if (right_result <= 0)
                            return TREE_ERROR_YCHI_MATAN;
                        *result = log(right_result);
                        break;
                    case OP_EXP:
                        *result = exp(right_result);
                        break;
                    default:
                        return TREE_ERROR_UNKNOWN_OPERATION;
                }
                return TREE_ERROR_NO;
            }

        case NODE_VAR:
            {
                if (node->data.var_definition.name == NULL)
                    return TREE_ERROR_VARIABLE_NOT_FOUND;

                char* var_name = node->data.var_definition.name;
                double value = 0.0;
                TreeErrorType error = GetVariableValue(var_table, var_name, &value);

                if (error == TREE_ERROR_VARIABLE_NOT_FOUND)
                {
                    error = AddVariable(var_table, var_name);
                    if (error != TREE_ERROR_NO)
                        return error;

                    error = RequestVariableValue(var_table, var_name);
                    if (error != TREE_ERROR_NO) return error;

                    error = GetVariableValue(var_table, var_name, &value);
                    if (error != TREE_ERROR_NO) return error;
                }
                else if (error == TREE_ERROR_VARIABLE_UNDEFINED)
                {
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

    return EvaluateTreeRecursive(tree->root, var_table, result, 0);
}

Node* CreateNode(NodeType type, ValueOfTreeElement data, Node* left, Node* right)
{
    Node* node = (Node*)calloc(1, sizeof(Node));
    if (!node)
        return NULL;

    node->type = type;
    node->left = left;
    node->right = right;
    node->parent = NULL;
    node->data = data;

    // Устанавливаем приоритет в зависимости от типа узла
    switch (type)
    {
        case NODE_NUM:
        case NODE_VAR:
            node->priority = 0;  // Наименьший приоритет для чисел и переменных
            break;

        case NODE_OP:
            switch (data.op_value)
            {
                case OP_ADD:
                case OP_SUB:
                    node->priority = 1;  // Низкий приоритет для сложения/вычитания
                    break;

                case OP_MUL:
                case OP_DIV:
                    node->priority = 2;  // Средний приоритет для умножения/деления
                    break;

                case OP_SIN:
                case OP_COS:
                case OP_LN:
                case OP_EXP:
                    node->priority = 3;  // Высокий приоритет для унарных операций
                    break;

                case OP_POW:
                    node->priority = 4;  // Наивысший приоритет для степени
                    break;

                default:
                    node->priority = 0;
                    break;
            }
            break;

        default:
            node->priority = 0;
            break;
    }

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
    Node* new_node = NULL;

    switch (original->type)
    {
        case NODE_NUM:
            data.num_value = original->data.num_value;
            new_node = CREATE_NUM(data.num_value);
            break;

        case NODE_VAR:
            data.var_definition.hash = original->data.var_definition.hash;
            if (original->data.var_definition.name != NULL)
            {
                data.var_definition.name = strdup(original->data.var_definition.name);
            }
            else
            {
                data.var_definition.name = NULL;
            }
            new_node = CreateNode(NODE_VAR, data, NULL, NULL);
            break;

        case NODE_OP:
            data.op_value = original->data.op_value;
            if (original->data.op_value == OP_SIN || original->data.op_value == OP_COS ||
                original->data.op_value == OP_LN || original->data.op_value == OP_EXP)
            {
                new_node =  CREATE_UNARY_OP(data.op_value, CopyNode(original->right));
            }
            else
            {
                new_node = CREATE_OP(data.op_value, CopyNode(original->left), CopyNode(original->right));
            }
            break;

        default:
            return NULL;
    }

    if (new_node != NULL)
        new_node->priority = original->priority;

    return new_node;
}

static bool ContainsVariable(Node* node, const char* variable_name)
{
    if (node == NULL)
        return false;

    unsigned int target_hash = ComputeHash(variable_name);

    switch (node->type)
    {
        case NODE_VAR:
            return (node->data.var_definition.hash == target_hash) &&
                   (node->data.var_definition.name != NULL)        &&
                   (strcmp(node->data.var_definition.name, variable_name) == 0);

        case NODE_OP:
            return ContainsVariable(node->left, variable_name) ||
                   ContainsVariable(node->right, variable_name);

        case NODE_NUM:
        default:
            return false;
    }
}

// ==================== ФУНКЦИИ ДИФФЕРЕНЦИРОВАНИЯ ====================

static Node* DifferentiateAddSub(Node* node, const char* variable_name, OperationType op)
{
    Node* left_deriv  = DifferentiateNode(node->left,  variable_name);
    Node* right_deriv = DifferentiateNode(node->right, variable_name);

    if (!left_deriv || !right_deriv)
    {
        FreeNodes(2, left_deriv, right_deriv);
        return NULL;
    }

    return CreateCheckedOp(op, left_deriv, right_deriv);
}

static Node* DifferentiateMul(Node* node, const char* variable_name)
{
    Node* u = CopyNode(node->left);
    Node* v = CopyNode(node->right);
    Node* du_dx = DifferentiateNode(node->left,  variable_name);
    Node* dv_dx = DifferentiateNode(node->right, variable_name);

    if (!u || !v || !du_dx || !dv_dx)
    {
        FreeNodes(4, u, v, du_dx, dv_dx);
        return NULL;
    }

    Node* term1 = CreateCheckedOp(OP_MUL, u, dv_dx);
    if (!term1)
    {
        FreeNodes(3, v, du_dx, dv_dx);
        return NULL;
    }

    Node* term2 = CreateCheckedOp(OP_MUL, v, du_dx);
    if (!term2)
    {
        FreeNodes(2, term1, du_dx);
        return NULL;
    }

    Node* result = CreateCheckedOp(OP_ADD, term1, term2);
    if (!result)
    {
        FreeNodes(2, term1, term2);
    }

    return result;
}

static Node* DifferentiateDiv(Node* node, const char* variable_name)
{
    Node* u = CopyNode(node->left);
    Node* v = CopyNode(node->right);
    Node* du_dx = DifferentiateNode(node->left, variable_name);
    Node* dv_dx = DifferentiateNode(node->right, variable_name);

    if (!u || !v || !du_dx || !dv_dx)
    {
        FreeNodes(4, u, v, du_dx, dv_dx);
        return NULL;
    }

    Node* numerator_term1 = CreateCheckedOp(OP_MUL, v, du_dx);
    if (!numerator_term1)
    {
        FreeNodes(4, u, v, du_dx, dv_dx);
        return NULL;
    }

    Node* numerator_term2 = CreateCheckedOp(OP_MUL, u, dv_dx);
    if (!numerator_term2)
    {
        FreeNodes(3, numerator_term1, u, dv_dx);
        return NULL;
    }

    Node* numerator = CreateCheckedOp(OP_SUB, numerator_term1, numerator_term2);
    if (!numerator)
    {
        FreeNodes(2, numerator_term1, numerator_term2);
        return NULL;
    }

    Node* v_copy1 = CopyNode(node->right);
    Node* v_copy2 = CopyNode(node->right);
    Node* v_squared = CreateCheckedOp(OP_MUL, v_copy1, v_copy2);
    if (!v_squared)
    {
        FreeNodes(3, numerator, v_copy1, v_copy2);
        return NULL;
    }

    Node* result = CreateCheckedOp(OP_DIV, numerator, v_squared);
    if (!result)
        FreeNodes(2, numerator, v_squared);

    return result;
}

static Node* DifferentiateSin(Node* node, const char* variable_name)
{
    Node* u = CopyNode(node->right);
    Node* du_dx = DifferentiateNode(node->right, variable_name);

    if (!u || !du_dx)
    {
        FreeNodes(2, u, du_dx);
        return NULL;
    }

    Node* cos_u = CreateCheckedUnaryOp(OP_COS, u);
    if (!cos_u)
    {
        FreeNodes(1, du_dx);
        return NULL;
    }

    Node* result = CreateCheckedOp(OP_MUL, cos_u, du_dx);
    if (!result)
    {
        FreeNodes(2, cos_u, du_dx);
    }

    return result;
}

static Node* DifferentiateCos(Node* node, const char* variable_name)
{
    Node* u = CopyNode(node->right);
    Node* du_dx = DifferentiateNode(node->right, variable_name);

    if (!u || !du_dx)
    {
        FreeNodes(2, u, du_dx);
        return NULL;
    }

    Node* sin_u = CreateCheckedUnaryOp(OP_SIN, u);
    if (!sin_u)
    {
        FreeNodes(1, du_dx);
        return NULL;
    }

    Node* minus_one = CREATE_NUM(-1.0);
    if (!minus_one)
    {
        FreeNodes(2, sin_u, du_dx);
        return NULL;
    }

    Node* minus_sin_u = CreateCheckedOp(OP_MUL, minus_one, sin_u);
    if (!minus_sin_u)
    {
        FreeNodes(3, minus_one, sin_u, du_dx);
        return NULL;
    }

    Node* result = CreateCheckedOp(OP_MUL, minus_sin_u, du_dx);
    if (!result)
        FreeNodes(2, minus_sin_u, du_dx);

    return result;
}

static Node* DifferentiatePowerVarConst(Node* u, Node* v, Node* du_dx, Node* dv_dx)
{
    Node* a_minus_one = CREATE_NUM(v->data.num_value - 1.0);
    if (!a_minus_one)
        return NULL;

    Node* u_pow_a_minus_one = CreateCheckedOp(OP_POW, u, a_minus_one);
    if (!u_pow_a_minus_one)
    {
        FreeNodes(1, a_minus_one);
        return NULL;
    }

    Node* a_times_pow = CreateCheckedOp(OP_MUL, v, u_pow_a_minus_one);
    if (!a_times_pow)
    {
        FreeNodes(1, u_pow_a_minus_one);
        return NULL;
    }

    Node* result = CreateCheckedOp(OP_MUL, a_times_pow, du_dx);
    if (!result)
        FreeNodes(1, a_times_pow);
    else
        FreeSubtree(dv_dx);

    return result;
}

static Node* DifferentiatePowerConstVar(Node* u, Node* v, Node* du_dx, Node* dv_dx)
{
    Node* a_pow_x = CreateCheckedOp(OP_POW, u, v);
    if (!a_pow_x)
        return NULL;

    Node* u_copy = CopyNode(u);
    if (!u_copy)
    {
        FreeNodes(1, a_pow_x);
        return NULL;
    }

    Node* ln_a = CreateCheckedUnaryOp(OP_LN, u_copy);
    if (!ln_a)
    {
        FreeNodes(2, a_pow_x, u_copy);
        return NULL;
    }

    Node* result = CreateCheckedOp(OP_MUL, a_pow_x, ln_a);
    if (!result)
        FreeNodes(2, a_pow_x, ln_a);
    else
        FreeNodes(2, du_dx, dv_dx);

    return result;
}

static Node* DifferentiatePowerVarVar(Node* u, Node* v, Node* du_dx, Node* dv_dx)
{
    Node* u_pow_v = CreateCheckedOp(OP_POW, u, v);
    if (!u_pow_v)
        return NULL;

    Node* u_copy_for_ln = CopyNode(u);
    Node* u_copy_for_div = CopyNode(u);
    Node* v_copy_for_div = CopyNode(v);

    if (!u_copy_for_ln || !u_copy_for_div || !v_copy_for_div)
    {
        FreeNodes(4, u_pow_v, u_copy_for_ln, u_copy_for_div, v_copy_for_div);
        return NULL;
    }

    Node* ln_u = CreateCheckedUnaryOp(OP_LN, u_copy_for_ln);
    if (!ln_u)
    {
        FreeNodes(4, u_pow_v, u_copy_for_ln, u_copy_for_div, v_copy_for_div);
        return NULL;
    }

    Node* dv_ln_u = CreateCheckedOp(OP_MUL, dv_dx, ln_u);
    if (!dv_ln_u)
    {
        FreeNodes(4, u_pow_v, u_copy_for_div, v_copy_for_div, ln_u);
        return NULL;
    }

    Node* v_div_u = CreateCheckedOp(OP_DIV, v_copy_for_div, u_copy_for_div);
    if (!v_div_u)
    {
        FreeNodes(3, u_pow_v, dv_ln_u, v_copy_for_div);
        return NULL;
    }

    Node* v_du_div_u = CreateCheckedOp(OP_MUL, v_div_u, du_dx);
    if (!v_du_div_u)
    {
        FreeNodes(3, u_pow_v, dv_ln_u, v_div_u);
        return NULL;
    }

    Node* bracket = CreateCheckedOp(OP_ADD, dv_ln_u, v_du_div_u);
    if (!bracket)
    {
        FreeNodes(3, u_pow_v, dv_ln_u, v_du_div_u);
        return NULL;
    }

    Node* result = CreateCheckedOp(OP_MUL, u_pow_v, bracket);
    if (!result)
        FreeNodes(2, u_pow_v, bracket);

    return result;
}
//FIXME DSL-ка уберет тела этих функций и все эти тела будут создаваться этими макросами в свитч-кейсе
static Node* DifferentiatePow(Node* node, const char* variable_name)
{
    Node* u = CopyNode(node->left);
    Node* v = CopyNode(node->right);
    Node* du_dx = DifferentiateNode(node->left, variable_name);
    Node* dv_dx = DifferentiateNode(node->right, variable_name);

    if (!u || !v || !du_dx || !dv_dx)
    {
        FreeNodes(4, u, v, du_dx, dv_dx);
        return NULL;
    }

    bool left_has_var = ContainsVariable(node->left, variable_name);
    bool right_has_var = ContainsVariable(node->right, variable_name);

    Node* result = NULL;

    if (left_has_var && !right_has_var)
        result = DifferentiatePowerVarConst(u, v, du_dx, dv_dx);
    else if (!left_has_var && right_has_var)
        result = DifferentiatePowerConstVar(u, v, du_dx, dv_dx);
    else
        result = DifferentiatePowerVarVar(u, v, du_dx, dv_dx);

    if (!result)
        FreeNodes(4, u, v, du_dx, dv_dx);

    return result;
}

static Node* DifferentiateLn(Node* node, const char* variable_name)
{
    Node* u = CopyNode(node->right);
    Node* du_dx = DifferentiateNode(node->right, variable_name);

    if (!u || !du_dx)
    {
        FreeNodes(2, u, du_dx);
        return NULL;
    }

    Node* one = CREATE_NUM(1.0);
    if (!one)
    {
        FreeNodes(2, u, du_dx);
        return NULL;
    }

    Node* one_div_u = CreateCheckedOp(OP_DIV, one, u);
    if (!one_div_u)
    {
        FreeNodes(3, one, u, du_dx);
        return NULL;
    }

    Node* result = CreateCheckedOp(OP_MUL, one_div_u, du_dx);
    if (!result)
        FreeNodes(2, one_div_u, du_dx);

    return result;
}

static Node* DifferentiateExp(Node* node, const char* variable_name)
{
    Node* u = CopyNode(node->right);
    Node* du_dx = DifferentiateNode(node->right, variable_name);

    if (!u || !du_dx)
    {
        FreeNodes(2, u, du_dx);
        return NULL;
    }

    Node* exp_u = CreateCheckedUnaryOp(OP_EXP, u);
    if (!exp_u)
    {
        FreeNodes(1, du_dx);
        return NULL;
    }

    Node* result = CreateCheckedOp(OP_MUL, exp_u, du_dx);
    if (!result)
        FreeNodes(2, exp_u, du_dx);

    return result;
}

static Node* DifferentiateNode(Node* node, const char* variable_name)
{
    if (node == NULL)
        return NULL;

    switch (node->type)
    {
        case NODE_NUM:
            return CREATE_NUM(0.0);

        case NODE_VAR:
            if (node->data.var_definition.name &&
                strcmp(node->data.var_definition.name, variable_name) == 0)
            {
                return CREATE_NUM(1.0);
            }
            else
            {
                return CREATE_NUM(0.0);
            }

        case NODE_OP:
            switch (node->data.op_value)
            {
                case OP_ADD:
                case OP_SUB:
                    return DifferentiateAddSub(node, variable_name, node->data.op_value);

                case OP_MUL:
                    return DifferentiateMul(node, variable_name);

                case OP_DIV:
                    return DifferentiateDiv(node, variable_name);

                case OP_SIN:
                    return DifferentiateSin(node, variable_name);

                case OP_COS:
                    return DifferentiateCos(node, variable_name);

                case OP_POW:
                    return DifferentiatePow(node, variable_name);

                case OP_LN:
                    return DifferentiateLn(node, variable_name);

                case OP_EXP:
                    return DifferentiateExp(node, variable_name);

                default:
                    return CREATE_NUM(0.0);
            }

        default:
            return CREATE_NUM(0.0);
    }
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
    result_tree->size = CountTreeNodes(derivative_root);

    return TREE_ERROR_NO;
}

// Node* CreateNodeFromToken(const char* token, Node* parent)
// {
//     static OperationInfo operations[] = {
//         {0, "+",   OP_ADD},
//         {0, "-",   OP_SUB},
//         {0, "*",   OP_MUL},
//         {0, "/",   OP_DIV},
//         {0, "sin", OP_SIN},
//         {0, "cos", OP_COS},
//         {0, "^",   OP_POW},
//         {0, "ln",  OP_LN},
//         {0, "exp", OP_EXP}
//     };
//
//     static size_t operations_count = sizeof(operations) / sizeof(operations[0]);
//     static bool hashes_initialized = false;
//
//     if (!hashes_initialized)
//     {
//         for (size_t i = 0; i < operations_count; i++)
//         {
//             operations[i].hash = ComputeHash(operations[i].name);
//         }
//         hashes_initialized = true;
//     }
//
//     unsigned int token_hash = ComputeHash(token);
//     Node* node = NULL;
//
//     for (size_t i = 0; i < operations_count; i++)
//     {
//         if (token_hash == operations[i].hash)
//         {
//             node = CREATE_OP(operations[i].op_value, NULL, NULL);
//             break;
//         }
//     }
//
//     if (node == NULL)
//     {
//         if (isdigit(token[0]) || (token[0] == '-' && isdigit(token[1])))
//         {
//             node = CREATE_NUM(atof(token));
//         }
//         else
//         {
//             ValueOfTreeElement data = {};
//             data.var_definition.hash = token_hash;
//             data.var_definition.name = strdup(token);
//             node = CreateNode(NODE_VAR, data, NULL, NULL);
//         }
//     }
//
//     if (node != NULL)
//         node->parent = parent;
//
//     return node;
// }

static void ReplaceNode(Node** node_ptr, Node* new_node)
{
    if (node_ptr == NULL || *node_ptr == NULL)
        return;

    Node* old_node = *node_ptr;
    *node_ptr = new_node;

    if (new_node != NULL)
        new_node->parent = old_node->parent;

    FreeSubtree(old_node);
}

// ==================== ФУНКЦИИ ОПТИМИЗАЦИИ С ДАМПОМ ====================

static TreeErrorType ConstantFoldingOptimizationWithDump(Node** node, FILE* tex_file, Tree* tree, VariableTable* var_table)
{
    if (node == NULL || *node == NULL)
        return TREE_ERROR_NULL_PTR;

    TreeErrorType error = TREE_ERROR_NO;

    if ((*node)->left != NULL)
    {
        error = ConstantFoldingOptimizationWithDump(&(*node)->left, tex_file, tree, var_table);
        if (error != TREE_ERROR_NO)
            return error;
    }

    if ((*node)->right != NULL)
    {
        error = ConstantFoldingOptimizationWithDump(&(*node)->right, tex_file, tree, var_table);
        if (error != TREE_ERROR_NO)
            return error;
    }

    if ((*node)->type == NODE_OP)
    {
        if (((*node)->data.op_value == OP_SIN || (*node)->data.op_value == OP_COS ||
             (*node)->data.op_value == OP_LN || (*node)->data.op_value == OP_EXP) &&
            (*node)->right != NULL && (*node)->right->type == NODE_NUM)
        {
            double result = 0.0;
            bool can_fold = true;

            switch ((*node)->data.op_value)
            {
                case OP_SIN:
                    result = sin((*node)->right->data.num_value);
                    break;
                case OP_COS:
                    result = cos((*node)->right->data.num_value);
                    break;
                case OP_LN:
                    if ((*node)->right->data.num_value <= 0)
                        can_fold = false;
                    else
                        result = log((*node)->right->data.num_value);
                    break;
                case OP_EXP:
                    result = exp((*node)->right->data.num_value);
                    break;
                default:
                    can_fold = false;
                    break;
            }

            if (can_fold)
            {
                Node* new_node = CREATE_NUM(result);
                if (new_node != NULL)
                {
                    ReplaceNode(node, new_node);

                    double new_result = 0.0;
                    if (EvaluateTree(tree, var_table, &new_result) == TREE_ERROR_NO && tex_file != NULL)
                    {
                        char description[kMaxTexDescriptionLength] = {0};
                        snprintf(description, sizeof(description),
                                "constant folding simplified part of expression to: %.2f", result);
                        DumpOptimizationStepToFile(tex_file, description, tree, new_result);
                    }
                }
            }
        }
        else if ((*node)->left  != NULL && (*node)->left->type  == NODE_NUM &&
                 (*node)->right != NULL && (*node)->right->type == NODE_NUM)
        {
            double left_val  = (*node)->left->data.num_value;
            double right_val = (*node)->right->data.num_value;
            double result = 0.0;
            bool can_fold = true;

            switch ((*node)->data.op_value)
            {
                case OP_ADD:
                    result = left_val + right_val;
                    break;
                case OP_SUB:
                    result = left_val - right_val;
                    break;
                case OP_MUL:
                    result = left_val * right_val;
                    break;
                case OP_DIV:
                    if (is_zero(right_val))
                        can_fold = false;
                    else
                        result = left_val / right_val;
                    break;
                default:
                    can_fold = false;
                    break;
            }

            if (can_fold)
            {
                Node* new_node = CREATE_NUM(result);
                if (new_node != NULL)
                {
                    ReplaceNode(node, new_node);

                    double new_result = 0.0;
                    if (EvaluateTree(tree, var_table, &new_result) == TREE_ERROR_NO && tex_file != NULL)
                    {
                        char description[kMaxTexDescriptionLength] = {0};
                        snprintf(description, sizeof(description),
                                "constant folding simplified part of expression to: %.2f", result);
                        DumpOptimizationStepToFile(tex_file, description, tree, new_result);
                        TreeDump(tree, "vova_hochet_glyanut.htm"); //ЭТО ИМЕННО РЕСПЕКТ
                    }
                }
            }
        }
    }
//FIXME создать структуру дифференциатора
    return TREE_ERROR_NO;
}
//FIXME дампить на уровень выше этих функций, description просто через аргументы передавать
static TreeErrorType NeutralElementsOptimizationWithDump(Node** node, FILE* tex_file, Tree* tree, VariableTable* var_table)
{
    if (node == NULL || *node == NULL)
        return TREE_ERROR_NULL_PTR;

    TreeErrorType error = TREE_ERROR_NO;

    if ((*node)->left != NULL)
    {
        error = NeutralElementsOptimizationWithDump(&(*node)->left, tex_file, tree, var_table);
        if (error != TREE_ERROR_NO)
            return error;
    }

    if ((*node)->right != NULL)
    {
        error = NeutralElementsOptimizationWithDump(&(*node)->right, tex_file, tree, var_table);
        if (error != TREE_ERROR_NO)
            return error;
    }

    if ((*node)->type == NODE_OP)
    {
        Node* new_node = NULL;
        const char* description = NULL;

        switch ((*node)->data.op_value)
        {
            case OP_ADD:
                if ((*node)->right != NULL && (*node)->right->type == NODE_NUM &&
                    is_zero((*node)->right->data.num_value))
                {
                    new_node = CopyNode((*node)->left);
                    description = "adding zero simplified";
                }
                else if ((*node)->left != NULL && (*node)->left->type == NODE_NUM &&
                         is_zero((*node)->left->data.num_value))
                {
                    new_node = CopyNode((*node)->right);
                    description = "adding zero simplified";
                }
                break;

            case OP_SUB: //FIXME  ДОБАВИТЬ Случай 0 - A => -A и для выражений: 0 - выражение => -1 * выражение
                if ((*node)->right != NULL && (*node)->right->type == NODE_NUM &&
                    is_zero((*node)->right->data.num_value))
                {
                    new_node = CopyNode((*node)->left);
                    description = "- 0 simplified";
                }
                break;

            case OP_MUL: //СИГМА СКИБИДИ
                if (((*node)->left != NULL && (*node)->left->type == NODE_NUM &&
                     is_zero((*node)->left->data.num_value)) ||
                    ((*node)->right != NULL && (*node)->right->type == NODE_NUM &&
                     is_zero((*node)->right->data.num_value)))
                {
                    new_node = CREATE_NUM(0.0);
                    description = "mul zero simplified";
                }
                else if ((*node)->right != NULL && (*node)->right->type == NODE_NUM &&
                         is_one((*node)->right->data.num_value))
                {
                    new_node = CopyNode((*node)->left);
                    description = "mul one simplified";
                }
                else if ((*node)->left != NULL && (*node)->left->type == NODE_NUM &&
                         is_one((*node)->left->data.num_value))
                {
                    new_node = CopyNode((*node)->right);
                    description = "mul one simplified";
                }
                else if ((*node)->left != NULL && (*node)->left->type == NODE_NUM &&
                         is_minus_one((*node)->left->data.num_value))
                {
                    // проверяем, не является ли правый операнд тоже умножением на -1
                    if ((*node)->right != NULL && (*node)->right->type == NODE_OP &&
                        (*node)->right->data.op_value == OP_MUL)
                    {
                        Node* right_node = (*node)->right;
                        if (right_node->left != NULL && right_node->left->type == NODE_NUM &&
                            is_minus_one(right_node->left->data.num_value))
                        {
                            // -1 * (-1 * A) => A
                            new_node = CopyNode(right_node->right);
                            description = "double minus simplified";
                        }
                        else if (right_node->right != NULL && right_node->right->type == NODE_NUM &&
                                 is_minus_one(right_node->right->data.num_value))
                        {
                            // -1 * (A * -1) => A
                            new_node = CopyNode(right_node->left);
                            description = "double minus simplified";
                        }
                    }
                    else if ((*node)->right != NULL && (*node)->right->type == NODE_NUM &&
                             is_minus_one((*node)->right->data.num_value))
                    {
                        // -1 * -1 => 1
                        new_node = CREATE_NUM(1.0);
                        description = "minus times minus simplified";
                    }
                }
                else if ((*node)->right != NULL && (*node)->right->type == NODE_NUM &&
                         is_minus_one((*node)->right->data.num_value))
                {
                    // проверяем, не является ли левый операнд тоже умножением на -1
                    if ((*node)->left != NULL && (*node)->left->type == NODE_OP &&
                        (*node)->left->data.op_value == OP_MUL)
                    {
                        Node* left_node = (*node)->left;
                        if (left_node->left != NULL && left_node->left->type == NODE_NUM &&
                            is_minus_one(left_node->left->data.num_value))
                        {
                            // (-1 * A) * -1 => A
                            new_node = CopyNode(left_node->right);
                            description = "double minus simplified";
                        }
                        else if (left_node->right != NULL && left_node->right->type == NODE_NUM &&
                                 is_minus_one(left_node->right->data.num_value))
                        {
                            // (A * -1) * -1 => A
                            new_node = CopyNode(left_node->left);
                            description = "double minus simplified";
                        }
                    }
                }
                break;
            case OP_DIV:
                if ((*node)->left != NULL && (*node)->left->type == NODE_NUM &&
                    is_zero((*node)->left->data.num_value) &&
                    (*node)->right != NULL &&
                    !((*node)->right->type == NODE_NUM && is_zero((*node)->right->data.num_value)))
                {
                    new_node = CREATE_NUM(0.0);
                    description = "0 / simplified";
                }
                else if ((*node)->right != NULL && (*node)->right->type == NODE_NUM &&
                         is_one((*node)->right->data.num_value))
                {
                    new_node = CopyNode((*node)->left);
                    description = " / 1 simplified";
                }
                break;
            case OP_POW:
                if ((*node)->right != NULL && (*node)->right->type == NODE_NUM &&
                    is_zero((*node)->right->data.num_value))
                {
                    new_node = CREATE_NUM(1.0);
                    description = "^0 simplified";
                }
                else if ((*node)->right != NULL && (*node)->right->type == NODE_NUM &&
                         is_one((*node)->right->data.num_value))
                {
                    new_node = CopyNode((*node)->left);
                    description = "^1 simplified";
                }
                else if ((*node)->left != NULL && (*node)->left->type == NODE_NUM &&
                         is_one((*node)->left->data.num_value))
                {
                    new_node = CREATE_NUM(1.0);
                    description = "1^ simplified";
                }
                break;
            default:
                break;
        }

        if (new_node != NULL && description != NULL)
        {
            ReplaceNode(node, new_node);

            double new_result = 0.0;
            if (EvaluateTree(tree, var_table, &new_result) == TREE_ERROR_NO && tex_file != NULL)
            {
                DumpOptimizationStepToFile(tex_file, description, tree, new_result);
                TreeDump(tree, "vova_hochet_glyanut.htm"); //ЭТО ИМЕННО РЕСПЕКТ
            }
        }
    }

    return TREE_ERROR_NO;
}

size_t CountTreeNodes(Node* node)
{
    if (node == NULL)
        return 0;

    return 1 + CountTreeNodes(node->left) + CountTreeNodes(node->right);
}

static TreeErrorType OptimizeSubtreeWithDump(Node** node, FILE* tex_file, Tree* tree, VariableTable* var_table)
{
    if (node == NULL || *node == NULL)
        return TREE_ERROR_NULL_PTR;

    size_t old_size = 0;
    size_t new_size = CountTreeNodes(*node);
    TreeErrorType error = TREE_ERROR_NO;

    do
    {
        old_size = new_size;

        error = ConstantFoldingOptimizationWithDump(node, tex_file, tree, var_table);
        if (error != TREE_ERROR_NO) return error;

        error = NeutralElementsOptimizationWithDump(node, tex_file, tree, var_table);
        if (error != TREE_ERROR_NO) return error;

        new_size = CountTreeNodes(*node);
    } while (new_size != old_size);

    return TREE_ERROR_NO;
}

TreeErrorType OptimizeTreeWithDump(Tree* tree, FILE* tex_file, VariableTable* var_table)
{
    if (tree == NULL)
        return TREE_ERROR_NULL_PTR;

    if (tree->root == NULL)
        return TREE_ERROR_NULL_PTR;

    double result_before = 0.0;
    EvaluateTree(tree, var_table, &result_before);

    if (tex_file != NULL)
    {
        fprintf(tex_file, "\\section*{Optimization}\n");
        fprintf(tex_file, "Before optimization: ");

        char expression[kMaxLengthOfTexExpression] = {0};
        int pos = 0;
        TreeToStringSimple(tree->root, expression, &pos, sizeof(expression));
        fprintf(tex_file, "\\begin{dmath} %s \\end{dmath}\n\n", expression);
    }

    TreeErrorType error = OptimizeSubtreeWithDump(&tree->root, tex_file, tree, var_table);
    if (error != TREE_ERROR_NO)
        return error;

    tree->size = CountTreeNodes(tree->root);

    double result_after = 0.0;
    EvaluateTree(tree, var_table, &result_after);

    if (tex_file != NULL)
    {
        fprintf(tex_file, "\\subsection*{Result optimization}\n");

        char expression[kMaxLengthOfTexExpression] = {0};
        int pos = 0;
        TreeToStringSimple(tree->root, expression, &pos, sizeof(expression));
        fprintf(tex_file, "Final expression: \\[ %s \\]\n\n", expression);
        fprintf(tex_file, "Final result: \\[ %.6f \\]\n\n", result_after);
    }

    return TREE_ERROR_NO;
}

// ==================== UNDEF MACROS ====================
#undef CREATE_NUM
#undef CREATE_OP
#undef CREATE_UNARY_OP
#undef CREATE_VAR
#undef CHECK_AND_CREATE
#undef RELEASE_IF_NULL
