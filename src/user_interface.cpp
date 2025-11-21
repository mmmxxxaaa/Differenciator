#include "user_interface.h"
#include <assert.h>
#include <stdio.h>
#include "tree_error_types.h"

const char* GetDataBaseFilename(int argc, const char** argv)
{
    assert(argv);

    return (argc < 1) ? kDefaultDataBaseFilename : argv[1];
}


const char* GetTreeErrorString(TreeErrorType error)
{
    switch (error)
    {
        case TREE_ERROR_NO:                 return "Успешное выполнение";
        case TREE_ERROR_DIVISION_BY_ZERO:   return "Деление на ноль!";
        case TREE_ERROR_VARIABLE_NOT_FOUND: return "Переменная не найдена!";
        case TREE_ERROR_VARIABLE_UNDEFINED: return "Переменная не определена!";
        case TREE_ERROR_INVALID_INPUT:      return "Неверный ввод значения переменной!";
        case TREE_ERROR_NULL_PTR:           return "Нулевой указатель!";
        case TREE_ERROR_UNKNOWN_OPERATION:  return "Неизвестная операция!";
        case TREE_ERROR_IO:                 return "Ошибка ввода-вывода (файл не найден или недоступен)";
        case TREE_ERROR_FORMAT:             return "Ошибка формата файла";
        case TREE_ERROR_ALLOCATION:         return "Ошибка выделения памяти";
        case TREE_ERROR_NO_VARIABLES:       return "Переменные не найдены";
        default:                            return "Неизвестная ошибка";
    }
}

void PrintTreeError(TreeErrorType error)
{
    printf("%s\n", GetTreeErrorString(error));
}
