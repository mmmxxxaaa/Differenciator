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
#include "DSL.h"

// ==================== ПРОТОТИПЫ ФУНКЦИЙ ====================
static bool  ContainsVariable(Node* node, const char* variable_name);
static void  ReplaceNode(Node** node_ptr, Node* new_node);
static Node* DifferentiateNode(Node* node, const char* variable_name);

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

                if (is_binary(node->data.op_value))
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
                    case OP_TAN:
                        *result = tan(right_result);
                        break;
                    case OP_COT:
                        if (is_zero(tan(right_result)))
                            return TREE_ERROR_DIVISION_BY_ZERO;
                        *result = 1.0 / tan(right_result);
                        break;
                    case OP_ARCSIN:
                        if (right_result < -1.0 || right_result > 1.0)
                            return TREE_ERROR_MATH_DOMAIN;
                        *result = asin(right_result);
                        break;
                    case OP_ARCCOS:
                        if (right_result < -1.0 || right_result > 1.0)
                            return TREE_ERROR_MATH_DOMAIN;
                        *result = acos(right_result);
                        break;
                    case OP_ARCTAN:
                        *result = atan(right_result);
                        break;
                    case OP_ARCCOT:
                        *result = M_PI/2.0 - atan(right_result);
                        break;
                    case OP_SINH:
                        *result = sinh(right_result);
                        break;
                    case OP_COSH:
                        *result = cosh(right_result);
                        break;
                    case OP_TANH:
                        *result = tanh(right_result);
                        break;
                    case OP_COTH:
                        if (is_zero(tanh(right_result)))
                            return TREE_ERROR_DIVISION_BY_ZERO;
                        *result = 1.0 / tanh(right_result);
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

    switch (type)
    {
        case NODE_NUM:
        case NODE_VAR:
            node->priority = 0;
            break;

        case NODE_OP:
            switch (data.op_value)
            {
                case OP_ADD:
                case OP_SUB:
                    node->priority = 1;
                    break;

                case OP_MUL:
                case OP_DIV:
                    node->priority = 2;
                    break;

                case OP_SIN:
                case OP_COS:
                case OP_LN:
                case OP_EXP:
                    node->priority = 3;
                    break;

                case OP_POW:
                    node->priority = 4;
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

static Node* CreateVariableNode(const char* name)
{
    if (!name)
        return NULL;

    ValueOfTreeElement data = {};
    data.var_definition.name = strdup(name);
    if (!data.var_definition.name)
        return NULL;

    data.var_definition.hash = ComputeHash(name);
    return CreateNode(NODE_VAR, data, NULL, NULL);
}

Node* CopyNode(Node* original)
{
    if (original == NULL)
        return NULL;

    ValueOfTreeElement data = {};
    Node* new_node = NULL;

    switch (original->type)
    {
        case NODE_NUM:
            data.num_value = original->data.num_value;
            new_node = NUM(data.num_value);
            break;

        case NODE_VAR:
            new_node = CreateVariableNode(original->data.var_definition.name ?
                                          original->data.var_definition.name : "?");
            break;

        case NODE_OP:
            data.op_value = original->data.op_value;
            if (original->data.op_value == OP_SIN || original->data.op_value == OP_COS ||
                original->data.op_value == OP_LN || original->data.op_value == OP_EXP)
            {
                new_node = CreateNode(NODE_OP, data, NULL, CopyNode(original->right));
            }
            else
            {
                new_node = CreateNode(NODE_OP, data, CopyNode(original->left), CopyNode(original->right));
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

// ==================== ДИФФЕРЕНЦИРОВАНИЕ ЧЕРЕЗ DSL ====================

static Node* DifferentiateNode(Node* node, const char* variable_name)
{
    if (node == NULL)
        return NULL;

    switch (node->type)
    {
        case NODE_NUM:
            return NUM(0.0);

        case NODE_VAR:
            if (node->data.var_definition.name &&
                strcmp(node->data.var_definition.name, variable_name) == 0)
            {
                return NUM(1.0);
            }
            else
            {
                return NUM(0.0);
            }

        case NODE_OP:
            switch (node->data.op_value)
            {
                case OP_ADD:
                    return ADD(DIFF(node->left, variable_name),
                              DIFF(node->right, variable_name));

                case OP_SUB:
                    return SUB(DIFF(node->left, variable_name),
                              DIFF(node->right, variable_name));

                case OP_MUL:
                    return ADD(MUL(COPY(node->left),
                                   DIFF(node->right, variable_name)),
                               MUL(COPY(node->right),
                                   DIFF(node->left, variable_name)));

                case OP_DIV:
                    return DIV(SUB(MUL(COPY(node->right),
                                        DIFF(node->left, variable_name)),
                                   MUL(COPY(node->left),
                                        DIFF(node->right, variable_name))),
                               MUL(COPY(node->right),
                                   COPY(node->right)));

                case OP_SIN:
                    return MUL(COS(COPY(node->right)),
                              DIFF(node->right, variable_name));

                case OP_COS:
                    return MUL(MUL(NUM(-1.0),
                                   SIN(COPY(node->right))),
                               DIFF(node->right, variable_name));

                case OP_LN:
                    return MUL(DIV(NUM(1.0),
                                   COPY(node->right)),
                              DIFF(node->right, variable_name));

                case OP_EXP:
                    return MUL(EXP(COPY(node->right)),
                              DIFF(node->right, variable_name));

                case OP_POW:
                {
                    bool left_has_var = ContainsVariable(node->left, variable_name);
                    bool right_has_var = ContainsVariable(node->right, variable_name);

                    if (left_has_var && !right_has_var)
                    {
                        // x^a -> a * x^(a-1) * dx
                        return MUL(MUL(COPY(node->right),
                                       POW(COPY(node->left),
                                           NUM(node->right->data.num_value - 1.0))),
                                  DIFF(node->left, variable_name));
                    }
                    else if (!left_has_var && right_has_var)
                    {
                        // a^x -> a^x * ln(a) * dx
                        return MUL(MUL(POW(COPY(node->left), COPY(node->right)),
                                       LN(COPY(node->left))),
                                  DIFF(node->right, variable_name));
                    }
                    else
                    {
                        // x^g(x) -> x^g(x) * (g'(x)*ln(x) + g(x)/x * dx)
                        Node* u_pow_v = POW(COPY(node->left), COPY(node->right));
                        Node* bracket = ADD(MUL(DIFF(node->right, variable_name),
                                               LN(COPY(node->left))),
                                           MUL(DIV(COPY(node->right),
                                                   COPY(node->left)),
                                               DIFF(node->left, variable_name)));
                        return MUL(u_pow_v, bracket);
                    }
                }
                case OP_TAN:
                    return MUL(DIV(NUM(1.0),
                                   MUL(COS(COPY(node->right)),
                                       COS(COPY(node->right)))),
                               DIFF(node->right, variable_name));

                case OP_COT:
                    return MUL(NUM(-1.0),
                               MUL(DIV(NUM(1.0),
                                       MUL(SIN(COPY(node->right)),
                                           SIN(COPY(node->right)))),
                                   DIFF(node->right, variable_name)));

                case OP_ARCSIN:
                    return MUL(DIV(NUM(1.0),
                                   SQRT(SUB(NUM(1.0),
                                            MUL(COPY(node->right),
                                                COPY(node->right))))),
                               DIFF(node->right, variable_name));

                case OP_ARCCOS:
                    return MUL(NUM(-1.0),
                               MUL(DIV(NUM(1.0),
                                       SQRT(SUB(NUM(1.0),
                                                MUL(COPY(node->right),
                                                    COPY(node->right))))),
                                   DIFF(node->right, variable_name)));

                case OP_ARCTAN:
                    return MUL(DIV(NUM(1.0),
                                   ADD(NUM(1.0),
                                       MUL(COPY(node->right),
                                           COPY(node->right)))),
                               DIFF(node->right, variable_name));

                case OP_ARCCOT:
                    return MUL(NUM(-1.0),
                               MUL(DIV(NUM(1.0),
                                       ADD(NUM(1.0),
                                           MUL(COPY(node->right),
                                               COPY(node->right)))),
                                   DIFF(node->right, variable_name)));

                case OP_SINH:
                    return MUL(COSH(COPY(node->right)),
                               DIFF(node->right, variable_name));

                case OP_COSH:
                    return MUL(SINH(COPY(node->right)),
                               DIFF(node->right, variable_name));

                case OP_TANH:
                    return MUL(SUB(NUM(1.0),
                                   MUL(TANH(COPY(node->right)),
                                       TANH(COPY(node->right)))),
                               DIFF(node->right, variable_name));

                case OP_COTH:
                    return MUL(SUB(NUM(1.0),
                                   MUL(COTH(COPY(node->right)),
                                       COTH(COPY(node->right)))),
                               DIFF(node->right, variable_name));
                                default:
                                    return NUM(0.0);
            }

        default:
            return NUM(0.0);
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
    // FIXME - dsl
        if (is_unary((*node)->data.op_value) && IsNodeType((*node)->right, NODE_NUM))
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
                case OP_TAN:
                    result = tan((*node)->right->data.num_value);
                    break;
                case OP_COT:
                    if (is_zero(tan((*node)->right->data.num_value)))
                        can_fold = false;
                    else
                        result = 1.0 / tan((*node)->right->data.num_value);
                    break;
                case OP_ARCSIN:
                    if ((*node)->right->data.num_value < -1.0 || (*node)->right->data.num_value > 1.0)
                        can_fold = false;
                    else
                        result = asin((*node)->right->data.num_value);
                    break;
                case OP_ARCCOS:
                    if ((*node)->right->data.num_value < -1.0 || (*node)->right->data.num_value > 1.0)
                        can_fold = false;
                    else
                        result = acos((*node)->right->data.num_value);
                    break;
                case OP_ARCTAN:
                    result = atan((*node)->right->data.num_value);
                    break;
                case OP_ARCCOT:
                    result = M_PI/2.0 - atan((*node)->right->data.num_value);
                    break;
                case OP_SINH:
                    result = sinh((*node)->right->data.num_value);
                    break;
                case OP_COSH:
                    result = cosh((*node)->right->data.num_value);
                    break;
                case OP_TANH:
                    result = tanh((*node)->right->data.num_value);
                    break;
                case OP_COTH:
                    if (is_zero(tanh((*node)->right->data.num_value)))
                        can_fold = false;
                    else
                        result = 1.0 / tanh((*node)->right->data.num_value);
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
                Node* new_node = NUM(result);
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
        // FIXME - dsl
        else if (IsNodeType((*node)->left, NODE_NUM) && IsNodeType((*node)->right, NODE_NUM))
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
                Node* new_node = NUM(result);
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
    }

    return TREE_ERROR_NO;
}

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
        // FIXME - dsl
            case OP_ADD:
                if (IsNodeType((*node)->right, NODE_NUM) &&
                    is_zero((*node)->right->data.num_value))
                {
                    new_node = CopyNode((*node)->left);
                    description = "adding zero simplified";
                }
                else if (IsNodeType((*node)->left, NODE_NUM) &&
                         is_zero((*node)->left->data.num_value))
                {
                    new_node = CopyNode((*node)->right);
                    description = "adding zero simplified";
                }
                break;

            case OP_SUB:
                if (IsNodeType((*node)->right, NODE_NUM) &&
                    is_zero((*node)->right->data.num_value))
                {
                    new_node = CopyNode((*node)->left);
                    description = "- 0 simplified";
                }
                break;

            case OP_MUL:
                if ((IsNodeType((*node)->left, NODE_NUM) &&
                     is_zero((*node)->left->data.num_value)) ||
                    (IsNodeType((*node)->right, NODE_NUM) &&
                     is_zero((*node)->right->data.num_value)))
                {
                    new_node = NUM(0.0);
                    description = "mul zero simplified";
                }
                else if (IsNodeType((*node)->right, NODE_NUM) &&
                         is_one((*node)->right->data.num_value))
                {
                    new_node = CopyNode((*node)->left);
                    description = "mul one simplified";
                }
                else if (IsNodeType((*node)->left, NODE_NUM) &&
                         is_one((*node)->left->data.num_value))
                {
                    new_node = CopyNode((*node)->right);
                    description = "mul one simplified";
                }
                else if (IsNodeType((*node)->left, NODE_NUM) &&
                         is_minus_one((*node)->left->data.num_value))
                {
                    if (IsNodeType((*node)->right, NODE_OP) &&
                        (*node)->right->data.op_value == OP_MUL)
                    {
                        Node* right_node = (*node)->right;
                        if (IsNodeType(right_node->left, NODE_NUM) &&
                            is_minus_one(right_node->left->data.num_value))
                        {
                            new_node = CopyNode(right_node->right);
                            description = "double minus simplified";
                        }
                        else if (IsNodeType(right_node->right, NODE_NUM) &&
                                 is_minus_one(right_node->right->data.num_value))
                        {
                            new_node = CopyNode(right_node->left);
                            description = "double minus simplified";
                        }
                    }
                    else if (IsNodeType((*node)->right, NODE_NUM) &&
                             is_minus_one((*node)->right->data.num_value))
                    {
                        new_node = NUM(1.0);
                        description = "minus times minus simplified";
                    }
                }
                else if ((*node)->right != NULL && (*node)->right->type == NODE_NUM &&
                         is_minus_one((*node)->right->data.num_value))
                {
                    if (IsNodeType((*node)->left, NODE_OP) &&
                        (*node)->left->data.op_value == OP_MUL)
                    {
                        Node* left_node = (*node)->left;
                        if (IsNodeType(left_node, NODE_NUM) &&
                            is_minus_one(left_node->left->data.num_value))
                        {
                            new_node = CopyNode(left_node->right);
                            description = "double minus simplified";
                        }
                        else if (IsNodeType(left_node->right, NODE_NUM) &&
                                 is_minus_one(left_node->right->data.num_value))
                        {
                            new_node = CopyNode(left_node->left);
                            description = "double minus simplified";
                        }
                    }
                }
                break;
            case OP_DIV:
                if (IsNodeType((*node)->left, NODE_NUM)&&
                    is_zero((*node)->left->data.num_value) &&
                    (*node)->right != NULL &&
                    !((*node)->right->type == NODE_NUM && is_zero((*node)->right->data.num_value)))
                {
                    new_node = NUM(0.0);
                    description = "0 / simplified";
                }
                else if (IsNodeType((*node)->right, NODE_NUM) &&
                         is_one((*node)->right->data.num_value))
                {
                    new_node = CopyNode((*node)->left);
                    description = " / 1 simplified";
                }
                break;
            case OP_POW:
                if (IsNodeType((*node)->right, NODE_NUM) &&
                    is_zero((*node)->right->data.num_value))
                {
                    new_node = NUM(1.0);
                    description = "^0 simplified";
                }
                else if (IsNodeType((*node)->right, NODE_NUM) &&
                         is_one((*node)->right->data.num_value))
                {
                    new_node = CopyNode((*node)->left);
                    description = "^1 simplified";
                }
                else if (IsNodeType((*node)->left, NODE_NUM) &&
                         is_one((*node)->left->data.num_value))
                {
                    new_node = NUM(1.0);
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
        fprintf(tex_file, "Final expression: \\begin{dmath} %s \\end{dmath}\n\n", expression);
        fprintf(tex_file, "Final result: \\begin{dmath} %.6f \\end{dmath}\n\n", result_after);
        DumpVariableTableToFile(tex_file, var_table);
    }

    return TREE_ERROR_NO;
}

// ==================== РАЗЛОЖЕНИЕ В РЯД ТЕЙЛОРА ====================


#include "DSL_undef.h"
