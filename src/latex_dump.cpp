#include "latex_dump.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "logic_functions.h"

static const OpFormat formats[OP_COUNT] = {
    /* OP_ADD */    {"", " + ", "",        true,  true,  false},
    /* OP_SUB */    {"", " - ", "",        true,  true,  true },
    /* OP_MUL */    {"", " \\cdot ", "",   true,  true,  false},
    /* OP_DIV */    {"\\frac{", "}{", "}", false, true,  false},
    /* OP_POW */    {"{", "}^{", "}",      false, true,  false},
    /* OP_SIN */    {"\\sin(", "", ")",    false, false, false},
    /* OP_COS */    {"\\cos(", "", ")",    false, false, false},
    /* OP_TAN */    {"\\tan(", "", ")",    false, false, false},
    /* OP_COT */    {"\\cot(", "", ")",    false, false, false},
    /* OP_ARCSIN */ {"\\arcsin(", "", ")", false, false, false},
    /* OP_ARCCOS */ {"\\arccos(", "", ")", false, false, false},
    /* OP_ARCTAN */ {"\\arctan(", "", ")", false, false, false},
    /* OP_ARCCOT */ {"\\arccot(", "", ")", false, false, false},
    /* OP_SINH */   {"\\sinh(", "", ")",   false, false, false},
    /* OP_COSH */   {"\\cosh(", "", ")",   false, false, false},
    /* OP_TANH */   {"\\tanh(", "", ")",   false, false, false},
    /* OP_COTH */   {"\\coth(", "", ")",   false, false, false},
    /* OP_LN */     {"\\ln(", "", ")",     false, false, false},
    /* OP_EXP */    {"e^{", "", "}",       false, false, false}
};

const OpFormat* GetOpFormat(OperationType op_type)
{
    if (op_type >= 0 && op_type < OP_COUNT)
    {
        return &formats[op_type];
    }
    return NULL;
}

void TreeToStringSimple(Node* node, char* buffer, int* pos, int buffer_size)
{
    if (node == NULL || *pos >= buffer_size - 1)
        return;

    switch (node->type)
    {
        case NODE_NUM:
            // для отрицательных чисел всегда добавляем скобки
            if (node->data.num_value < 0)
                *pos += snprintf(buffer + *pos, buffer_size - *pos, "(%g)", node->data.num_value);
            else
                *pos += snprintf(buffer + *pos, buffer_size - *pos, "%g", node->data.num_value);
            break;

        case NODE_VAR:
            if (node->data.var_definition.name)
                *pos += snprintf(buffer + *pos, buffer_size - *pos, "%s", node->data.var_definition.name);
            else
                *pos += snprintf(buffer + *pos, buffer_size - *pos, "?");
            break;

        case NODE_OP:
        {
            const OpFormat* fmt = GetOpFormat(node->data.op_value);
            if (fmt == NULL)
            {
                *pos += snprintf(buffer + *pos, buffer_size - *pos, "?");
                break;
            }

            if (!fmt->is_binary)
            {
                *pos += snprintf(buffer + *pos, buffer_size - *pos, "%s", fmt->prefix);
                TreeToStringSimple(node->right, buffer, pos, buffer_size);
                *pos += snprintf(buffer + *pos, buffer_size - *pos, "%s", fmt->postfix);
                break;
            }

            if (!fmt->should_compare_priority)
            {
                // простые бинарные операторы (деление, степень)
                *pos += snprintf(buffer + *pos, buffer_size - *pos, "%s", fmt->prefix);
                TreeToStringSimple(node->left, buffer, pos, buffer_size);
                *pos += snprintf(buffer + *pos, buffer_size - *pos, "%s", fmt->infix);
                TreeToStringSimple(node->right, buffer, pos, buffer_size);
                *pos += snprintf(buffer + *pos, buffer_size - *pos, "%s", fmt->postfix);
                break;
            }

            // сложные бинарные операторы (с проверкой приоритетов)
            bool left_needs_parentheses = IsNodeType(node->left, NODE_OP) &&
                                          (node->left->priority < node->priority);

            bool right_needs_parentheses = false;
            if (IsNodeType(node->right, NODE_OP))
            {
                if (fmt->right_use_less_equal)
                    right_needs_parentheses = (node->right->priority <= node->priority);
                else
                    right_needs_parentheses = (node->right->priority < node->priority);
            }

            // левый аргумент
            if (left_needs_parentheses)
            {
                *pos += snprintf(buffer + *pos, buffer_size - *pos, "(");
                TreeToStringSimple(node->left, buffer, pos, buffer_size);
                *pos += snprintf(buffer + *pos, buffer_size - *pos, ")");
            }
            else
            {
                TreeToStringSimple(node->left, buffer, pos, buffer_size);
            }

            // сам оператор
            if (fmt->infix[0] != '\0')
                *pos += snprintf(buffer + *pos, buffer_size - *pos, "%s", fmt->infix);

            // правый аргумент
            if (right_needs_parentheses)
            {
                *pos += snprintf(buffer + *pos, buffer_size - *pos, "(");
                TreeToStringSimple(node->right, buffer, pos, buffer_size);
                *pos += snprintf(buffer + *pos, buffer_size - *pos, ")");
            }
            else
            {
                TreeToStringSimple(node->right, buffer, pos, buffer_size);
            }
            break;
        }

        default:
            *pos += snprintf(buffer + *pos, buffer_size - *pos, "?");
    }
}

