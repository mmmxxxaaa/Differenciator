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

    if (error_loading == TREE_ERROR_NO)
    {
        TreeBaseDump(&tree);

        for (int i = 0; i < var_table.number_of_variables; i++)
            RequestVariableValue(&var_table, var_table.variables[i].name);

        TreeErrorType error = EvaluateTree(&tree, &var_table, &result);
        if (error == TREE_ERROR_NO)
            printf("Результат вычисления: %.6f\n", result);

        size_t size_before_optimization = CountTreeNodes(tree.root);
        error = OptimizeTree(&tree);
        if (error == TREE_ERROR_NO)
        {
            size_t size_after_optimization = CountTreeNodes(tree.root);
            printf("Оптимизация: %zu -> %zu узлов\n",
                   size_before_optimization, size_after_optimization);

            if (size_before_optimization != size_after_optimization)
            {
                error = EvaluateTree(&tree, &var_table, &result);
                if (error == TREE_ERROR_NO)
                    printf("Результат после оптимизации: %.6f\n", result);
            }
        }

        char* diff_variable = SelectDifferentiationVariable(&var_table); //выбираем переменную из таблице, по которой будем дифференцировать

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

            error = OptimizeTree(&derivative_trees[i]);
            error = EvaluateTree(&derivative_trees[i], &var_table, &derivative_results[i]);
            if (error == TREE_ERROR_NO)
            {
                printf("Производная %d: %.6f\n", i + 1, derivative_results[i]);
                actual_derivative_count++;
            }

            current_tree = &derivative_trees[i];
            if (i >= 2)
                break; //FIXME
        }

        if (actual_derivative_count > 0)
        {
            Tree** derivative_trees_ptr = (Tree**)calloc(actual_derivative_count, sizeof(Tree*));
            for (int i = 0; i < actual_derivative_count; i++)
                derivative_trees_ptr[i] = &derivative_trees[i];

            error = GenerateLatexDumpWithDerivatives(&tree, derivative_trees_ptr, derivative_results,
                                                     actual_derivative_count, &var_table,
                                                     "expression_analysis.tex", result);

            free(derivative_trees_ptr);
        }

        free(diff_variable);
        for (int i = 0; i < actual_derivative_count; i++)
        {
            TreeDtor(&derivative_trees[i]);
        }
    }

    CloseTreeLog("differenciator_tree");
    CloseTreeLog("differentiator_parse");

    DestroyVariableTable(&var_table);
    TreeDtor(&tree);

    return 0;
}
