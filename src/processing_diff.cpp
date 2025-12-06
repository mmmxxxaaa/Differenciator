#include "processing_diff.h"
#include "io_diff.h"
#include "dump.h"
#include "operations.h"
#include "tree_common.h"
#include "latex_dump.h"
#include "user_interface.h"
#include "new_great_input.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==================== CONTEXT MANAGEMENT FUNCTIONS ====================

DifferentiatorStruct* CreateDifferentiatorStruct()
{
    DifferentiatorStruct* diff_struct = (DifferentiatorStruct*)calloc(1, sizeof(DifferentiatorStruct));
    if (!diff_struct) return NULL;

    TreeCtor(&diff_struct->tree);
    InitVariableTable(&diff_struct->var_table);

    return diff_struct;
}

void DestroyDifferentiatorStruct(DifferentiatorStruct* diff_struct)
{
    if (!diff_struct) return;

    if (diff_struct->expression) free(diff_struct->expression);
    if (diff_struct->tex_file) fclose(diff_struct->tex_file);

    DestroyVariableTable(&diff_struct->var_table);
    TreeDtor(&diff_struct->tree);
    free(diff_struct);
}

// ==================== MAIN PROCESSING FUNCTIONS ====================

TreeErrorType InitializeExpression(DifferentiatorStruct* diff_struct, int argc, const char** argv)
{
    if (!diff_struct) return TREE_ERROR_NULL_PTR;

    const char* input_file = GetDataBaseFilename(argc, argv);
    if (!input_file)
    {
        return TREE_ERROR_NO_VARIABLES;
    }

    diff_struct->expression = ReadExpressionFromFile(input_file);
    if (!diff_struct->expression)
    {
        return TREE_ERROR_OPENING_FILE;
    }

    printf("Expression from file: %s\n", diff_struct->expression);
    return TREE_ERROR_NO;
}

TreeErrorType ParseExpressionTree(DifferentiatorStruct* diff_struct)
{
    if (!diff_struct || !diff_struct->expression) return TREE_ERROR_NULL_PTR;

    const char* ptr = diff_struct->expression;
    diff_struct->tree.root = GetGovnoNaBosuNogu(&ptr, &diff_struct->var_table);

    if (!diff_struct->tree.root)
    {
        return TREE_ERROR_FORMAT;
    }

    diff_struct->tree.size = CountTreeNodes(diff_struct->tree.root);
    printf("Successfully parsed expression. Tree size: %zu\n", diff_struct->tree.size);

    return TREE_ERROR_NO;
}

TreeErrorType InitializeLatexOutput(DifferentiatorStruct* diff_struct)
{
    if (!diff_struct) return TREE_ERROR_NULL_PTR;

    diff_struct->tex_file = fopen(kTexFilename, "w");
    if (!diff_struct->tex_file)
    {
        return TREE_ERROR_OPENING_FILE;
    }

    StartLatexDump(diff_struct->tex_file);
    return TREE_ERROR_NO;
}

TreeErrorType RequestVariableValues(DifferentiatorStruct* diff_struct)
{
    if (!diff_struct) return TREE_ERROR_NULL_PTR;

    for (int i = 0; i < diff_struct->var_table.number_of_variables; i++)
    {
        TreeErrorType error = RequestVariableValue(&diff_struct->var_table, diff_struct->var_table.variables[i].name);
        if (error != TREE_ERROR_NO)
        {
            return error;
        }
    }

    return TREE_ERROR_NO;
}

TreeErrorType EvaluateOriginalFunction(DifferentiatorStruct* diff_struct)
{
    if (!diff_struct) return TREE_ERROR_NULL_PTR;

    TreeErrorType error = EvaluateTree(&diff_struct->tree, &diff_struct->var_table, &diff_struct->result);
    if (error != TREE_ERROR_NO)
    {
        return error;
    }

    printf("Calculation result: %.6f\n", diff_struct->result);

    if (diff_struct->tex_file)
    {
        DumpOriginalFunctionToFile(diff_struct->tex_file, &diff_struct->tree, diff_struct->result);
        DumpVariableTableToFile(diff_struct->tex_file, &diff_struct->var_table);
    }

    return TREE_ERROR_NO;
}

