#include "io_diff.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dump.h"
#include "tree_base.h"

void InitLoadProgress(LoadProgress* progress)
{
    if (progress)
    {
        progress->size = 0;
        progress->capacity = 10;
        progress->current_depth = 0;
        progress->items = (NodeDepthInfo*)calloc(progress->capacity, sizeof(NodeDepthInfo));
    }
}

void AddNodeToLoadProgress(LoadProgress* progress, Node* node, size_t depth)
{
    if (progress == NULL)
        return;

    if (progress->size >= progress->capacity)
    {
        progress->capacity *= 2;
        progress->items = (NodeDepthInfo*)realloc(progress->items, progress->capacity * sizeof(NodeDepthInfo));
    }
    progress->items[progress->size].node  = node;
    progress->items[progress->size].depth = depth;
    progress->size++;
}

void FreeLoadProgress(LoadProgress* progress)
{
    free(progress->items);

    progress->items         = NULL;
    progress->capacity      = 0;
    progress->size          = 0;
    progress->current_depth = 0;
}


size_t GetFileSize(FILE* file)
{
    fseek(file, 0, SEEK_END);
    long file_size_long = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size_long <= 0)
    {
        fclose(file);
        return 0;
    }

    return (size_t)file_size_long;
}

NodeType DetermineNodeType(const char* token)
{
    if (strcmp(token, "+") == 0   || strcmp(token, "-") == 0    ||
        strcmp(token, "*") == 0   || strcmp(token, "/") == 0    ||
        strcmp(token, "sin") == 0 || strcmp(token, "cos") == 0)
    {
        return NODE_OP;
    }
    else if (isdigit(token[0]) || (token[0] == '0' && isdigit(token[1])))
    {
        return NODE_NUM;
    }
    else
        return NODE_VAR;
}

static void SkipSpaces(const char* buffer, size_t* pos)
{
    while (isspace(buffer[*pos]))
        (*pos)++;
}

char* ReadToken(char* buffer, size_t* pos)
{
    SkipSpaces(buffer, pos);

    if (buffer[*pos] == '\0')
        return NULL;

    size_t start = *pos;

    while(buffer[*pos] != '\0' && buffer[*pos] != ' '  &&
          buffer[*pos] != '\t' && buffer[*pos] != '\n' &&
          buffer[*pos] != '\r' && buffer[*pos] != '(' &&
          buffer[*pos] != ')')
    {
        (*pos)++;
    }

    if ((*pos) == start)
        return NULL;

    size_t length = *pos - start;

    char* token = (char*)calloc(length + 1, sizeof(char));
    if (token == NULL)
        return NULL;

    strncpy(token, buffer + start, length);
    token[length] = '\0';

    return token;
}

static Node* ReadNodeFromBuffer(Tree* tree, char* buffer, size_t buffer_length, size_t* pos, Node* parent,
                                int depth, LoadProgress* progress)
{
    SkipSpaces(buffer, pos);

    if (buffer[*pos] == '\0')
        return NULL;

    if (strncmp(buffer + *pos, "nul", sizeof("nul") - 1) == 0) //FIXME мб в константу
    {
        *pos += sizeof("nul") - 1;
        return NULL;
    }

    if (buffer[*pos] != '(')
        return NULL;

    (*pos)++;
    SkipSpaces(buffer, pos);

    char* str_ptr = ReadToken(buffer, pos);
    if (str_ptr == NULL)
        return NULL;

    Node* node = CreateNodeFromToken(str_ptr, parent); //ХУЙНЯ надо передавать тип
    //FIXME почему
    // Node* node = CreateNode(str_ptr, parent); //FIXME теперь надо передавать в конструктор узла тип данных в узле    if (node == NULL)
    free(str_ptr);
    if (node == NULL)
        return NULL;

    if (tree != NULL)
        tree->size++;

    if (progress != NULL)
    {
        AddNodeToLoadProgress(progress, node, depth);
        TreeLoadDump(tree, "akinator_parse", buffer, buffer_length, *pos, progress, "Node created");
    }

    SkipSpaces(buffer, pos);
    node->left = ReadNodeFromBuffer(tree, buffer, buffer_length, pos, node, depth + 1, progress);

    SkipSpaces(buffer, pos);
    node->right = ReadNodeFromBuffer(tree, buffer, buffer_length, pos, node, depth + 1, progress);

    SkipSpaces(buffer, pos);
    if (buffer[*pos] != ')')
    {
        // TreeDestroyWithDataRecursive(node);
        return NULL;
    }
    (*pos)++;

    if (progress != NULL)
        TreeLoadDump(tree, "akinator_parse", buffer, buffer_length, *pos, progress, "Subtree complete");

    return node;
}

TreeErrorType TreeLoad(Tree* tree, const char* filename)
{
    if (tree == NULL || filename == NULL)
        return TREE_ERROR_NULL_PTR;

    printf("Trying to open file: %s\n", filename); // отладочная информация

    FILE* file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("Failed to open file: %s\n", filename); // отладочная информация
        return TREE_ERROR_IO;
    }

    size_t file_size = GetFileSize(file);
    printf("File size: %zu\n", file_size); // отладочная информация

    if (file_size == 0)
    {
        fclose(file);
        printf("File is empty\n"); // отладочная информация
        return TREE_ERROR_FORMAT;
    }

    char* file_buffer = (char*)calloc(file_size + 1, sizeof(char));
    if (file_buffer == NULL)
    {
        fclose(file);
        return TREE_ERROR_ALLOCATION;
    }

    size_t bytes_read = fread(file_buffer, 1, file_size, file);
    file_buffer[bytes_read] = '\0';
    fclose(file);

    printf("File content: %s\n", file_buffer); // отладочная информация

    LoadProgress progress = {};
    InitLoadProgress(&progress);

    size_t pos = 0;
    size_t buffer_length = strlen(file_buffer);
    tree->root = ReadNodeFromBuffer(tree, file_buffer, buffer_length, &pos, NULL, 0, &progress);

    if (tree->root == NULL)
    {
        printf("Failed to parse tree from file\n"); // отладочная информация
        free(file_buffer);
        FreeLoadProgress(&progress);
        return TREE_ERROR_FORMAT;
    }

    tree->file_buffer = file_buffer;

    printf("Tree loaded successfully!\n"); // отладочная информация

    TreeDump(tree, "tree_ready");

    FreeLoadProgress(&progress);

    return TREE_ERROR_NO;
}
