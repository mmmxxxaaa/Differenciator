#ifndef LATEX_DUMP_H
#define LATEX_DUMP_H

#include "tree_base.h"
#include "variable_parse.h"
#include <stdio.h>

void          TreeToStringSimple(Node* node, char* buffer, int* pos, int buffer_size);
TreeErrorType StartLatexDump(FILE* file);
TreeErrorType EndLatexDump(FILE* file);
TreeErrorType DumpOriginalFunctionToFile(FILE* file, Tree* tree, double result_value);
TreeErrorType DumpOptimizationStepToFile(FILE* file, const char* description, Tree* tree, double result_value);
TreeErrorType DumpDerivativeToFile(FILE* file, Tree* derivative_tree, double derivative_result, int derivative_order);
TreeErrorType DumpVariableTableToFile(FILE* file, VariableTable* var_table);


// TreeErrorType GenerateLatexDump(Tree* tree, VariableTable* var_table, const char* filename, double result_value);
// TreeErrorType GenerateLatexDumpWithDerivatives(Tree* tree, Tree** derivative_trees, double* derivative_results,
//                                               int derivative_count, VariableTable* var_table,
//                                               const char* filename, double result_value);
// TreeErrorType DumpOptimizationStep(FILE* file, const char* description, Tree* tree, double result_value);
// char* TreeToLatex(Node* node);
// char* NodeToLatex(Node* node);

#endif // LATEX_DUMP_H
