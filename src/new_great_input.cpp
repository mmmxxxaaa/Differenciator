#include "new_great_input.h"

#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "tree_base.h"

//FIXME надо возвращать позицию где произошла синтаксическая ошибка

// ==================== DSL ДЛЯ СОЗДАНИЯ УЗЛОВ ====================
#define CREATE_NUM(value)          CreateNode(NODE_NUM, (ValueOfTreeElement){.num_value = (value)}, NULL, NULL)
#define CREATE_OP(op, left, right) CreateNode(NODE_OP,  (ValueOfTreeElement){.op_value = (op)}, (left), (right))
#define CREATE_UNARY_OP(op, right) CreateNode(NODE_OP,  (ValueOfTreeElement){.op_value = (op)}, NULL, (right))
#define CREATE_VAR(name)           CreateNode(NODE_VAR, (ValueOfTreeElement){.var_definition = (VariableDefinition){.name = strdup(name), .hash = ComputeHash(name)}}, NULL, NULL)

#define CHECK_AND_CREATE(condition, creator) \
    ((condition) ? (creator) : (NULL))

#define RELEASE_IF_NULL(ptr, ...) \
    do { \
        if (!(ptr)) \
        {           \
            Node* nodes[] = {__VA_ARGS__}; \
            for (size_t i = 0; i < sizeof(nodes)/sizeof(nodes[0]); i++) \
                if (nodes[i]) FreeSubtree(nodes[i]); \
        } \
    } while(0)
// ===================================================================

static ParserContext* CreateParserContext(VariableTable* var_table)
{
    static OperationInfo default_operations[] = {
        {0, "sin", OP_SIN},
        {0, "cos", OP_COS},
        {0, "ln", OP_LN},
        {0, "exp", OP_EXP}
    };

    static size_t default_operations_count = sizeof(default_operations) / sizeof(default_operations[0]);

    ParserContext* context = (ParserContext*)calloc(1, sizeof(ParserContext));
    if (!context)
        return NULL;

    context->var_table = var_table;

    context->operations = default_operations; //сохраняем указатель на статический массив зарезервированных операций
    context->operations_count = default_operations_count;
    context->hashes_initialized = false;

    return context;
}

static void FreeParserContext(ParserContext* context)
{
    free(context);
}

static void InitializeOperationHashes(ParserContext* context)
{
    if (!context || context->hashes_initialized)
        return;

    for (size_t i = 0; i < context->operations_count; i++)
    {
        OperationInfo* op = &context->operations[i]; //FIXME почему компилится без явного приведения?
        op->hash = ComputeHash(op->name);
    }

    context->hashes_initialized = true;
}

Node* GetG(const char** string, VariableTable* var_table)
{
    assert(string);
    assert(var_table);

    ParserContext* context = CreateParserContext(var_table);
    if (!context)
        return NULL;

    Node* val = GetE(string, context);

    if (**string != '$')
    {
        printf("Expected end of expression '$'\n");
        SyntaxError();
        if (val)
            FreeSubtree(val);

        FreeParserContext(context);
        return NULL;
    }

    FreeParserContext(context);
    return val;
}

Node* GetE(const char** string, ParserContext* context)
{
    assert(string);
    assert(context);

    Node* val = GetT(string, context);
    if (!val)
        return NULL;

    while (**string == '+' || **string == '-')
    {
        char op_char = **string;
        OperationType op = (op_char == '+') ? OP_ADD : OP_SUB;

        (*string)++;
        Node* val2 = GetT(string, context);
        if (!val2)
        {
            FreeSubtree(val);
            return NULL;
        }

        Node* new_val = CREATE_OP(op, val, val2);
        if (!new_val)
        {
            FreeSubtree(val);
            FreeSubtree(val2);
            return NULL;
        }
        val = new_val;
    }

    return val;
}

Node* GetT(const char** string, ParserContext* context)
{
    assert(string);
    assert(context);

    Node* val = GetF(string, context);
    if (!val)
        return NULL;

    while (**string == '*' || **string == '/')
    {
        char op_char = **string;
        OperationType op = (op_char == '*') ? OP_MUL : OP_DIV;

        (*string)++;
        Node* val2 = GetF(string, context);
        if (!val2)
        {
            FreeSubtree(val);
            return NULL;
        }

        Node* new_val = CREATE_OP(op, val, val2);
        if (!new_val)
        {
            FreeSubtree(val);
            FreeSubtree(val2);
            return NULL;
        }
        val = new_val;
    }

    return val;
}

