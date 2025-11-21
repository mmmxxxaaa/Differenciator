#include <stdio.h>
#include <stdlib.h>
#include "io_diff.h"
#include "dump.h"
#include "operations.h"
#include "variable_parse.h"
#include "tree_base.h"
#include "tree_common.h"
#include "latex_dump.h"
#include "user_interface.h"

int main()
{
    Tree tree = {};
    TreeCtor(&tree);

    VariableTable var_table = {};
    InitVariableTable(&var_table);

    double result = 0.0;

    InitTreeLog("differenciator_tree");
    InitTreeLog("differentiator_parse");

    TreeErrorType error = TreeLoad(&tree, "differenciator_tree.txt");

    if (error == TREE_ERROR_NO)
    {
        printf("Дерево успешно загружено!\n");
        TreeBaseDump(&tree);

        printf("\n=== ВЫЧИСЛЕНИЕ ВЫРАЖЕНИЯ ===\n");
        error = EvaluateTree(&tree, &var_table, &result);

        if (error == TREE_ERROR_NO)
        {
            printf("Результат вычисления: %.6f\n", result);
        }
        else
        {
            printf("Ошибка вычисления: %d\n", error);
            PrintTreeError(error);
        }

        printf("\n=== ВЫЧИСЛЕНИЕ ПРОИЗВОДНЫХ ===\n");

        Tree   derivative_trees[kMaxNumberOfDerivative] = {}; //FIXME мб это тоже в структурку?
        double derivative_results[kMaxNumberOfDerivative] = {};
        int    actual_derivative_count = 0;

        Tree* current_tree = &tree;
        for (int i = 0; i < kMaxNumberOfDerivative; i++)
        {
            TreeCtor(&derivative_trees[i]);

            error = DifferentiateTree(current_tree, "x", &derivative_trees[i]);
            if (error != TREE_ERROR_NO)
            {
                printf("Ошибка вычисления производной порядка %d: %d\n", i + 1, error);
                TreeDtor(&derivative_trees[i]);
                break;
            }

            printf("Производная порядка %d успешно вычислена!\n", i + 1);
            TreeBaseDump(&derivative_trees[i]);

            error = EvaluateTree(&derivative_trees[i], &var_table, &derivative_results[i]);
            if (error == TREE_ERROR_NO)
            {
                printf("Значение производной порядка %d: %.6f\n", i + 1, derivative_results[i]);
                actual_derivative_count++;
            }
            else
            {
                printf("Ошибка вычисления значения производной порядка %d\n", i + 1);
                PrintTreeError(error);
                derivative_results[i] = 0.0;
                actual_derivative_count++;
            }

            current_tree = &derivative_trees[i];
        }

        printf("\n=== ГЕНЕРАЦИЯ LATEX ДОКУМЕНТА ===\n");

        if (actual_derivative_count > 0)
        {
            Tree** derivative_trees_ptr = (Tree**)calloc(actual_derivative_count, sizeof(Tree*)); //массив указателей на деревья производных
            for (int i = 0; i < actual_derivative_count; i++)
                derivative_trees_ptr[i] = &derivative_trees[i];

            error = GenerateLatexDumpWithDerivatives(&tree, derivative_trees_ptr, derivative_results,
                                                   actual_derivative_count, &var_table,
                                                   "expression_analysis.tex", result);

            free(derivative_trees_ptr);
        }

        if (error == TREE_ERROR_NO)
        {
            printf("LaTeX документ успешно создан!\n");
            if (actual_derivative_count > 0)
            {
                printf("Документ содержит производные до %d порядка\n", actual_derivative_count);
            }
        }
        else
        {
            printf("Ошибка создания LaTeX документа: %d\n", error);
        }

        for (int i = 0; i < actual_derivative_count; i++)
        {
            TreeDtor(&derivative_trees[i]);
        }
    }
    else
    {
        printf("Ошибка загрузки дерева: %d\n", error);
        PrintTreeError(error);
    }

    CloseTreeLog("differenciator_tree");
    CloseTreeLog("differentiator_parse");

    DestroyVariableTable(&var_table);
    TreeDtor(&tree);

    return 0;
}
