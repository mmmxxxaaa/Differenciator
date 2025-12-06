#ifndef TREE_COMMON_H_
#define TREE_COMMON_H_

#include <stdlib.h>

const int         kMaxSystemCommandLength             = 512;
const int         kMaxLengthOfFilename                = 256;
const char* const kGeneralFolderNameForLogs           = "tree_logs";
const int         kMaxLengthOfAnswer                  = 256;
const int         kMaxInputCapacity                   = 256;
const int         kMaxPathDepth                       = 100;
const int         kTreeDumpAfterAddingElementCapacity = 512;
const char* const kDefaultDataBaseFilename            = "differenciator_tree.txt";
const int         kMaxNumberOfDerivative              = 4;
const int         kMaxLengthOfTexExpression           = 30000;
const char* const kTexFilename                        = "full_analysis.tex";
const int         kMaxDotBufferLength                 = 64;
const int         kMaxTexDescriptionLength            = 256;
const int         kMaxNOfVariables                    = 100;
const int         kMaxVariableLength                  = 32;
const int         kMaxFuncNameLength                  = 256;
const int         kMaxCustomNotationLength            = 32;
const int         kTaylor                             = 7;

typedef enum {
    NODE_OP,
    NODE_VAR,
    NODE_NUM
} NodeType;

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_POW,
    OP_SIN,
    OP_COS,
    OP_TAN,
    OP_COT,
    OP_ARCSIN,
    OP_ARCCOS,
    OP_ARCTAN,
    OP_ARCCOT,
    OP_SINH,
    OP_COSH,
    OP_TANH,
    OP_COTH,
    OP_LN,
    OP_EXP,
    OP_COUNT
} OperationType;

typedef struct {
    unsigned int hash;
    char* name;
} VariableDefinition;

typedef struct {
    unsigned int hash;
    const char* name;
    OperationType op_value;
} OperationInfo;

typedef union {
    double             num_value;
    OperationType      op_value;
    VariableDefinition var_definition;
} ValueOfTreeElement;

typedef struct Node {
    ValueOfTreeElement  data;
    NodeType            type;
    struct Node*        left;
    struct Node*        right;
    struct Node*        parent;
    int                 priority;  // Приоритет операции (0 для чисел и переменных)
} Node;

typedef struct {
    Node* root;
    size_t size;
    char* file_buffer;
} Tree;

#endif //TREE_COMMON_H_
