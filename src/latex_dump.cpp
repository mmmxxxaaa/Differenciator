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

static TreeErrorType DumpOriginalFunction(FILE* file, Tree* tree, double result_value)
{
    if (file == NULL || tree == NULL)
        return TREE_ERROR_NULL_PTR;

    char expression[kMaxLengthOfTexExpression] = {0};
    int pos = 0;
    TreeToStringSimple(tree->root, expression, &pos, sizeof(expression));

    fprintf(file, "\\section*{Mathematical Expression}\n\n");
    fprintf(file, "Expression: \\[ %s \\]\n\n", expression);
    fprintf(file, "Result: \\[ %.6f \\]\n\n", result_value);

    return TREE_ERROR_NO;
}

static TreeErrorType DumpDerivative(FILE* file, Tree* derivative_tree, double derivative_result, int derivative_order)
{
    if (file == NULL || derivative_tree == NULL)
        return TREE_ERROR_NULL_PTR;

    char derivative_expr[kMaxLengthOfTexExpression] = {0}; //FIXME
    int pos = 0;
    TreeToStringSimple(derivative_tree->root, derivative_expr, &pos, sizeof(derivative_expr));

    const char* derivative_notation = NULL;
    char custom_notation[32] = {0}; //FIXME

    if (derivative_order == 1)
    {
        derivative_notation = "f'(x)";
    }
    else if (derivative_order == 2)
    {
        derivative_notation = "f''(x)";
    }
    else if (derivative_order == 3)
    {
        derivative_notation = "f'''(x)";
    }
    else
    {
        snprintf(custom_notation, sizeof(custom_notation), "f^{(%d)}(x)", derivative_order);
        derivative_notation = custom_notation;
    }

    fprintf(file, "\\subsection*{Derivative of Order %d}\n", derivative_order);
    fprintf(file, "Derivative: \\[ %s = %s \\]\n\n", derivative_notation, derivative_expr);
    fprintf(file, "Value of derivative at point: \\[ %s = %.6f \\]\n\n", derivative_notation, derivative_result);

    return TREE_ERROR_NO;
}

static TreeErrorType DumpVariableTable(FILE* file, VariableTable* var_table)
{
    if (file == NULL || var_table == NULL)
        return TREE_ERROR_NULL_PTR;

    if (var_table->number_of_variables <= 0)
        return TREE_ERROR_NO;

    fprintf(file, "\\section*{Variables}\n");
    fprintf(file, "\\begin{tabular}{|c|c|}\n");
    fprintf(file, "\\hline\n");
    fprintf(file, "Name & Value \\\\\n");
    fprintf(file, "\\hline\n");

    for (int i = 0; i < var_table->number_of_variables; i++) //вывод таблицы переменных
    {
        fprintf(file, "%s & %.4f \\\\\n", var_table->variables[i].name, var_table->variables[i].value);
    }

    fprintf(file, "\\hline\n");
    fprintf(file, "\\end{tabular}\n\n");

    return TREE_ERROR_NO;
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

    TreeErrorType error = DumpOriginalFunction(file, tree, result_value);
    if (error != TREE_ERROR_NO)
    {
        fclose(file);
        return error;
    }

    error = DumpVariableTable(file, var_table);
    if (error != TREE_ERROR_NO && error != TREE_ERROR_NO_VARIABLES)
    {
        fclose(file);
        return error;
    }

    fprintf(file, "\\end{document}\n");
    fclose(file);

    printf("Simple LaTeX file created: %s\n", filename);
    printf("To compile: pdflatex %s\n", filename);

    return TREE_ERROR_NO;
}

TreeErrorType GenerateLatexDumpWithDerivatives(Tree* tree, Tree** derivative_trees, double* derivative_results,
                                              int derivative_count, VariableTable* var_table,
                                              const char* filename, double result_value)
{
    if (tree == NULL || derivative_trees == NULL || derivative_results == NULL ||
        var_table == NULL || filename == NULL)
        return TREE_ERROR_NULL_PTR;

    FILE* file = fopen(filename, "w");
    if (!file)
        return TREE_ERROR_IO;

    fprintf(file, "\\documentclass{article}\n");
    fprintf(file, "\\usepackage[utf8]{inputenc}\n");
    fprintf(file, "\\usepackage{amsmath}\n");
    fprintf(file, "\\begin{document}\n\n");

    fprintf(file, "\\section*{Mathematical Expression Analysis}\n\n");

    TreeErrorType error = DumpOriginalFunction(file, tree, result_value);
    if (error != TREE_ERROR_NO)
    {
        fclose(file);
        return error;
    }

    for (int i = 0; i < derivative_count; i++)
    {
        error = DumpDerivative(file, derivative_trees[i], derivative_results[i], i + 1);
        if (error != TREE_ERROR_NO)
        {
            fclose(file);
            return error;
        }
    }

    error = DumpVariableTable(file, var_table);
    if (error != TREE_ERROR_NO && error != TREE_ERROR_NO_VARIABLES)
    {
        fclose(file);
        return error;
    }

    fprintf(file, "\\end{document}\n");
    fclose(file);

    printf("LaTeX file with %d derivatives created: %s\n", derivative_count, filename);
    printf("To compile: pdflatex %s\n", filename);

    return TREE_ERROR_NO;
}
