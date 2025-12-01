#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "io_diff.h"
#include "dump.h"
#include "operations.h"
#include "variable_parse.h"
#include "tree_base.h"
#include "tree_common.h"
#include "latex_dump.h"
#include "user_interface.h"
#include "new_great_input.h"

char* ReadExpressionFromFile(const char* filename)
{
    FILE* file = fopen(filename, "r");
    if (!file)
    {
        printf("Error: cannot open file %s\n", filename);
        return NULL;
    }

    size_t file_size = GetFileSize(file);

    if (file_size <= 0)
    {
        fclose(file);
        return NULL;
    }

    char* expression = (char*) calloc(file_size + 2, sizeof(char)); // +2 для $ и \0
    if (!expression)
    {
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(expression, 1, file_size, file);
    expression[bytes_read] = '\0';
    fclose(file);

    if (bytes_read > 0 && expression[bytes_read - 1] == '\n')
    {
        expression[bytes_read - 1] = '$';
        expression[bytes_read] = '\0';
    }
    else if (expression[bytes_read - 1] != '$') //ХУЙНЯ это ничего не обрабатывает
    {
        expression[bytes_read] = '$';
        expression[bytes_read + 1] = '\0';
    }

    return expression;
}

int main(int argc, const char** argv)
{
    const char* input_file = GetDataBaseFilename(argc, argv);

    Tree tree = {};
    TreeCtor(&tree);

    VariableTable var_table = {};
    InitVariableTable(&var_table);

    double result = 0.0;

    InitTreeLog("differenciator_tree");
    InitTreeLog("differentiator_parse");

    char* expression = ReadExpressionFromFile(input_file);
    if (!expression)
    {
        printf("Error: failed to read expression from file %s\n", input_file);
        return 1;
    }

    printf("Expression from file: %s\n", expression);

    const char* ptr = expression;
    tree.root = GetG(&ptr, &var_table);

    TreeErrorType error_loading = TREE_ERROR_NO;
    if (!tree.root)
    {
        printf("Error: failed to parse expression\n");
        error_loading = TREE_ERROR_FORMAT;
    }
    else
    {
        tree.size = CountTreeNodes(tree.root);
        printf("Successfully parsed expression. Tree size: %zu\n", tree.size);
    }

    FILE* tex_file = fopen(kTexFilename, "w");
    if (!tex_file)
    {
        printf("Error: failed to create file %s\n", kTexFilename);
        free(expression);
        return 1;
    }

    StartLatexDump(tex_file);

    if (error_loading == TREE_ERROR_NO)
    {
        for (int i = 0; i < var_table.number_of_variables; i++)
            RequestVariableValue(&var_table, var_table.variables[i].name);

        TreeErrorType error = EvaluateTree(&tree, &var_table, &result);
        if (error == TREE_ERROR_NO)
            printf("Calculation result: %.6f\n", result);

        DumpOriginalFunctionToFile(tex_file, &tree, result);
        DumpVariableTableToFile(tex_file, &var_table);

        size_t size_before_optimization = CountTreeNodes(tree.root);
        error = OptimizeTreeWithDump(&tree, tex_file, &var_table);

        if (error == TREE_ERROR_NO)
        {
            size_t size_after_optimization = CountTreeNodes(tree.root);
            printf("Optimization: %zu -> %zu nodes\n",
                   size_before_optimization, size_after_optimization);

            if (size_before_optimization != size_after_optimization)
            {
                error = EvaluateTree(&tree, &var_table, &result);
                if (error == TREE_ERROR_NO)
                    printf("Result after optimization: %.6f\n", result);

            }
        }
//
//         // ==================================================
//         // ОДИН ГРАФИК ФУНКЦИИ
//         // ==================================================
//         bool has_x_variable = false;
//         double x_value = 0.0;
//
//         // Проверяем наличие переменной x в таблице
//         for (int i = 0; i < var_table.number_of_variables; i++)
//         {
//             if (strcmp(var_table.variables[i].name, "x") == 0)
//             {
//                 has_x_variable = true;
//                 x_value = var_table.variables[i].value;
//                 break;
//             }
//         }
//
//         if (has_x_variable)
//         {
//             fprintf(tex_file, "\\section*{Graph of the Function}\n");
//
//             char latex_expr[kMaxLengthOfTexExpression] = {0};
//             int pos = 0;
//             TreeToStringSimple(tree.root, latex_expr, &pos, sizeof(latex_expr));
//
//             char* pgfplot_expr = ConvertLatexToPGFPlot(latex_expr);
//             if (pgfplot_expr)
//             {
//                 printf("Plot expression: %s\n", pgfplot_expr);
//
//                 double x_min = x_value - 5.0; //FIXME пока так
//                 double x_max = x_value + 5.0;
//
//                 AddLatexPlot(tex_file, pgfplot_expr, x_min, x_max, "Function Graph");
//                 free(pgfplot_expr);
//             }
//             else
//             {
//                 fprintf(tex_file, "Failed to convert expression for plotting.\n");
//             }
//         }
//         else
//         {
//             fprintf(tex_file, "\\section*{Graph of the Function}\n");
//             fprintf(tex_file, "Cannot plot graph: variable 'x' not found in expression.\n");
//         }
//         // ==================================================

        fprintf(tex_file, "\\section*{Differentiation}\n");

        char* diff_variable = SelectDifferentiationVariable(&var_table);
        if (diff_variable != NULL)
        {
            fprintf(tex_file, "Differentiation variable: \\[ %s \\]\n\n", diff_variable);

            Tree   derivative_trees[kMaxNumberOfDerivative] = {};
            double derivative_results[kMaxNumberOfDerivative] = {};
            int    actual_derivative_count = 0;

            Tree* current_tree = &tree;
            for (int i = 0; i < kMaxNumberOfDerivative; i++)
            {
                TreeCtor(&derivative_trees[i]);

                error = DifferentiateTree(current_tree, diff_variable, &derivative_trees[i]);
                if (error != TREE_ERROR_NO)
                {
                    TreeDtor(&derivative_trees[i]);
                    break;
                }

                fprintf(tex_file, "\\subsection*{Optimization of derivative %d}\n", i + 1);
                error = OptimizeTreeWithDump(&derivative_trees[i], tex_file, &var_table);

                error = EvaluateTree(&derivative_trees[i], &var_table, &derivative_results[i]);
                if (error == TREE_ERROR_NO)
                {
                    printf("Derivative %d: %.6f\n", i + 1, derivative_results[i]);
                    actual_derivative_count++;

                    DumpDerivativeToFile(tex_file, &derivative_trees[i], derivative_results[i], i + 1);
                }

                current_tree = &derivative_trees[i];
            }

            free(diff_variable);

            for (int i = 0; i < actual_derivative_count; i++)
                TreeDtor(&derivative_trees[i]);
        }
        else
        {
            fprintf(tex_file, "Failed to select variable for differentiation.\n\n");
        }
    }
    else
    {
        fprintf(tex_file, "\\section*{Error}\n");
        fprintf(tex_file, "Failed to parse expression from file: %s\n", input_file);
    }

    EndLatexDump(tex_file);
    fclose(tex_file);

    printf("Full analysis saved to file: full_analysis.tex\n");
    printf("To compile: pdflatex full_analysis.tex\n");

    CloseTreeLog("differenciator_tree");
    CloseTreeLog("differentiator_parse");

    DestroyVariableTable(&var_table);
    TreeDtor(&tree);
    free(expression);

    return 0;
}