char* ConvertLatexToPGFPlot(const char* latex_expr)
{
    if (!latex_expr)
        return NULL;

    size_t len = strlen(latex_expr);
    char* result = (char*)calloc(len * 2 + 1, sizeof(char));
    if (!result)
        return NULL;

    char* dest = result;
    const char* src = latex_expr;

    while (*src)
    {
        if (strncmp(src, "\\sin", 4) == 0)
        {
            strcpy(dest, "sin");
            dest += 3;
            src += 4;
        }
        else if (strncmp(src, "\\cos", 4) == 0)
        {
            strcpy(dest, "cos");
            dest += 3;
            src += 4;
        }
        else if (strncmp(src, "\\tan", 4) == 0)
        {
            strcpy(dest, "tan");
            dest += 3;
            src += 4;
        }
        else if (strncmp(src, "\\ln", 3) == 0)
        {
            strcpy(dest, "ln");
            dest += 2;
            src += 3;
        }
        else if (strncmp(src, "\\cdot", 5) == 0)
        {
            *dest++ = '*';
            src += 5;
        }
        else if (strncmp(src, "\\frac", 5) == 0)
        {
            *dest++ = '(';
            src += 5;
            while (*src && *src != '}')
            {
                *dest++ = *src++;
            }
            if (*src == '}')
            {
                *dest++ = ')';
                src++;
            }
            *dest++ = '/';
            *dest++ = '(';
            while (*src && *src != '}')
            {
                *dest++ = *src++;
            }
            if (*src == '}')
            {
                *dest++ = ')';
                src++;
            }
        }
        else if (strncmp(src, "e^{", 3) == 0)
        {
            strcpy(dest, "exp(");
            dest += 4;
            src += 3;
        }
        else if (*src == '^' && src[1] == '{')
        {
            *dest++ = '^';
            *dest++ = '(';
            src += 2;
        }
        else if (*src == '{')
        {
            *dest++ = '(';
            src++;
        }
        else if (*src == '}')
        {
            *dest++ = ')';
            src++;
        }
        else if (*src == '\\')
        {
            src++;
        }
        else if (strncmp(src, "\\tan", 4) == 0)
        {
            strcpy(dest, "tan");
            dest += 3;
            src += 4;
        }
        else if (strncmp(src, "\\cot", 4) == 0)
        {
            strcpy(dest, "cot");
            dest += 3;
            src += 4;
        }
        else if (strncmp(src, "\\arcsin", 7) == 0)
        {
            strcpy(dest, "asin");
            dest += 4;
            src += 7;
        }
        else if (strncmp(src, "\\arccos", 7) == 0)
        {
            strcpy(dest, "acos");
            dest += 4;
            src += 7;
        }
        else if (strncmp(src, "\\arctan", 7) == 0)
        {
            strcpy(dest, "atan");
            dest += 4;
            src += 7;
        }
        else if (strncmp(src, "\\arccot", 7) == 0)
        {
            strcpy(dest, "atan");
            dest += 4;
            src += 7;
        }
        else if (strncmp(src, "\\sinh", 5) == 0)
        {
            strcpy(dest, "sinh");
            dest += 4;
            src += 5;
        }
        else if (strncmp(src, "\\cosh", 5) == 0)
        {
            strcpy(dest, "cosh");
            dest += 4;
            src += 5;
        }
        else if (strncmp(src, "\\tanh", 5) == 0)
        {
            strcpy(dest, "tanh");
            dest += 4;
            src += 5;
        }
        else if (strncmp(src, "\\coth", 5) == 0)
        {
            strcpy(dest, "coth");
            dest += 4;
            src += 5;
        }
        else
        {
            *dest++ = *src++;
        }
    }

    *dest = '\0';
    return result;
}

