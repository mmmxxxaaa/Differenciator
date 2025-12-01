#ifndef NEW_GREAT_INPUT_H_
#define NEW_GREAT_INPUT_H_

#include "tree_common.h"
#include "variable_parse.h"
#include "operations.h"

typedef struct {
    VariableTable* var_table;
    OperationInfo* operations;
    size_t operations_count;
    bool hashes_initialized;
} ParserContext;

Node* GetG(const char** s, VariableTable* var_table);
Node* GetE(const char** s, ParserContext* context);
Node* GetT(const char** s, ParserContext* context);
Node* GetF(const char** s, ParserContext* context);
Node* GetP(const char** s, ParserContext* context);
Node* GetN(const char** s);
Node* GetV(const char** s, ParserContext* context);
Node* GetFunction(const char** s, ParserContext* context);
void SyntaxError();

#endif // NEW_GREAT_INPUT_H_
