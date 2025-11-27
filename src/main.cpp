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

    TreeErrorType error_loading = TreeLoad(&tree, input_file);

    FILE* tex_file = fopen("full_analysis.tex", "w");
    if (!tex_file)
    {
        printf("Error: failed to create file full_analysis.tex\n");
        return 1;
    }

    StartLatexDump(tex_file);

    if (error_loading == TREE_ERROR_NO)
    {
        TreeBaseDump(&tree);

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
                // if (i >= 2)
                //     break;
            }

            free(diff_variable);

            for (int i = 0; i < actual_derivative_count; i++)
            {
                TreeDtor(&derivative_trees[i]);
            }
        }
        else
        {
            fprintf(tex_file, "Failed to select variable for differentiation.\n\n");
        }
    }
    else
    {
        fprintf(tex_file, "\\section*{Error}\n");
        fprintf(tex_file, "Failed to load tree from file: %s\n", input_file);
    }

    EndLatexDump(tex_file);
    fclose(tex_file);

    printf("Full analysis saved to file: full_analysis.tex\n");
    printf("To compile: pdflatex full_analysis.tex\n");

    CloseTreeLog("differenciator_tree");
    CloseTreeLog("differentiator_parse");

    DestroyVariableTable(&var_table);
    TreeDtor(&tree);

    return 0;
}
