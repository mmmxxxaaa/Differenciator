#include "io_diff.h"
#include "dump.h"
#include "operations.h"
#include "variable_parse.h"
#include <stdio.h>
#include "tree_base.h"
#include "tree_common.h"
//FIXME разобраться с именами файлов
int main()
{
    Tree tree = {};
    VariableTable var_table = {};
    double result = 0.0;

    InitVariableTable(&var_table);

    InitTreeLog("differenciator_tree");
    InitTreeLog("akinator_parse");

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
            switch (error)
            {
                case TREE_ERROR_DIVISION_BY_ZERO:
                    printf("Деление на ноль!\n");
                    break;
                case TREE_ERROR_VARIABLE_NOT_FOUND:
                    printf("Переменная не найдена!\n");
                    break;
                case TREE_ERROR_VARIABLE_UNDEFINED:
                    printf("Переменная не определена!\n");
                    break;
                case TREE_ERROR_INVALID_INPUT:
                    printf("Неверный ввод значения переменной!\n");
                    break;
                case TREE_ERROR_NULL_PTR:
                    printf("Нулевой указатель!\n");
                    break;
                case TREE_ERROR_UNKNOWN_OPERATION:
                    printf("Неизвестная операция!\n");
                    break;
                default:
                    printf("Неизвестная ошибка вычисления\n");
            }
        }
    }
    else
    {
        printf("Ошибка загрузки дерева: %d\n", error);
        switch (error)
        {
            case TREE_ERROR_NULL_PTR:
                printf("Нулевой указатель\n");
                break;
            case TREE_ERROR_IO:
                printf("Ошибка ввода-вывода (файл не найден или недоступен)\n");
                break;
            case TREE_ERROR_FORMAT:
                printf("Ошибка формата файла\n");
                break;
            case TREE_ERROR_ALLOCATION:
                printf("Ошибка выделения памяти\n");
                break;
            default:
                printf("Неизвестная ошибка загрузки\n");
        }
    }

    CloseTreeLog("differenciator_tree");
    CloseTreeLog("akinator_parse");

    DestroyVariableTable(&var_table);
    TreeDtor(&tree);

    return 0;
}