Node* GetF(const char** string, ParserContext* context)
{
    assert(string);
    assert(context);

    Node* val = GetP(string, context);
    if (!val)
        return NULL;

    while (**string == '^')
    {
        (*string)++;
        Node* exponent = GetP(string, context);
        if (!exponent)
        {
            FreeSubtree(val);
            return NULL;
        }

        Node* new_val = CREATE_OP(OP_POW, val, exponent);
        if (!new_val)
        {
            FreeSubtree(val);
            FreeSubtree(exponent);
            return NULL;
        }
        val = new_val;
    }

    return val;
}

Node* GetP(const char** string, ParserContext* context)
{
    assert(string);
    assert(context);

    Node* func_node = GetFunction(string, context);
    if (func_node)
        return func_node;

    if (**string == '(')
    {
        (*string)++;
        Node* val = GetE(string, context);
        if (!val)
            return NULL;

        if (**string != ')')
        {
            printf("Expected closing ')'\n");
            SyntaxError();
            FreeSubtree(val);
            return NULL;
        }
        else
        {
            (*string)++;
        }
        return val;
    }

    Node* result = GetN(string);
    if (result != NULL) return result;

    result = GetV(string, context);
    if (result != NULL) return result;

    return NULL;
}

Node* GetN(const char** string)
{
    assert(string);

    if (isdigit(**string))
    {
        int val = 0;
        const char* start = *string;

        while ('0' <= **string && **string <= '9')
        {
            val = **string - '0' + val * 10;
            (*string)++;
        }

        if (start == *string)
        {
            SyntaxError();
            return NULL;
        }

        return CREATE_NUM((double)val);
    }

    return NULL;
}

Node* GetV(const char** string, ParserContext* context)
{
    assert(string);
    assert(context);

    if (!isalpha(**string))
        return NULL;

    char var_name[kMaxVariableLength] = {0};
    int i = 0;
    const char* start = *string;

    while (**string >= 'a' && **string <= 'z' && i < 255)
    {
        var_name[i++] = **string;
        (*string)++;
    }

    if (start == *string)
    {
        SyntaxError();
        return NULL;
    }

    TreeErrorType error = AddVariable(context->var_table, var_name);
    if (error != TREE_ERROR_NO && error != TREE_ERROR_VARIABLE_ALREADY_EXISTS &&
        error != TREE_ERROR_REDEFINITION_VARIABLE)
    {
        printf("Error adding variable to table: %d\n", error);
        return NULL;
    }

    ValueOfTreeElement data = {};
    data.var_definition.name = strdup(var_name);
    data.var_definition.hash = ComputeHash(var_name);

    return CreateNode(NODE_VAR, data, NULL, NULL);
}

static bool FindOperationByName(ParserContext* context, const char* func_name, OperationType* found_op) //используется в GetV
{
    assert(context);
    assert(func_name);
    assert(found_op);

    unsigned int func_hash = ComputeHash(func_name);

    for (size_t j = 0; j < context->operations_count; j++)
    {
        if (func_hash == context->operations[j].hash)
        {
            *found_op = context->operations[j].op_value;
            return true;
        }
    }

    return false;
}

Node* GetFunction(const char** string, ParserContext* context)
{
    assert(string);
    assert(context);

    InitializeOperationHashes(context);

    const char* original_pos = *string;

    char func_name[kMaxFuncNameLength] = {0};

    int chars_read = 0;
    assert(kMaxFuncNameLength == 256);
    if(sscanf(*string, "%255[a-z]%n", func_name, &chars_read) == 1)
    {
        *string += chars_read;
    }
    else
    {
        *string = original_pos;
        return NULL;
    }

    OperationType found_op = OP_ADD;
    bool found = FindOperationByName(context, func_name, &found_op);

    if (!found)
    {
        *string = original_pos;
        return NULL;
    }

    Node* arg = GetP(string, context);
    if (!arg)
    {
        *string = original_pos;
        return NULL;
    }

    return CREATE_UNARY_OP(found_op, arg);
}

void SyntaxError()
{
    printf("Syntax error!\n");
}

// ==================== UNDEF MACROS ====================
#undef CREATE_NUM
#undef CREATE_OP
#undef CREATE_UNARY_OP
#undef CREATE_VAR
#undef CHECK_AND_CREATE
#undef RELEASE_IF_NULL
