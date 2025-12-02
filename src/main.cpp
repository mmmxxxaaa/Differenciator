#include <stdio.h>
#include <stdlib.h>
#include "dump.h"
#include "processing_diff.h"
#include "tree_error_types.h"
#include "user_interface.h"


int main(int argc, const char** argv)
{
    DifferentiatorStruct* diff_struct = CreateDifferentiatorStruct();
    if (!diff_struct)
    {
        fprintf(stderr, "Критическая ошибка: не удалось создать структуру дифференциатора\n");
        return 1;
    }

    InitTreeLog("differenciator_tree");
    InitTreeLog("differentiator_parse");

    TreeErrorType error = TREE_ERROR_NO;

    if (error == TREE_ERROR_NO) error = InitializeExpression(diff_struct, argc, argv);

    if (error == TREE_ERROR_NO) error = ParseExpressionTree(diff_struct);

    if (error == TREE_ERROR_NO) error = InitializeLatexOutput(diff_struct);

    if (error == TREE_ERROR_NO) error = RequestVariableValues(diff_struct);

    if (error == TREE_ERROR_NO) error = EvaluateOriginalFunction(diff_struct);

    if (error == TREE_ERROR_NO) error = OptimizeExpressionTree(diff_struct);

    // строим график (если возможно)
    if (error == TREE_ERROR_NO)
    {
        TreeErrorType plot_error = PlotFunctionGraph(diff_struct);
        if (plot_error != TREE_ERROR_NO && plot_error != TREE_ERROR_NO_VARIABLES)
        {
            fprintf(stderr, "He удалось построить график: %s\n", GetTreeErrorString(plot_error));
        }
    }

    if (error == TREE_ERROR_NO) error = PerformDifferentiationProcess(diff_struct);

    if (error == TREE_ERROR_NO && diff_struct->tex_file)
    {
        error = FinalizeLatexOutput(diff_struct);
    }

    CloseTreeLog("differenciator_tree");
    CloseTreeLog("differentiator_parse");

    if (error == TREE_ERROR_NO)
    {
        printf("\n Программа успешно завершена!\n");
    }
    else
    {
        fprintf(stderr, "\n Программа завершилась c ошибкой:\n");
        fprintf(stderr, "  Код ошибки: %d\n", error);
        fprintf(stderr, "  Описание: %s\n", GetTreeErrorString(error));
    }

    DestroyDifferentiatorStruct(diff_struct);

    return (error == TREE_ERROR_NO) ? 0 : 1;
}
