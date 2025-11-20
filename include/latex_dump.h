#ifndef LATEX_DUMP_H
#define LATEX_DUMP_H

#include "tree_base.h"
#include "variable_parse.h"

TreeErrorType GenerateLatexDump(Tree* tree, VariableTable* var_table, const char* filename, double result_value);

char* TreeToLatex(Node* node);
char* NodeToLatex(Node* node);

#endif // LATEX_DUMP_H
