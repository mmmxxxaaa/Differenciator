#ifndef PROCESSING_DIFF_H
#define PROCESSING_DIFF_H

#include <stdio.h>
#include "tree_base.h"
#include "variable_parse.h"
#include "tree_error_types.h"

typedef struct {
    Tree tree;
    VariableTable var_table;
    char* expression;
    FILE* tex_file;
    double result;
} DifferentiatorStruct;

DifferentiatorStruct* CreateDifferentiatorStruct();
void DestroyDifferentiatorStruct(DifferentiatorStruct* diff_struct);

TreeErrorType InitializeExpression         (DifferentiatorStruct* diff_struct, int argc, const char** argv);
TreeErrorType ParseExpressionTree          (DifferentiatorStruct* diff_struct);
TreeErrorType InitializeLatexOutput        (DifferentiatorStruct* diff_struct);
TreeErrorType RequestVariableValues        (DifferentiatorStruct* diff_struct);
TreeErrorType EvaluateOriginalFunction     (DifferentiatorStruct* diff_struct);
TreeErrorType OptimizeExpressionTree       (DifferentiatorStruct* diff_struct);
TreeErrorType PlotFunctionGraph            (DifferentiatorStruct* diff_struct);
TreeErrorType PerformDifferentiationProcess(DifferentiatorStruct* diff_struct);
TreeErrorType FinalizeLatexOutput          (DifferentiatorStruct* diff_struct);
void PrintErrorAndCleanup(DifferentiatorStruct* diff_struct, TreeErrorType error);


#endif // PROCESSING_DIFF_H
