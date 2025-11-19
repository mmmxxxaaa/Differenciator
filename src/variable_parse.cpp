#include "variable_parse.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "tree_base.h"

void InitVariableTable(VariableTable* ptr_table)
{
    if (ptr_table == NULL)
    {
        fprintf(stderr, "Error: NULL pointer passed to InitVariableTable\n");
        return;
    }

    ptr_table->number_of_variables = 0;
    ptr_table->is_sorted = true;
    ptr_table->variables = (Variable*)calloc(kMaxNOfVariables, sizeof(Variable));

    for (int index_of_variable = 0; index_of_variable < kMaxNOfVariables; index_of_variable++)
    {
        ptr_table->variables[index_of_variable].name[0] = '\0';
        ptr_table->variables[index_of_variable].value = 0.0;
        ptr_table->variables[index_of_variable].hash = 0;
        ptr_table->variables[index_of_variable].is_defined = false;
    }
}

int FindVariableByName(VariableTable* ptr_table, const char* name_of_variable)
{
    if (ptr_table == NULL || name_of_variable == NULL)
        return -1;

    for (int i = 0; i < ptr_table->number_of_variables; i++)
    {
        if (strcmp(ptr_table->variables[i].name, name_of_variable) == 0)
            return i;
    }

    return -1;
}

int FindVariableByHash(VariableTable* ptr_table, unsigned int hash, const char* name_of_variable)
{
    if (ptr_table == NULL || name_of_variable == NULL)
        return -1;

    if (!ptr_table->is_sorted)
        SortVariableTable(ptr_table);

    if (ptr_table->number_of_variables == 0)
        return -1;

    size_t left = 0;
    size_t right = ptr_table->number_of_variables - 1;

    while (left <= right)
    {
        size_t middle = left + (right - left) / 2;

        if (ptr_table->variables[middle].hash == hash)
        {
            size_t same_hash_start = middle;
            size_t same_hash_end = middle;

            while (same_hash_start > 0 && ptr_table->variables[same_hash_start - 1].hash == hash)
                same_hash_start--;

            while (same_hash_end < ptr_table->number_of_variables - 1 &&
                   ptr_table->variables[same_hash_end + 1].hash == hash)
                same_hash_end++;

            if (same_hash_start == same_hash_end)
                return (int)middle;

            for (size_t i = same_hash_start; i <= same_hash_end; i++)
            {
                if (strcmp(ptr_table->variables[i].name, name_of_variable) == 0)
                    return (int)i;
            }
            return -1;
        }
        else if (ptr_table->variables[middle].hash < hash)
        {
            left = middle + 1;
        }
        else
        {
            if (middle == 0) break;
            right = middle - 1;
        }
    }
    return -1;
}

TreeErrorType AddVariable(VariableTable* ptr_table, const char* name_of_variable)
{
    if (ptr_table == NULL || name_of_variable == NULL)
        return TREE_ERROR_NULL_PTR;

    if (ptr_table->number_of_variables >= kMaxNOfVariables)
        return TREE_ERROR_VARIABLE_TABLE;

    if (FindVariableByName(ptr_table, name_of_variable) != -1)
        return TREE_ERROR_REDEFINITION_VARIABLE;

    int index = ptr_table->number_of_variables;
    strncpy(ptr_table->variables[index].name, name_of_variable, kMaxVariableLength - 1);
    ptr_table->variables[index].name[kMaxVariableLength - 1] = '\0';
    ptr_table->variables[index].value = 0.0;
    ptr_table->variables[index].hash = ComputeHash(name_of_variable);
    ptr_table->variables[index].is_defined = true;

    ptr_table->number_of_variables++;
    ptr_table->is_sorted = false;

    return TREE_ERROR_NO;
}

TreeErrorType SetVariableValue(VariableTable* ptr_table, const char* name_of_variable, double value)
{
    if (ptr_table == NULL || name_of_variable == NULL)
        return TREE_ERROR_NULL_PTR;

    int index = FindVariableByName(ptr_table, name_of_variable);
    if (index == -1)
        return TREE_ERROR_VARIABLE_NOT_FOUND;

    ptr_table->variables[index].value = value;
    ptr_table->variables[index].is_defined = true;

    return TREE_ERROR_NO;
}

TreeErrorType GetVariableValue(VariableTable* ptr_table, const char* name_of_variable, double* value)
{
    if (ptr_table == NULL || name_of_variable == NULL || value == NULL)
        return TREE_ERROR_NULL_PTR;

    int index = FindVariableByName(ptr_table, name_of_variable);
    if (index == -1)
        return TREE_ERROR_VARIABLE_NOT_FOUND;

    if (!ptr_table->variables[index].is_defined)
        return TREE_ERROR_VARIABLE_UNDEFINED;

    *value = ptr_table->variables[index].value;
    return TREE_ERROR_NO;
}

TreeErrorType RequestVariableValue(VariableTable* ptr_table, const char* variable_name)
{
    if (ptr_table == NULL || variable_name == NULL)
        return TREE_ERROR_NULL_PTR;

    printf("Введите значение для переменной '%s': ", variable_name);

    double value = 0.0;
    if (scanf("%lf", &value) != 1)
    {
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        return TREE_ERROR_INVALID_INPUT;
    }

    int c = '\0';
    while ((c = getchar()) != '\n' && c != EOF);

    return SetVariableValue(ptr_table, variable_name, value);
}

void SortVariableTable(VariableTable* ptr_table)
{
    if (ptr_table == NULL)
        return;

    if (ptr_table->number_of_variables > 1)
    {
        qsort(ptr_table->variables, ptr_table->number_of_variables, sizeof(Variable), CompareVariablesByHash);
        ptr_table->is_sorted = true;
    }
}

int CompareVariablesByHash(const void* first_var, const void* second_var)
{
    const Variable* varA = (const Variable*) first_var;
    const Variable* varB = (const Variable*) second_var;

    if (varA->hash != varB->hash)
        return (varA->hash < varB->hash) ? -1 : 1;

    return strcmp(varA->name, varB->name);
}

void DestroyVariableTable(VariableTable* ptr_table)
{
    if (ptr_table == NULL)
        return;

    if (ptr_table->variables != NULL)
    {
        free(ptr_table->variables);
        ptr_table->variables = NULL;
    }

    ptr_table->number_of_variables = 0;
    ptr_table->is_sorted = true;
}