TreeErrorType StartLatexDump(FILE* file)
{
    if (file == NULL)
        return TREE_ERROR_NULL_PTR;

    static const char* document_setup =
        "\\documentclass[12pt]{article}\n"
        "\\usepackage[utf8]{inputenc}\n"
        "\\usepackage{amsmath}\n"
        "\\usepackage{breqn}\n"
        "\\usepackage{pgfplots}\n"
        "\\pgfplotsset{compat=1.18}\n"
        "\\usepackage{geometry}\n"
        "\\geometry{a4paper, left=20mm, right=20mm, top=20mm, bottom=20mm}\n"
        "\\setlength{\\parindent}{0pt}\n"
        "\\setlength{\\parskip}{1em}\n"
        "\\begin{document}\n";

    static const char* title_page =
        "\\begin{titlepage}\n"
        "\\centering\n"
        "\\vspace*{2cm}\n"
        "{\\Huge \\textbf{Mathematical Expression Analysis}}\\par\n"
        "\\vspace{1cm}\n"
        "{\\Large Automatic Differentiation and Optimization}\\par\n"
        "\\vspace{2cm}\n"
        "{\\large Automatically generated report}\\par\n"
        "\\vspace{1cm}\n"
        "{\\large \\today}\\par\n"
        "\\vfill\n"
        "{\\large Author: Katkov Maksim Alekseevich}\\par\n"
        "\\end{titlepage}\n\n"
        "\\vspace{1cm}\n";

    static const char* intro =
        "\\section*{Introduction}\n"
        "\\addcontentsline{toc}{section}{Introduction}\n"
        "This document presents a complete analysis of a mathematical expression, including:\n"
        "\\begin{itemize}\n"
        "\\item Original expression and its evaluation\n"
        "\\item Optimization and simplification process\n"
        "\\item \\textbf{Lots of derivatives} of various orders\n"
        "\\item Variable table with their values\n"
        "\\end{itemize}\n"
        "\\newpage\n";

    fprintf(file, "%s", document_setup);
    fprintf(file, "%s", title_page);
    fprintf(file, "%s", intro);

    return TREE_ERROR_NO;
}

