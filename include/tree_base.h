#ifndef TREE_BASE_H_
#define TREE_BASE_H_

#include <stdbool.h>
#include "tree_common.h"
#include "tree_error_types.h"

#define DEBUG

#ifdef DEBUG
    #define DEBUG_PRINT(format, ...) \
        do { \
            fprintf(stderr, "[DEBUG %s:%d] ", __FILE__, __LINE__); \
            fprintf(stderr, format, ##__VA_ARGS__); \
        } while(0)
#else
    #define DEBUG_PRINT(format, ...) ((void)0)
#endif

TreeErrorType TreeCtor(Tree* tree);
TreeErrorType TreeDtor(Tree* tree);
void FreeNode(Node* node);

bool IsLeaf(Node* node);

unsigned int ComputeHash(const char* str);

#endif // TREE_BASE_H_
