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
                bool left_needs_parentheses = false;
                bool right_needs_parentheses = false;

                if (node->left && node->left->type == NODE_OP)
                {
                    left_needs_parentheses = (node->left->priority < node->priority);
                }

                if (node->right && node->right->type == NODE_OP)
                {
                    right_needs_parentheses = (node->right->priority < node->priority) ||
                                              (node->data.op_value == OP_SUB && node->right->priority <= node->priority) ||
                                              (node->data.op_value == OP_DIV && node->right->priority <= node->priority);
                }

                switch (node->data.op_value)
                {
                    case OP_ADD:
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

                        *pos += snprintf(buffer + *pos, buffer_size - *pos, " + ");

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
                    case OP_SUB:
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

                        *pos += snprintf(buffer + *pos, buffer_size - *pos, " - ");

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

                    case OP_MUL:
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

                        *pos += snprintf(buffer + *pos, buffer_size - *pos, " \\cdot ");

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
                    case OP_DIV:
                        *pos += snprintf(buffer + *pos, buffer_size - *pos, "\\frac{");
                        TreeToStringSimple(node->left,  buffer, pos, buffer_size);
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
            }
            break;

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
            // \frac{a}{b} -> (a)/(b)
            strcpy(dest, "( )/( )");
            dest += 7;
            src += 5;
            while (*src && *src != '}')
                src++;
            if (*src) src++;  // Пропускаем }
            while (*src && *src != '}')
                src++;
            if (*src)
                src++;  // Пропускаем }
        }
        else if (strncmp(src, "e^{", 3) == 0)
        {
            // e^{x} -> exp(x)
            strcpy(dest, "exp(");
            dest += 4;
            src += 3;
        }
        else if (src[0] == '^' && src[1] == '{')
        {
            //^{x} -> ^(x)
            *dest++ = '^';
            *dest++ = '(';
            src += 2;  // Пропускаем ^{
        }
        else if (*src == '{' || *src == '}')
        {
            if (*(src-1) == '^' || *(src+1) == '^')
            {
                // Для степеней {} -> ()
                if (*src == '{') *dest++ = '(';
                if (*src == '}') *dest++ = ')';
            }
            src++;
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

    fprintf(file, "\\documentclass[12pt]{article}\n");
    fprintf(file, "\\usepackage[utf8]{inputenc}\n");
    fprintf(file, "\\usepackage{amsmath}\n");
    fprintf(file, "\\usepackage{breqn}\n");

    fprintf(file, "\\usepackage{pgfplots}\n");
    fprintf(file, "\\pgfplotsset{compat=1.18}\n");

    fprintf(file, "\\usepackage{geometry}\n");
    fprintf(file, "\\geometry{a4paper, left=20mm, right=20mm, top=20mm, bottom=20mm}\n");
    fprintf(file, "\\setlength{\\parindent}{0pt}\n"); //убирает отступы в начале абзацев
    fprintf(file, "\\setlength{\\parskip}{1em}\n");   //устанавливает расстояние между абзацами в 1em
    fprintf(file, "\\begin{document}\n");

    fprintf(file, "\\begin{titlepage}\n");
    fprintf(file, "\\centering\n");
    fprintf(file, "\\vspace*{2cm}\n"); //добавляет вертикальный отступ указанной величины
    fprintf(file, "{\\Huge \\textbf{Mathematical Expression Analysis}}\\par\n");
    fprintf(file, "\\vspace{1cm}\n");
    fprintf(file, "{\\Large Automatic Differentiation and Optimization}\\par\n");
    fprintf(file, "\\vspace{2cm}\n");
    fprintf(file, "{\\large Automatically generated report}\\par\n");
    fprintf(file, "\\vspace{1cm}\n");
    fprintf(file, "{\\large \\today}\\par\n");  //par: после команд изменения шрифта (\Huge, \Large, \large) (для ограничения области их действия)
    fprintf(file, "\\vfill\n");
    fprintf(file, "{\\large Author: Katkov Maksim Alekseevich}\\par\n");
    fprintf(file, "\\end{titlepage}\n\n");

    // fprintf(file, "\\tableofcontents\n");
    fprintf(file, "\\vspace{1cm}\n");

    fprintf(file, "\\section*{Introduction}\n");
    fprintf(file, "\\addcontentsline{toc}{section}{Introduction}\n");
    fprintf(file, "This document presents a complete analysis of a mathematical expression, including:\n");
    fprintf(file, "\\begin{itemize}\n");
    fprintf(file, "\\item Original expression and its evaluation\n");
    fprintf(file, "\\item Optimization and simplification process\n");
    fprintf(file, "\\item \\textbf{Lots of derivatives} of various orders\n");
    fprintf(file, "\\item Variable table with their values\n");
    fprintf(file, "\\end{itemize}\n");
    fprintf(file, "\\newpage\n");

    return TREE_ERROR_NO;
}

TreeErrorType EndLatexDump(FILE* file)
{
    if (file == NULL)
        return TREE_ERROR_NULL_PTR;

    fprintf(file, "\\end{document}\n");
    return TREE_ERROR_NO;
}

TreeErrorType AddLatexPlot(FILE* file, const char* function_formula, //СИГМА СКИБИДИ
                          double x_min, double x_max, const char* title)
{
    if (file == NULL)
        return TREE_ERROR_NULL_PTR;

    fprintf(file, "\\begin{figure}[h]\n");
    fprintf(file, "\\centering\n");
    fprintf(file, "\\begin{tikzpicture}\n");
    fprintf(file, "\\begin{axis}[\n");
    fprintf(file, "    width=0.8\\textwidth,\n"); // Ширина графика"
    fprintf(file, "    height=0.6\\textwidth,\n"); // Высота"
    fprintf(file, "    axis lines = middle,\n");
    fprintf(file, "    xlabel = {$x$},\n");   //Подпись оси X
    fprintf(file, "    ylabel = {$f(x)$},\n"); //Подпись оси Y
    fprintf(file, "    grid = major,\n"); // Включить сетку
    fprintf(file, "    grid style = {dashed, gray!30},\n"); // Стиль сетки
    fprintf(file, "    legend pos = north west,\n"); // Положение легенд
    fprintf(file, "    title = {%s},\n", title);
    fprintf(file, "    domain = %f:%f,\n", x_min, x_max); // Область определения
    fprintf(file, "    samples = 200,\n"); /// Количество точек
    fprintf(file, "    trig format=rad\n"); //Углы в радианах

    fprintf(file, "]\n");

    fprintf(file, "\\addplot[blue, thick, smooth] {%s};\n", function_formula);
    fprintf(file, "\\addlegendentry{$f(x) = %s$}\n", function_formula);

    //FIXME сюда ТЕЙЛОРА

    fprintf(file, "\\end{axis}\n");
    fprintf(file, "\\end{tikzpicture}\n");
    fprintf(file, "\\caption{Plot: $f(x) = %s$}\n", function_formula);
    fprintf(file, "\\end{figure}\n");
    fprintf(file, "\\vspace{1cm}\n");

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
    // fprintf(file, "Result after simplification:\n");
    // fprintf(file, "\\begin{dmath} %.6f \\end{dmath}\n\n", result_value);
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
    char custom_notation[kMaxCustomNotationLength] = {0}; //FIXME

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