TreeErrorType AddFunctionPlot(DifferentiatorStruct* diff_struct, const char* diff_variable)
{
    if (!diff_struct || !diff_variable)
        return TREE_ERROR_NULL_PTR;

    double x_min = -10.0, x_max = 10.0;
    int num_points = 200;

    printf("\n=== Function Plot Generation ===\n");
    printf("Function: ");

    char expression[kMaxLengthOfTexExpression] = {0};
    int pos = 0;
    TreeToStringSimple(diff_struct->tree.root, expression, &pos, sizeof(expression));
    printf("%s\n", expression);

    printf("Plot variable: %s\n", diff_variable);
    printf("Enter plot range (min max, e.g., -10 10): ");

    if (scanf("%lf %lf", &x_min, &x_max) != 2)
    {
        printf("Using default range: [-10, 10]\n");
        x_min = -10.0;
        x_max = 10.0;
    }

    printf("Enter number of points (default 200): ");
    if (scanf("%d", &num_points) != 1 || num_points <= 0)
    {
        num_points = 200;
    }

    int c = 0;
    while ((c = getchar()) != '\n' && c != EOF);

    char* pgf_expr = ConvertLatexToPGFPlot(expression);
    if (!pgf_expr)
    {
        fprintf(stderr, "Error converting expression to PGFPlots format\n");
        return TREE_ERROR_MEMORY;
    }

    printf("PGFPlots expression: %s\n", pgf_expr);

    fprintf(diff_struct->tex_file, "\\section*{Function Plot}\n");
    fprintf(diff_struct->tex_file, "Plot of function $f(%s) = %s$ in range $[%.2f, %.2f]$.\n\n",
            diff_variable, expression, x_min, x_max);

    fprintf(diff_struct->tex_file, "\\begin{figure}[h]\n");
    fprintf(diff_struct->tex_file, "\\centering\n");
    fprintf(diff_struct->tex_file, "\\begin{tikzpicture}\n");
    fprintf(diff_struct->tex_file, "\\begin{axis}[\n");
    fprintf(diff_struct->tex_file, "    width=0.8\\textwidth,\n");
    fprintf(diff_struct->tex_file, "    height=0.6\\textwidth,\n");
    fprintf(diff_struct->tex_file, "    axis lines = middle,\n");
    fprintf(diff_struct->tex_file, "    xlabel = {$%s$},\n", diff_variable);
    fprintf(diff_struct->tex_file, "    ylabel = {$f(%s)$},\n", diff_variable);
    fprintf(diff_struct->tex_file, "    grid = major,\n");
    fprintf(diff_struct->tex_file, "    grid style = {dashed, gray!30},\n");
    fprintf(diff_struct->tex_file, "    legend pos = north west,\n");
    fprintf(diff_struct->tex_file, "    title = {Function Plot},\n");
    fprintf(diff_struct->tex_file, "    domain = %.2f:%.2f,\n", x_min, x_max);
    fprintf(diff_struct->tex_file, "    samples = %d,\n", num_points);
    fprintf(diff_struct->tex_file, "    smooth,\n");
    fprintf(diff_struct->tex_file, "    trig format=rad\n");
    fprintf(diff_struct->tex_file, "]\n");

    fprintf(diff_struct->tex_file, "\\addplot[blue, thick] {%s};\n", pgf_expr);
    fprintf(diff_struct->tex_file, "\\addlegendentry{$f(%s) = %s$}\n", diff_variable, expression);

    fprintf(diff_struct->tex_file, "\\end{axis}\n");
    fprintf(diff_struct->tex_file, "\\end{tikzpicture}\n");
    fprintf(diff_struct->tex_file, "\\caption{Plot of $f(%s) = %s$}\n",
            diff_variable, expression);
    fprintf(diff_struct->tex_file, "\\end{figure}\n");
    fprintf(diff_struct->tex_file, "\\vspace{1cm}\n\n");

    free(pgf_expr);
    printf("Plot successfully added to document.\n");

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
    fprintf(file, "Expression:\n");
    fprintf(file, "\\begin{dmath} %s \\end{dmath}\n\n", expression);
    fprintf(file, "Evaluation result:\n");
    fprintf(file, "\\begin{dmath} %.6f \\end{dmath}\n\n", result_value);

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

    fprintf(file, "\\begin{dmath} %s \\end{dmath}\n\n", expression);
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
    char custom_notation[kMaxCustomNotationLength] = {0};

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
    fprintf(file, "Derivative:\n");
    fprintf(file, "\\begin{dmath} %s = %s \\end{dmath}\n\n", derivative_notation, derivative_expr);
    fprintf(file, "Value of derivative at point:\n");
    fprintf(file, "\\begin{dmath} %s = %.6f \\end{dmath}\n\n", derivative_notation, derivative_result);

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
