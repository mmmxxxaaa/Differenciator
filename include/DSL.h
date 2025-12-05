#ifndef DSL_H
#define DSL_H

#include "tree_base.h"
#include "operations.h"
#include "tree_common.h"
#include "variable_parse.h"

// ==================== БАЗОВЫЕ МАКРОСЫ ====================
#define COPY(node) CopyNode(node)
#define DIFF(node, var) DifferentiateNode((node), (var))

// Проверка типов узлов
#define IS_NUM(node) ((node) && (node)->type == NODE_NUM)
#define IS_VAR(node) ((node) && (node)->type == NODE_VAR)
#define IS_OP(node) ((node) && (node)->type == NODE_OP)

// Проверка операций
#define IS_ADD(node) (IS_OP(node) && (node)->data.op_value == OP_ADD)
#define IS_SUB(node) (IS_OP(node) && (node)->data.op_value == OP_SUB)
#define IS_MUL(node) (IS_OP(node) && (node)->data.op_value == OP_MUL)
#define IS_DIV(node) (IS_OP(node) && (node)->data.op_value == OP_DIV)
#define IS_POW(node) (IS_OP(node) && (node)->data.op_value == OP_POW)
#define IS_SIN(node) (IS_OP(node) && (node)->data.op_value == OP_SIN)
#define IS_COS(node) (IS_OP(node) && (node)->data.op_value == OP_COS)
#define IS_LN(node)  (IS_OP(node) && (node)->data.op_value == OP_LN)
#define IS_EXP(node) (IS_OP(node) && (node)->data.op_value == OP_EXP)

// ==================== СОЗДАНИЕ УЗЛОВ ====================
#define NUM(val)     CreateNode(NODE_NUM, (ValueOfTreeElement){.num_value = (val)}, NULL, NULL)
#define VAR(var_name) CreateNode(NODE_VAR, (ValueOfTreeElement){.var_definition = {.hash = ComputeHash(var_name), .name = strdup(var_name)}}, NULL, NULL)
// Бинарные операции
#define ADD(left, right) CreateNode(NODE_OP, (ValueOfTreeElement){.op_value = OP_ADD}, (left), (right))
#define SUB(left, right) CreateNode(NODE_OP, (ValueOfTreeElement){.op_value = OP_SUB}, (left), (right))
#define MUL(left, right) CreateNode(NODE_OP, (ValueOfTreeElement){.op_value = OP_MUL}, (left), (right))
#define DIV(left, right) CreateNode(NODE_OP, (ValueOfTreeElement){.op_value = OP_DIV}, (left), (right))
#define POW(left, right) CreateNode(NODE_OP, (ValueOfTreeElement){.op_value = OP_POW}, (left), (right))

// Унарные операции
#define SIN(arg)  CreateNode(NODE_OP, (ValueOfTreeElement){.op_value = OP_SIN}, NULL, (arg))
#define COS(arg)  CreateNode(NODE_OP, (ValueOfTreeElement){.op_value = OP_COS}, NULL, (arg))
#define LN(arg)   CreateNode(NODE_OP, (ValueOfTreeElement){.op_value = OP_LN}, NULL, (arg))
#define EXP(arg)  CreateNode(NODE_OP, (ValueOfTreeElement){.op_value = OP_EXP}, NULL, (arg))

// ==================== ДЛЯ ДИФФЕРЕНЦИРОВАНИЯ ====================
#define U  COPY(node->left)
#define V  COPY(node->right)
#define DU DIFF(node->left, variable_name)
#define DV DIFF(node->right, variable_name)

// Правила дифференцирования в DSL-стиле
#define DIFF_ADD ADD(DU, DV)                      // d(u+v)/dx = du/dx + dv/dx
#define DIFF_SUB SUB(DU, DV)                      // d(u-v)/dx = du/dx - dv/dx
#define DIFF_MUL ADD(MUL(U, DV), MUL(V, DU))      // d(u*v)/dx = u*dv/dx + v*du/dx
#define DIFF_DIV DIV(SUB(MUL(V, DU), MUL(U, DV)), MUL(V, V))  // d(u/v)/dx = (v*du/dx - u*dv/dx) / v^2

// Унарные операции
#define DIFF_SIN MUL(COS(COPY(node->right)), DIFF(node->right, variable_name))                  // d(sin(u))/dx = cos(u) * du/dx
#define DIFF_COS MUL(MUL(NUM(-1.0), SIN(COPY(node->right))), DIFF(node->right, variable_name))  // d(cos(u))/dx = -sin(u) * du/dx
#define DIFF_LN  MUL(DIV(NUM(1.0), COPY(node->right)), DIFF(node->right, variable_name))        // d(ln(u))/dx = (1/u) * du/dx
#define DIFF_EXP MUL(EXP(COPY(node->right)), DIFF(node->right, variable_name))                  // d(e^u)/dx = e^u * du/dx

// ==================== УТИЛИТЫ ====================
#define FREE_NODES(count, ...) \
    do { \
        Node* nodes[] = {__VA_ARGS__}; \
        for (size_t i = 0; i < (count) && i < sizeof(nodes)/sizeof(nodes[0]); i++) \
            if (nodes[i]) FreeSubtree(nodes[i]); \
    } while(0)

// Проверка на константу
#define IS_CONSTANT(node, var) (!ContainsVariable((node), (var)))

#endif // DSL_H
