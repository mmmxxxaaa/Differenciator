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
const int         kMaxLengthOfTexExpression           = 10000;


typedef enum {
    NODE_OP,
    NODE_VAR,
    NODE_NUM
} NodeType;

typedef enum {
    OP_ADD,
    OP_MUL,
    OP_DIV,
    OP_SUB,
    OP_SIN,
    OP_COS
} OperationType; //FIXME расширить

typedef struct {
    unsigned int hash;
    char* name;
} VariableDefinition;

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
} Node;

typedef struct {
    Node* root;
    size_t size;
    char* file_buffer;
} Tree;

typedef struct {
    Node*  node;
    size_t depth;
} NodeDepthInfo;

typedef struct {
    NodeDepthInfo* items;
    size_t         size;
    size_t         capacity;
    size_t         current_depth;
} LoadProgress;


#endif //TREE_COMMON_H_
