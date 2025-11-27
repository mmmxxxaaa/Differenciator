#include "latex_dump.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void TreeToStringSimple(Node* node, char* buffer, int* pos, int buffer_size)
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
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, " \\cdot ");
                    TreeToStringSimple(node->right, buffer, pos, buffer_size);
                    break;
                case OP_DIV:
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, "\\frac{");
                    TreeToStringSimple(node->left, buffer, pos, buffer_size);
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, "}{");
                    TreeToStringSimple(node->right, buffer, pos, buffer_size);
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, "}");
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
                case OP_POW:
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, "{");
                    TreeToStringSimple(node->left, buffer, pos, buffer_size);
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, "}^{");
                    TreeToStringSimple(node->right, buffer, pos, buffer_size);
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, "}");
                    break;
                case OP_LN:
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, "\\ln(");
                    TreeToStringSimple(node->right, buffer, pos, buffer_size);
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, ")");
                    break;
                case OP_EXP:
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, "e^{");
                    TreeToStringSimple(node->right, buffer, pos, buffer_size);
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, "}");
                    break;
                default:
                    *pos += snprintf(buffer + *pos, buffer_size - *pos, "?");
            }
            break;

        default:
            *pos += snprintf(buffer + *pos, buffer_size - *pos, "?");
    }
}

TreeErrorType StartLatexDump(FILE* file)
{
    if (file == NULL)
        return TREE_ERROR_NULL_PTR;

    fprintf(file, "\\documentclass[12pt]{article}\n");
    fprintf(file, "\\usepackage[utf8]{inputenc}\n");
    fprintf(file, "\\usepackage{amsmath}\n");
    fprintf(file, "\\usepackage{geometry}\n");
    fprintf(file, "\\geometry{a4paper, left=20mm, right=20mm, top=20mm, bottom=20mm}\n");
    fprintf(file, "\\setlength{\\parindent}{0pt}\n");
    fprintf(file, "\\setlength{\\parskip}{1em}\n");
    fprintf(file, "\\begin{document}\n");

    fprintf(file, "\\section*{Mathematical Expression Analysis}\n\n");

    return TREE_ERROR_NO;
}

TreeErrorType EndLatexDump(FILE* file)
{
    if (file == NULL)
        return TREE_ERROR_NULL_PTR;

    fprintf(file, "\\end{document}\n");
    return TREE_ERROR_NO;
}

TreeErrorType DumpOriginalFunctionToFile(FILE* file, Tree* tree, double result_value)
{
    if (file == NULL || tree == NULL)
        return TREE_ERROR_NULL_PTR;

    char expression[kMaxLengthOfTexExpression] = {0};
    int pos = 0;
    TreeToStringSimple(tree->root, expression, &pos, sizeof(expression));

    fprintf(file, "\\subsection*{Original Expression}\n");
    fprintf(file, "Expression: \\[ %s \\]\n\n", expression);
    fprintf(file, "Evaluation result: \\[ %.6f \\]\n\n", result_value);

    return TREE_ERROR_NO;
}

TreeErrorType DumpOptimizationStepToFile(FILE* file, const char* description, Tree* tree, double result_value)
{
    if (file == NULL || description == NULL || tree == NULL)
        return TREE_ERROR_NULL_PTR;

    fprintf(file, "\\subsubsection*{Optimization Step}\n");
    fprintf(file, "It is easy to see that %s:\n\n", description);

    char expression[kMaxLengthOfTexExpression] = {0};
    int pos = 0;
    TreeToStringSimple(tree->root, expression, &pos, sizeof(expression));

    fprintf(file, "\\[ %s \\]\n\n", expression);
    fprintf(file, "Result after simplification: \\[ %.6f \\]\n\n", result_value);
    fprintf(file, "\\vspace{0.5em}\n");

    return TREE_ERROR_NO;
}

TreeErrorType DumpDerivativeToFile(FILE* file, Tree* derivative_tree, double derivative_result, int derivative_order)
{
    if (file == NULL || derivative_tree == NULL)
        return TREE_ERROR_NULL_PTR;

    char derivative_expr[kMaxLengthOfTexExpression] = {0};
    int pos = 0;
    TreeToStringSimple(derivative_tree->root, derivative_expr, &pos, sizeof(derivative_expr));

    const char* derivative_notation = NULL;
    char custom_notation[32] = {0};

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

TreeErrorType DumpVariableTableToFile(FILE* file, VariableTable* var_table)
{
    if (file == NULL || var_table == NULL)
        return TREE_ERROR_NULL_PTR;

    if (var_table->number_of_variables <= 0)
        return TREE_ERROR_NO;

    fprintf(file, "\\section*{Variable Table}\n");
    fprintf(file, "\\begin{tabular}{|c|c|}\n");
    fprintf(file, "\\hline\n");
    fprintf(file, "Name & Value \\\\\n");
    fprintf(file, "\\hline\n");

    for (int i = 0; i < var_table->number_of_variables; i++)
    {
        fprintf(file, "%s & %.4f \\\\\n", var_table->variables[i].name, var_table->variables[i].value);
    }

    fprintf(file, "\\hline\n");
    fprintf(file, "\\end{tabular}\n\n");

    return TREE_ERROR_NO;
}
