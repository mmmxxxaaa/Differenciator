#ifndef VARIABLE_PARSE_H_
#define VARIABLE_PARSE_H_

#include <stdlib.h>

#include "tree_error_types.h"
#include "tree_common.h"

typedef struct {
    char   name[kMaxVariableLength];
    double value;
    size_t hash;
    bool   is_defined;
} Variable;

typedef struct {
    Variable* variables;
    int       number_of_variables;
    bool      is_sorted;
} VariableTable;

void InitVariableTable(VariableTable* ptr_table);
int FindVariableByName(VariableTable* ptr_table, const char* name_of_variable); //если встретиили в первый раз, то добавялем вместо
TreeErrorType AddVariable         (VariableTable* ptr_table, const char* name_of_variable);
TreeErrorType SetVariableValue    (VariableTable* ptr_table, const char* name_of_variable, double value);
TreeErrorType GetVariableValue    (VariableTable* ptr_table, const char* name_of_variable, double* value);
TreeErrorType RequestVariableValue(VariableTable* ptr_table, const char* variable_name);

int  FindVariableByHash(VariableTable* ptr_table, unsigned int hash, const char* name_of_variable); //возвращаем индекс
void SortVariableTable(VariableTable* ptr_table);
int  CompareVariablesByHash(const void* first_var, const void* second_var);
void DestroyVariableTable(VariableTable* ptr_table);


#endif //VARIABLE_PARSE_H_
