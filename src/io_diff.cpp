#include "io_diff.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dump.h"
#include "tree_base.h"
#include "operations.h"

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

    char* expression = (char*)calloc(file_size + 2, sizeof(char)); // +2 для $ и \0
    if (!expression)
    {
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(expression, 1, file_size, file);
    expression[bytes_read] = '\0';
    fclose(file);

    if (bytes_read > 0)
    {
        if (expression[bytes_read - 1] == '\n')
        {
            expression[bytes_read - 1] = '$';
            expression[bytes_read] = '\0';
        }
        else
        {
            expression[bytes_read] = '$';
            expression[bytes_read + 1] = '\0';
        }
    }
    else
    {
        expression[0] = '$';
        expression[1] = '\0';
    }

    return expression;
}

size_t GetFileSize(FILE* file)
{
    fseek(file, 0, SEEK_END);
    long file_size_long = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size_long <= 0)
        return 0;

    return (size_t)file_size_long;
}

void SkipSpaces(const char* buffer, size_t* pos) //
{
    while (isspace(buffer[*pos]))
        (*pos)++;
}
