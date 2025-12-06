#ifndef IO_DIFFERENCIATOR_H_
#define IO_DIFFERENCIATOR_H_

#include <stdlib.h>
#include <stdio.h>
#include "tree_base.h"
#include "tree_error_types.h"

char* ReadExpressionFromFile(const char* filename);
void SkipSpaces(const char* buffer, size_t* pos);
size_t GetFileSize(FILE* file);

#endif //IO_DIFFERENCIATOR_H_
