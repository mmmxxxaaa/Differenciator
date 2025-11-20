#include "latex_dump.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void TreeToStringSimple(Node* node, char* buffer, int* pos, int buffer_size)
{
    if (node == NULL || *pos >= buffer_size - 1)
        return;

    switch (node->type)
    {
        case NODE_NUM:
            *pos += snprintf(buffer + *pos, buffer_size - *pos, "%.2f", node->data.num_value);
            break;

        case NODE_VAR:
            if (node->data.var_definition.name)
            {
                *pos += snprintf(buffer + *pos, buffer_size - *pos, "%s", node->data.var_definition.name);
            }
            else
            {
                *pos += snprintf(buffer + *pos, buffer_size - *pos, "?");
            }
            break;

        case NODE_OP:
            switch (node->data.op_value)
            {
                case OP_ADD:
                    TreeToStringSimple(node->left, buffer, pos, buffer_size);
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, " + ");
                    TreeToStringSimple(node->right, buffer, pos, buffer_size);
                    break;
                case OP_SUB:
                    TreeToStringSimple(node->left, buffer, pos, buffer_size);
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, " - ");
                    TreeToStringSimple(node->right, buffer, pos, buffer_size);
                    break;
                case OP_MUL:
                    TreeToStringSimple(node->left, buffer, pos, buffer_size);
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, " * ");
                    TreeToStringSimple(node->right, buffer, pos, buffer_size);
                    break;
                case OP_DIV:
                    TreeToStringSimple(node->left, buffer, pos, buffer_size);
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, " / ");
                    TreeToStringSimple(node->right, buffer, pos, buffer_size);
                    break;
                case OP_SIN:
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, "\\sin(");
                    TreeToStringSimple(node->right, buffer, pos, buffer_size);
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, ")");
                    break;
                case OP_COS:
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, "\\cos(");
                    TreeToStringSimple(node->right, buffer, pos, buffer_size);
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, ")");
                    break;
                default:
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, "?");
            }
            break;

        default:
            *pos += snprintf(buffer + *pos, buffer_size - *pos, "?");
    }
}

TreeErrorType GenerateLatexDump(Tree* tree, VariableTable* var_table, const char* filename, double result_value)
{
    if (tree == NULL || var_table == NULL || filename == NULL)
        return TREE_ERROR_NULL_PTR;

    FILE* file = fopen(filename, "w");
    if (!file)
        return TREE_ERROR_IO;

    fprintf(file, "\\documentclass{article}\n");
    fprintf(file, "\\usepackage[utf8]{inputenc}\n");
    fprintf(file, "\\usepackage{amsmath}\n");
    fprintf(file, "\\begin{document}\n\n");

    fprintf(file, "\\section*{Mathematical Expression}\n\n");

    char expression[1024] = {0}; //FIXME
    int pos = 0;
    TreeToStringSimple(tree->root, expression, &pos, sizeof(expression));

    fprintf(file, "Expression: \\[ %s \\]\n\n", expression);

    fprintf(file, "Result: \\[ %.6f \\]\n\n", result_value);

    if (var_table->number_of_variables > 0)
    {
        fprintf(file, "\\section*{Variables}\n");
        fprintf(file, "\\begin{tabular}{|c|c|}\n");
        fprintf(file, "\\hline\n");
        fprintf(file, "Name & Value \\\\\n");
        fprintf(file, "\\hline\n");

        for (int i = 0; i < var_table->number_of_variables; i++)
        {
            fprintf(file, "%s & %.4f \\\\\n",
                   var_table->variables[i].name,
                   var_table->variables[i].value);
        }

        fprintf(file, "\\hline\n");
        fprintf(file, "\\end{tabular}\n\n");
    }

    fprintf(file, "\\end{document}\n");

    fclose(file);

    printf("Simple LaTeX file created: %s\n", filename);
    printf("To compile: pdflatex -interaction=nonstopmode %s\n", filename);

    return TREE_ERROR_NO;
}