TreeErrorType OptimizeExpressionTree(DifferentiatorStruct* diff_struct)
{
    if (!diff_struct) return TREE_ERROR_NULL_PTR;

    size_t size_before = CountTreeNodes(diff_struct->tree.root);

    TreeErrorType error = OptimizeTreeWithDump(&diff_struct->tree, diff_struct->tex_file, &diff_struct->var_table);
    if (error != TREE_ERROR_NO)
    {
        return error;
    }

    size_t size_after = CountTreeNodes(diff_struct->tree.root);
    printf("Optimization: %zu -> %zu nodes\n", size_before, size_after);

    if (size_before != size_after)
    {
        error = EvaluateTree(&diff_struct->tree, &diff_struct->var_table, &diff_struct->result);
        if (error == TREE_ERROR_NO)
        {
            printf("Result after optimization: %.6f\n", diff_struct->result);
        }
        return error;
    }

    return TREE_ERROR_NO;
}

TreeErrorType PerformDifferentiationProcess(DifferentiatorStruct* diff_struct)
{
    if (!diff_struct || !diff_struct->tex_file)
        return TREE_ERROR_NULL_PTR;

    char original_expr[kMaxLengthOfTexExpression] = {0};
    int pos = 0;
    TreeToStringSimple(diff_struct->tree.root, original_expr, &pos, sizeof(original_expr));

    fprintf(diff_struct->tex_file, "\\section*{Differentiation}\n");

    char* diff_variable = SelectDifferentiationVariable(&diff_struct->var_table);
    if (!diff_variable)
    {
        fprintf(diff_struct->tex_file, "Failed to select variable for differentiation.\\newline\n\n");
        return TREE_ERROR_NO_VARIABLES;
    }

    fprintf(diff_struct->tex_file, "Differentiation variable: \\[ %s \\]\\newline\n\n", diff_variable);

    TreeErrorType plot_error = AddFunctionPlot(diff_struct, diff_variable);
    if (plot_error != TREE_ERROR_NO)
    {
        fprintf(diff_struct->tex_file, "\\textbf{Note:} Could not create function plot.\\newline\n");
    }

    Tree derivative_trees[kMaxNumberOfDerivative] = {};
    double derivative_results[kMaxNumberOfDerivative] = {};
    int actual_derivative_count = 0;

    Tree* current_tree = &diff_struct->tree;

    for (int i = 0; i < kMaxNumberOfDerivative; i++)
    {
        TreeCtor(&derivative_trees[i]);

        TreeErrorType error = DifferentiateTree(current_tree, diff_variable, &derivative_trees[i]);
        if (error != TREE_ERROR_NO)
        {
            TreeDtor(&derivative_trees[i]);
            break;
        }

        fprintf(diff_struct->tex_file, "\\subsection*{Derivative %d Optimization}\n", i + 1);
        error = OptimizeTreeWithDump(&derivative_trees[i], diff_struct->tex_file, &diff_struct->var_table);

        error = EvaluateTree(&derivative_trees[i], &diff_struct->var_table, &derivative_results[i]);
        if (error == TREE_ERROR_NO)
        {
            printf("Derivative %d: %.6f\n", i + 1, derivative_results[i]);
            actual_derivative_count++;

            fprintf(diff_struct->tex_file, "Original expression:\n");
            fprintf(diff_struct->tex_file, "\\begin{dmath} f(x) = %s \\end{dmath}\\newline\n\n", original_expr);

            fprintf(diff_struct->tex_file, "Optimized derivative:\n");

            DumpDerivativeToFile(diff_struct->tex_file, &derivative_trees[i], derivative_results[i], i + 1);
        }

        current_tree = &derivative_trees[i];
    }

    free(diff_variable);

    for (int i = 0; i < actual_derivative_count; i++)
    {
        TreeDtor(&derivative_trees[i]);
    }

    return TREE_ERROR_NO;
}

TreeErrorType FinalizeLatexOutput(DifferentiatorStruct* diff_struct)
{
    if (!diff_struct) return TREE_ERROR_NULL_PTR;

    if (diff_struct->tex_file)
    {
        EndLatexDump(diff_struct->tex_file);
        fclose(diff_struct->tex_file);
        diff_struct->tex_file = NULL;

        printf("Full analysis saved to file: %s\n", kTexFilename);
        printf("To compile: pdflatex %s\n", kTexFilename);
    }

    return TREE_ERROR_NO;
}
