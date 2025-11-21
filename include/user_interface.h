#ifndef TREE_USER_INTERFACE_H_
#define TREE_USER_INTERFACE_H_

#include "tree_common.h"
#include "tree_error_types.h"

const char* GetDataBaseFilename(int argc, const char** argv);
const char* GetTreeErrorString(TreeErrorType error);
void PrintTreeError(TreeErrorType error);


#endif // TREE_USERT_INTERFACE_H_
