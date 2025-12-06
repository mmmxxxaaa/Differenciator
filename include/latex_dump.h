#ifndef LATEX_DUMP_H
#define LATEX_DUMP_H

#include "tree_base.h"
#include "tree_common.h"
#include "variable_parse.h"
#include "processing_diff.h"

#include <stdio.h>

typedef struct {
    const char* prefix;         // то, что должно быть до аргумента
    const char* infix;          // то, что должно быть между аргументами (для бинарных)
    const char* postfix;        // то, что должно быть после аргумента
    bool should_compare_priority;  // нужно ли сравнивать приоритеты
    bool is_binary;             // бинарная или унарная операция
    bool right_use_less_equal;  // использовать <= вместо < для правого аргумента
} OpFormat;

void          TreeToStringSimple(Node* node, char* buffer, int* pos, int buffer_size);
const OpFormat* GetOpFormat(OperationType op_type);

char*         ConvertLatexToPGFPlot(const char* latex_expr);

TreeErrorType StartLatexDump(FILE* file);
TreeErrorType AddFunctionPlot(DifferentiatorStruct* diff_struct, const char* diff_variable);
TreeErrorType EndLatexDump(FILE* file);

TreeErrorType DumpOriginalFunctionToFile(FILE* file, Tree* tree, double result_value);
TreeErrorType DumpOptimizationStepToFile(FILE* file, const char* description, Tree* tree, double result_value);
TreeErrorType DumpDerivativeToFile(FILE* file, Tree* derivative_tree, double derivative_result, int derivative_order);
TreeErrorType DumpVariableTableToFile(FILE* file, VariableTable* var_table);


#endif // LATEX_DUMP_H
