#include "user_interface.h"
#include <assert.h>

const char* GetDataBaseFilename(int argc, const char** argv)
{
    assert(argv);

    return (argc < 1) ? kDefaultDataBaseFilename : argv[1];
}
