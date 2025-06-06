// mp - v1.4.0 - MIT License - https://github.com/seajee/mp.h

// TODO: Include documentation on how to use the library

//----------------
// Header section
//----------------

#ifndef MP_H_
#define MP_H_

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MP_STR_UNKNOWN "?"

//------------------------
// Mathematical constants
//------------------------

#define MP_PI 3.14159265358979323846 // pi
#define MP_E  2.7182818284590452354  // e

//----------------
// Dynamic array
//----------------

#define MP_DA_INITIAL_CAPACITY 256

#define mp_da_append(da, item)                                               \
    do {                                                                     \
        if ((da)->count >= (da)->capacity) {                                 \
            (da)->capacity = (da)->capacity == 0                             \
                ? MP_DA_INITIAL_CAPACITY : (da)->capacity * 2;               \
            (da)->items =                                                    \
                realloc((da)->items, (da)->capacity * sizeof(*(da)->items)); \
            assert((da)->items != NULL && "Buy more RAM LOL");               \
        }                                                                    \
        (da)->items[(da)->count++] = (item);                                 \
    } while (0)

#define mp_da_free(da)      \
    do {                    \
        free((da)->items);  \
        (da)->items = NULL; \
        (da)->count = 0;    \
        (da)->capacity = 0; \
    } while (0)

#define mp_da_reset(da)  \
    do {                 \
        (da)->count = 0; \
    } while (0)

//-------
// Arena
//-------

// TODO: Implement the arena as a linked list of regions

#define MP_ARENA_DEFAULT_CAPACITY (8*1024)

typedef struct {
    size_t count;
    size_t capacity;
    void *data;
} MP_Arena;

MP_Arena mp_arena_init(size_t capacity);
void *mp_arena_alloc(MP_Arena *arena, size_t size);
void mp_arena_free(MP_Arena *arena);
void mp_arena_reset(MP_Arena *arena);

//-----------
// Tokenizer
//-----------

#define MP_NAME_CAPACITY 4

typedef enum {
    MP_TOKEN_INVALID,
    MP_TOKEN_EOF,
    MP_TOKEN_NUMBER,
    MP_TOKEN_SYMBOL,
    MP_TOKEN_NAME,
    MP_TOKEN_PLUS,
    MP_TOKEN_MINUS,
    MP_TOKEN_MULTIPLY,
    MP_TOKEN_DIVIDE,
    MP_TOKEN_POWER,
    MP_TOKEN_LPAREN,
    MP_TOKEN_RPAREN,
    MP_TOKEN_COUNT
} MP_Token_Type;

typedef struct {
    MP_Token_Type type;
    size_t position;
    union {
        double value;
        char symbol;
        char name[MP_NAME_CAPACITY + 1]; // +1 for '\0'
    };
} MP_Token;

typedef struct {
    size_t count;
    size_t capacity;
    MP_Token *items;
} MP_Token_List;

//----------------
// Error handling
//----------------

// TODO: Simplify error handling

typedef enum {
    MP_ERROR_OK,
    MP_ERROR_INVALID_TOKEN,
    MP_ERROR_INVALID_EXPRESSION,
    MP_ERROR_EMPTY_EXPRESSION,
    MP_ERROR_INVALID_NODE,
    MP_ERROR_INVALID_FUNCTION,
    MP_ERROR_ZERO_DIVISION,
    MP_ERROR_COUNT
} MP_Error_Type;

typedef struct {
    bool error;
    MP_Error_Type error_type;
    size_t error_position;
    MP_Token faulty_token;
    double value;
} MP_Result;

const char *mp_error_to_string(MP_Error_Type err);

//---------------------
// Tokenizer functions
//---------------------

MP_Result mp_tokenize(MP_Token_List *list, const char *expr);
const char *mp_token_to_string(MP_Token token);
void mp_print_token_list(MP_Token_List list);

//--------
// Parser
//--------

typedef enum {
    MP_NODE_INVALID,
    MP_NODE_NUMBER,
    MP_NODE_SYMBOL,
    MP_NODE_FUNCTION,
    MP_NODE_ADD,
    MP_NODE_SUBTRACT,
    MP_NODE_MULTIPLY,
    MP_NODE_DIVIDE,
    MP_NODE_POWER,
    MP_NODE_PLUS,
    MP_NODE_MINUS,
    MP_NODE_COUNT
} MP_Node_Type;

#define MP_FUNCTION_STR_LN   "ln"
#define MP_FUNCTION_STR_LOG  "log"
#define MP_FUNCTION_STR_SIN  "sin"
#define MP_FUNCTION_STR_COS  "cos"
#define MP_FUNCTION_STR_TAN  "tan"
#define MP_FUNCTION_STR_SQRT "sqrt"

typedef enum {
    MP_FUNCTION_INVALID,
    MP_FUNCTION_LN,
    MP_FUNCTION_LOG,
    MP_FUNCTION_SIN,
    MP_FUNCTION_COS,
    MP_FUNCTION_TAN,
    MP_FUNCTION_SQRT,
    MP_FUNCTION_COUNT
} MP_Function;

typedef struct MP_Tree_Node MP_Tree_Node;

struct MP_Tree_Node {
    MP_Node_Type type;

    union {
        struct {
            MP_Tree_Node *lhs;
            MP_Tree_Node *rhs;
        } binop;

        struct {
            MP_Tree_Node *node;
        } unary;

        struct {
            MP_Function name;
            MP_Tree_Node *arg;
        } function;

        union {
            double value;
            char symbol;
        };
    };
};

typedef struct {
    MP_Token_List tokens;
    MP_Token current;
    size_t cursor;
} MP_Parser;

typedef struct {
    MP_Tree_Node *root;
    MP_Result result;
} MP_Parse_Tree;

MP_Tree_Node *mp_make_node(MP_Arena *a, MP_Node_Type t, double value);
MP_Tree_Node *mp_make_node_symbol(MP_Arena *a, char symbol);
MP_Tree_Node *mp_make_node_unary(MP_Arena *a, MP_Node_Type t, MP_Tree_Node *node);
MP_Tree_Node *mp_make_node_binop(MP_Arena *a, MP_Node_Type t,
                                 MP_Tree_Node *lhs, MP_Tree_Node *rhs);
MP_Tree_Node *mp_make_node_function(MP_Arena *a, const MP_Token *name,
                                    MP_Tree_Node *arg);

MP_Result mp_parse(MP_Arena *a, MP_Parse_Tree *tree, MP_Token_List list);
void mp_parser_advance(MP_Parser *parser);
MP_Tree_Node *mp_parse_expr(MP_Arena *a, MP_Parser *parser, MP_Result *result);
MP_Tree_Node *mp_parse_term(MP_Arena *a, MP_Parser *parser, MP_Result *result);
MP_Tree_Node *mp_parse_factor(MP_Arena *a, MP_Parser *parser, MP_Result *result);
MP_Tree_Node *mp_parse_primary(MP_Arena *a, MP_Parser *parser, MP_Result *result);
void mp_print_parse_tree(MP_Parse_Tree tree);
void mp_print_tree_node(MP_Tree_Node *root);

const char *mp_function_name_to_string(MP_Function name);

//-------------
// Interpreter
//-------------

typedef struct {
    MP_Parse_Tree tree;
    MP_Arena arena;
    double vars[26]; // a - z
} MP_Interpreter;

MP_Result mp_interpret(MP_Interpreter *interpreter);
MP_Result mp_interpret_node(MP_Interpreter *interpreter, MP_Tree_Node *root);

MP_Interpreter mp_interpreter_init(MP_Parse_Tree tree, MP_Arena arena);
void mp_interpreter_var(MP_Interpreter *interpreter, char var, double value);
void mp_interpreter_free(MP_Interpreter *interpreter);

//----------
// Compiler
//----------

typedef enum {
    MP_OP_INVALID,
    MP_OP_PUSH_NUM,
    MP_OP_PUSH_VAR,
    MP_OP_ADD,
    MP_OP_SUB,
    MP_OP_MUL,
    MP_OP_DIV,
    MP_OP_POW,
    MP_OP_NEG,
    MP_OP_COUNT
} MP_Opcode;

typedef struct {
    size_t count;
    size_t capacity;
    uint8_t *items;
} MP_Program;

typedef struct {
    size_t count;
    size_t capacity;
    double *items;
} MP_Stack;

typedef struct {
    MP_Program program;
    MP_Stack stack;
    double vars[26]; // a - z
    size_t ip;
} MP_Vm;

typedef struct {
    bool present;
    double value;
} MP_Optional;

bool mp_program_compile(MP_Program *p, MP_Parse_Tree parse_tree);
bool mp_program_compile_node(MP_Program *p, MP_Tree_Node *node);
void mp_program_push_opcode(MP_Program *p, MP_Opcode op);
void mp_program_push_const(MP_Program *p, double value);
void mp_program_push_var(MP_Program *p, char var);
void mp_print_program(MP_Program p);

void mp_stack_push(MP_Stack *stack, double n);
MP_Optional mp_stack_pop(MP_Stack *stack);
MP_Optional mp_stack_peek(MP_Stack *stack);

MP_Vm mp_vm_init(MP_Program program);
void mp_vm_var(MP_Vm *vm, char var, double value);
bool mp_vm_run(MP_Vm *vm);
double mp_vm_result(MP_Vm *vm);
void mp_vm_free(MP_Vm *vm);

//----------------
// Simplified API
//----------------

typedef enum {
    MP_MODE_INTERPRET,
    MP_MODE_COMPILE,
    MP_MODE_COUNT
} MP_Mode;

typedef struct {
    MP_Mode mode;
    union {
        MP_Interpreter interpreter;
        MP_Vm vm;
    };
} MP_Env;

MP_Env *mp_init(const char *expression);
MP_Env *mp_init_mode(const char *expression, MP_Mode mode);
void mp_variable(MP_Env *env, char var, double value);
MP_Result mp_evaluate(MP_Env *env);
void mp_free(MP_Env *env);

#endif // MP_H_

//------------------------
// Implementation section
//------------------------

#ifdef MP_IMPLEMENTATION

//-------
// Arena
//-------

MP_Arena mp_arena_init(size_t capacity)
{
    MP_Arena arena = {0};
    arena.capacity = capacity;
    arena.count = 0;
    arena.data = malloc(capacity);
    assert(arena.data != NULL);

    return arena;
}

void *mp_arena_alloc(MP_Arena *arena, size_t size)
{
    if (arena->data == NULL) {
        *arena = mp_arena_init(MP_ARENA_DEFAULT_CAPACITY);
    }

    assert(arena->count + size <= arena->capacity);

    void *result = (uint8_t*)arena->data + arena->count;
    arena->count += size;

    return result;
}

void mp_arena_free(MP_Arena *arena)
{
    if (arena == NULL)
        return;

    arena->count = 0;
    arena->capacity = 0;

    free(arena->data);
    arena->data = NULL;
}

void mp_arena_reset(MP_Arena *arena)
{
    if (arena == NULL)
        return;

    arena->count = 0;
}

//-----------
// Tokenizer
//-----------

MP_Result mp_tokenize(MP_Token_List *list, const char *expr)
{
    MP_Result result = {0};
    size_t cursor = 0;
    size_t end = strlen(expr);

    while (cursor < end) {
        char c = expr[cursor];
        MP_Token token = {0};
        token.position = cursor;

        switch (c) {
            case '\n':
            case '\t':
            case ' ': {
                ++cursor;
            } break;

            case '+': {
                token.type = MP_TOKEN_PLUS;
                mp_da_append(list, token);
                ++cursor;
            } break;

            case '-': {
                token.type = MP_TOKEN_MINUS;
                mp_da_append(list, token);
                ++cursor;
            } break;

            case '*': {
                token.type = MP_TOKEN_MULTIPLY;
                mp_da_append(list, token);
                ++cursor;
            } break;

            case '/': {
                token.type = MP_TOKEN_DIVIDE;
                mp_da_append(list, token);
                ++cursor;
            } break;

            case '^': {
                token.type = MP_TOKEN_POWER;
                mp_da_append(list, token);
                ++cursor;
            } break;

            case '(': {
                token.type = MP_TOKEN_LPAREN;
                mp_da_append(list, token);
                ++cursor;
            } break;

            case ')': {
                token.type = MP_TOKEN_RPAREN;
                mp_da_append(list, token);
                ++cursor;
            } break;

            default: {
                // Numbers
                if (isdigit(c)) {
                    char *end;
                    token.type = MP_TOKEN_NUMBER;
                    token.value = strtod(&expr[cursor], &end);
                    cursor = end - expr;
                    mp_da_append(list, token);
                    break;
                }

                // Symbols / Names
                if (islower(c)) {
                    // Symbol
                    if (cursor >= end - 1 || !islower(expr[cursor + 1])) {
                        token.type = MP_TOKEN_SYMBOL;
                        token.symbol = c;
                        mp_da_append(list, token);
                        ++cursor;
                        break;
                    }

                    // Name
                    token.type = MP_TOKEN_NAME;
                    size_t name_len = 0;
                    do {
                        token.name[name_len++] = expr[cursor];
                        cursor++;
                    } while (name_len <= MP_NAME_CAPACITY
                            && cursor < end && islower(expr[cursor]));

                    if (name_len <= MP_NAME_CAPACITY) {
                        mp_da_append(list, token);
                        break;
                    }
                }

                // Invalid
                token.type = MP_TOKEN_INVALID;
                result.error = true;
                result.error_type = MP_ERROR_INVALID_TOKEN;
                result.error_position = cursor;
                result.faulty_token = token;
                return result;
            } break;
        }
    }

    return result;
}

const char *mp_token_to_string(MP_Token token)
{
    switch (token.type) {
        case MP_TOKEN_EOF:      return "TOKEN_EOF";
        case MP_TOKEN_INVALID:  return "TOKEN_INVALID";
        case MP_TOKEN_NUMBER:   return "TOKEN_NUMBER";
        case MP_TOKEN_SYMBOL:   return "TOKEN_SYMBOL";
        case MP_TOKEN_NAME:     return "TOKEN_NAME";
        case MP_TOKEN_PLUS:     return "TOKEN_PLUS";
        case MP_TOKEN_MINUS:    return "TOKEN_MINUS";
        case MP_TOKEN_MULTIPLY: return "TOKEN_MULTIPLY";
        case MP_TOKEN_DIVIDE:   return "TOKEN_DIVIDE";
        case MP_TOKEN_POWER:    return "TOKEN_POWER";
        case MP_TOKEN_LPAREN:   return "TOKEN_LPAREN";
        case MP_TOKEN_RPAREN:   return "TOKEN_RPAREN";
        default:                return MP_STR_UNKNOWN;
    }
}

void mp_print_token_list(MP_Token_List list)
{
    for (size_t i = 0; i < list.count; ++i) {
        MP_Token token = list.items[i];
        printf("%ld: %s", i, mp_token_to_string(token));
        if (token.type == MP_TOKEN_NUMBER) {
            printf(" %f", token.value);
        } else if (token.type == MP_TOKEN_SYMBOL) {
            printf(" %c", token.symbol);
        } else if (token.type == MP_TOKEN_NAME) {
            printf(" %s", token.name);
        }
        printf("\n");
    }
}

const char *mp_error_to_string(MP_Error_Type err)
{
    switch (err) {
        case MP_ERROR_OK:                 return "No error";
        case MP_ERROR_INVALID_TOKEN:      return "Unexpected token";
        case MP_ERROR_INVALID_EXPRESSION: return "Invalid expression";
        case MP_ERROR_EMPTY_EXPRESSION:   return "Empty expression";
        case MP_ERROR_INVALID_NODE:       return "Invalid expression";
        case MP_ERROR_INVALID_FUNCTION:   return "Invalid function";
        case MP_ERROR_ZERO_DIVISION:      return "Division by zero";
        default:                          return "Unknown error";
    }
}

MP_Tree_Node *mp_make_node_binop(MP_Arena *a, MP_Node_Type t,
                                 MP_Tree_Node *lhs, MP_Tree_Node *rhs)
{
    MP_Tree_Node *r = mp_arena_alloc(a, sizeof(*r));
    r->type = t;
    r->binop.lhs = lhs;
    r->binop.rhs = rhs;
    return r;
}

MP_Tree_Node *mp_make_node_function(MP_Arena *a, const MP_Token *name,
                                    MP_Tree_Node *arg)
{
    MP_Tree_Node *r = mp_arena_alloc(a, sizeof(*r));
    r->type = MP_NODE_FUNCTION;
    
    const char *name_str = name->name;
    if (strcmp(MP_FUNCTION_STR_LN, name_str) == 0) {
        r->function.name = MP_FUNCTION_LN;

    } else if (strcmp(MP_FUNCTION_STR_LOG, name_str) == 0) {
        r->function.name = MP_FUNCTION_LOG;

    } else if (strcmp(MP_FUNCTION_STR_SIN, name_str) == 0) {
        r->function.name = MP_FUNCTION_SIN;

    } else if (strcmp(MP_FUNCTION_STR_COS, name_str) == 0) {
        r->function.name = MP_FUNCTION_COS;

    } else if (strcmp(MP_FUNCTION_STR_TAN, name_str) == 0) {
        r->function.name = MP_FUNCTION_TAN;

    } else if (strcmp(MP_FUNCTION_STR_SQRT, name_str) == 0) {
        r->function.name = MP_FUNCTION_SQRT;

    } else {
        r->function.name = MP_FUNCTION_INVALID;
    }

    r->function.arg = arg;
    return r;
}

MP_Tree_Node *mp_make_node_unary(MP_Arena *a, MP_Node_Type t, MP_Tree_Node *node)
{
    MP_Tree_Node *r = mp_arena_alloc(a, sizeof(*r));
    r->type = t;
    r->unary.node = node;
    return r;
}

MP_Tree_Node *mp_make_node_symbol(MP_Arena *a, char symbol)
{
    MP_Tree_Node *r = mp_arena_alloc(a, sizeof(*r));
    r->type = MP_NODE_SYMBOL;
    r->symbol = symbol;
    return r;
}

MP_Tree_Node *mp_make_node(MP_Arena *a, MP_Node_Type t, double value)
{
    MP_Tree_Node *r = mp_arena_alloc(a, sizeof(*r));
    r->type = t;
    r->value = value;
    return r;
}

MP_Result mp_parse(MP_Arena *a, MP_Parse_Tree *tree, MP_Token_List list)
{
    MP_Result result = {0};

    MP_Parser parser = {0};
    parser.tokens = list;

    mp_parser_advance(&parser);

    if (parser.current.type == MP_TOKEN_EOF) {
        result.error = true;
        result.error_type = MP_ERROR_EMPTY_EXPRESSION;
        return result;
    }

    MP_Tree_Node *tree_root = mp_parse_expr(a, &parser, &result);
    tree->root = tree_root;

    if (parser.current.type != MP_TOKEN_EOF) {
        result.error = true;
        result.error_type = MP_ERROR_INVALID_EXPRESSION;
        result.error_position = parser.current.position;
        return result;
    }

    return result;
}

void mp_parser_advance(MP_Parser *parser)
{
    if (parser->cursor >= parser->tokens.count) {
        parser->current.type = MP_TOKEN_EOF;
        return;
    }

    parser->current = parser->tokens.items[parser->cursor++];
}

MP_Tree_Node *mp_parse_expr(MP_Arena *a, MP_Parser *parser, MP_Result *result)
{
    MP_Tree_Node *result_node = mp_parse_term(a, parser, result);
    MP_Token *cur = &parser->current;

    while (!result->error
        && (cur->type == MP_TOKEN_PLUS || cur->type == MP_TOKEN_MINUS)) {

        if (cur->type == MP_TOKEN_PLUS) {
            mp_parser_advance(parser);
            result_node = mp_make_node_binop(a, MP_NODE_ADD, result_node,
                                             mp_parse_term(a, parser, result));
        } else if (cur->type == MP_TOKEN_MINUS) {
            mp_parser_advance(parser);
            result_node = mp_make_node_binop(a, MP_NODE_SUBTRACT, result_node,
                                             mp_parse_term(a, parser, result));
        }
    }

    return result_node;
}

MP_Tree_Node *mp_parse_term(MP_Arena *a, MP_Parser *parser, MP_Result *result)
{
    MP_Tree_Node *result_node = mp_parse_factor(a, parser, result);
    MP_Token *cur = &parser->current;

    while (!result->error
        && (cur->type == MP_TOKEN_MULTIPLY || cur->type == MP_TOKEN_DIVIDE)) {

        if (cur->type == MP_TOKEN_MULTIPLY) {
            mp_parser_advance(parser);
            result_node = mp_make_node_binop(a, MP_NODE_MULTIPLY, result_node,
                                             mp_parse_factor(a, parser, result));
        } else if (cur->type == MP_TOKEN_DIVIDE) {
            mp_parser_advance(parser);
            result_node = mp_make_node_binop(a, MP_NODE_DIVIDE, result_node,
                                             mp_parse_factor(a, parser, result));
        }
    }

    return result_node;
}

MP_Tree_Node *mp_parse_factor(MP_Arena *a, MP_Parser *parser, MP_Result *result)
{
    MP_Token *cur = &parser->current;
    MP_Tree_Node *result_node;

    if (cur->type == MP_TOKEN_NAME) {
        MP_Token name = parser->current;
        mp_parser_advance(parser);

        if (cur->type != MP_TOKEN_LPAREN) {
            result->error = true;
            result->error_type = MP_ERROR_INVALID_EXPRESSION;
            result->error_position = cur->position;
            return NULL;
        }
        mp_parser_advance(parser);

        result_node = mp_make_node_function(a, &name,
                mp_parse_expr(a, parser, result));

        if (cur->type != MP_TOKEN_RPAREN) {
            result->error = true;
            result->error_type = MP_ERROR_INVALID_EXPRESSION;
            result->error_position = cur->position;
            return NULL;
        }

        mp_parser_advance(parser);
        return result_node;
    }

    result_node = mp_parse_primary(a, parser, result);

    if (cur->type == MP_TOKEN_POWER) {
        mp_parser_advance(parser);
        result_node = mp_make_node_binop(a, MP_NODE_POWER, result_node,
                                         mp_parse_primary(a, parser, result));
    }

    return result_node;
}

MP_Tree_Node *mp_parse_primary(MP_Arena *a, MP_Parser *parser, MP_Result *result)
{
    MP_Token *cur = &parser->current;

    if (cur->type == MP_TOKEN_LPAREN) {
        mp_parser_advance(parser);
        MP_Tree_Node *result_node = mp_parse_expr(a, parser, result);

        if (cur->type != MP_TOKEN_RPAREN) {
            result->error = true;
            result->error_type = MP_ERROR_INVALID_EXPRESSION;
            result->error_position = cur->position;
            return result_node;
        }

        mp_parser_advance(parser);
        return result_node;
    }

    if (cur->type == MP_TOKEN_NUMBER) {
        MP_Tree_Node *number = mp_make_node(a, MP_NODE_NUMBER, cur->value);
        mp_parser_advance(parser);
        return number;
    }

    if (cur->type == MP_TOKEN_SYMBOL) {
        MP_Tree_Node *symbol = mp_make_node_symbol(a, cur->symbol);
        mp_parser_advance(parser);
        return symbol;
    }

    if (cur->type == MP_TOKEN_PLUS) {
        mp_parser_advance(parser);
        return mp_make_node_unary(a, MP_NODE_PLUS,
                                  mp_parse_factor(a, parser, result));
    }

    if (cur->type == MP_TOKEN_MINUS) {
        mp_parser_advance(parser);
        return mp_make_node_unary(a, MP_NODE_MINUS,
                                  mp_parse_factor(a, parser, result));
    }

    result->error = true;
    result->error_type = MP_ERROR_INVALID_EXPRESSION;
    result->error_position = cur->position;
    result->faulty_token = *cur;

    return NULL;

}

void mp_print_parse_tree(MP_Parse_Tree tree)
{
    mp_print_tree_node(tree.root);
    printf("\n");
}

void mp_print_tree_node(MP_Tree_Node *root)
{
    if (root == NULL)
        return;

    switch (root->type) {
        case MP_NODE_INVALID: {
            printf("INVALID");
        } break;

        case MP_NODE_NUMBER: {
            printf("%f", root->value);
        } break;

        case MP_NODE_SYMBOL: {
            printf("%c", root->symbol);
        } break;

        case MP_NODE_FUNCTION: {
            printf("%s(", mp_function_name_to_string(root->function.name));
            mp_print_tree_node(root->function.arg);
            printf(")");
        } break;

        case MP_NODE_ADD: {
            printf("add(");
            mp_print_tree_node(root->binop.lhs);
            printf(",");
            mp_print_tree_node(root->binop.rhs);
            printf(")");
        } break;

        case MP_NODE_SUBTRACT: {
            printf("sub(");
            mp_print_tree_node(root->binop.lhs);
            printf(",");
            mp_print_tree_node(root->binop.rhs);
            printf(")");
        } break;

        case MP_NODE_MULTIPLY: {
            printf("mul(");
            mp_print_tree_node(root->binop.lhs);
            printf(",");
            mp_print_tree_node(root->binop.rhs);
            printf(")");
        } break;

        case MP_NODE_DIVIDE: {
            printf("div(");
            mp_print_tree_node(root->binop.lhs);
            printf(",");
            mp_print_tree_node(root->binop.rhs);
            printf(")");
        } break;

        case MP_NODE_POWER: {
            printf("pow(");
            mp_print_tree_node(root->binop.lhs);
            printf(",");
            mp_print_tree_node(root->binop.rhs);
            printf(")");
        } break;

        case MP_NODE_PLUS: {
            printf("plus(");
            mp_print_tree_node(root->unary.node);
            printf(")");
        } break;

        case MP_NODE_MINUS: {
            printf("minus(");
            mp_print_tree_node(root->unary.node);
            printf(")");
        } break;

        default: {
            printf(MP_STR_UNKNOWN);
        } break;
    }
}

const char *mp_function_name_to_string(MP_Function name)
{
    switch (name) {
        case MP_FUNCTION_LN:   return MP_FUNCTION_STR_LN;
        case MP_FUNCTION_LOG:  return MP_FUNCTION_STR_LOG;
        case MP_FUNCTION_SIN:  return MP_FUNCTION_STR_SIN;
        case MP_FUNCTION_COS:  return MP_FUNCTION_STR_COS;
        case MP_FUNCTION_TAN:  return MP_FUNCTION_STR_TAN;
        case MP_FUNCTION_SQRT: return MP_FUNCTION_STR_SQRT;
        default:               return MP_STR_UNKNOWN;
    }
}

//-------------
// Interpreter
//-------------

MP_Result mp_interpret(MP_Interpreter *interpreter)
{
    MP_Result r = {0};
    if (interpreter == NULL) {
        r.error = true;
        return r;
    }

    if (interpreter->tree.root == NULL) {
        r.error = true;
        r.error_type = MP_ERROR_EMPTY_EXPRESSION;
        return r;
    }

    return mp_interpret_node(interpreter, interpreter->tree.root);
}

MP_Result mp_interpret_node(MP_Interpreter *interpreter, MP_Tree_Node *root)
{
    MP_Result result = {0};

    if (root == NULL) {
        result.error = true;
        result.error_type = MP_ERROR_INVALID_NODE;
        return result;
    }

    switch (root->type) {
        case MP_NODE_INVALID: {
            result.error = true;
            result.error_type = MP_ERROR_INVALID_NODE;
            return result;
        } break;

        case MP_NODE_NUMBER: {
            result.value = root->value;
        } break;

        case MP_NODE_SYMBOL: {
            assert('a' <= root->symbol && root->symbol <= 'z');
            result.value = interpreter->vars[root->symbol - 'a'];
            return result;
        } break;

        case MP_NODE_FUNCTION: {
            MP_Result arg = mp_interpret_node(interpreter, root->function.arg);
            if (arg.error) return arg;

            switch (root->function.name) {
                case MP_FUNCTION_LN:
                    result.value = log(arg.value);
                    break;

                case MP_FUNCTION_LOG:
                    result.value = log10(arg.value);
                    break;

                case MP_FUNCTION_SIN:
                    result.value = sin(arg.value);
                    break;

                case MP_FUNCTION_COS:
                    result.value = cos(arg.value);
                    break;

                case MP_FUNCTION_TAN:
                    result.value = tan(arg.value);
                    break;

                case MP_FUNCTION_SQRT:
                    result.value = sqrt(arg.value);
                    break;

                default:
                    result.error = true;
                    result.error_type = MP_ERROR_INVALID_FUNCTION;
                    break;
            }

            return result;
        } break;

        case MP_NODE_ADD: {
            MP_Result a = mp_interpret_node(interpreter, root->binop.lhs);
            if (a.error) return a;
            MP_Result b = mp_interpret_node(interpreter, root->binop.rhs);
            if (b.error) return b;
            result.value = a.value + b.value;
        } break;

        case MP_NODE_SUBTRACT: {
            MP_Result a = mp_interpret_node(interpreter, root->binop.lhs);
            if (a.error) return a;
            MP_Result b = mp_interpret_node(interpreter, root->binop.rhs);
            if (b.error) return b;
            result.value = a.value - b.value;
        } break;

        case MP_NODE_MULTIPLY: {
            MP_Result a = mp_interpret_node(interpreter, root->binop.lhs);
            if (a.error) return a;
            MP_Result b = mp_interpret_node(interpreter, root->binop.rhs);
            if (b.error) return b;
            result.value = a.value * b.value;
        } break;

        case MP_NODE_DIVIDE: {
            MP_Result b = mp_interpret_node(interpreter, root->binop.rhs);
            if (b.error) return b;
            if (b.value == 0.0) {
                result.error = true;
                result.error_type = MP_ERROR_ZERO_DIVISION;
                return result;
            }

            MP_Result a = mp_interpret_node(interpreter, root->binop.lhs);
            if (a.error) return a;
            result.value = a.value / b.value;
        } break;

        case MP_NODE_POWER: {
            MP_Result b = mp_interpret_node(interpreter, root->binop.rhs);
            if (b.error) return b;
            MP_Result a = mp_interpret_node(interpreter, root->binop.lhs);
            if (a.error) return a;
            result.value = pow(a.value, b.value);
        } break;

        case MP_NODE_PLUS: {
            result = mp_interpret_node(interpreter, root->unary.node);
        } break;

        case MP_NODE_MINUS: {
            MP_Result n = mp_interpret_node(interpreter, root->unary.node);
            result.value = -n.value;
        } break;

        default: {
            result.error = true;
            result.error_type = MP_ERROR_INVALID_NODE;
            return result;
        } break;
    }

    return result;
}

MP_Interpreter mp_interpreter_init(MP_Parse_Tree tree, MP_Arena arena)
{
    MP_Interpreter intpr = {0};
    intpr.tree = tree;
    intpr.arena = arena;

    return intpr;
}

void mp_interpreter_var(MP_Interpreter *interpreter, char var, double value)
{
    if (interpreter == NULL)
        return;

    assert('a' <= var && var <= 'z');
    interpreter->vars[var - 'a'] = value;
}

void mp_interpreter_free(MP_Interpreter *interpreter)
{
    mp_arena_free(&interpreter->arena);
}

//----------
// Compiler
//----------

// TODO: Implement function evaluation in the compiler and the VM

bool mp_program_compile(MP_Program *p, MP_Parse_Tree parse_tree)
{
    if (p == NULL)
        return false;

    return mp_program_compile_node(p, parse_tree.root);
}

bool mp_program_compile_node(MP_Program *p, MP_Tree_Node *node)
{
    switch (node->type) {
        case MP_NODE_INVALID: {
            return false;
        } break;

        case MP_NODE_NUMBER: {
            mp_program_push_opcode(p, MP_OP_PUSH_NUM);
            mp_program_push_const(p, node->value);
        } break;

        case MP_NODE_SYMBOL: {
            assert('a' <= node->symbol && node->symbol <= 'z');
            mp_program_push_opcode(p, MP_OP_PUSH_VAR);
            mp_program_push_var(p, node->symbol - 'a');
        } break;

        case MP_NODE_ADD: {
            if (!mp_program_compile_node(p, node->binop.lhs)) return false;
            if (!mp_program_compile_node(p, node->binop.rhs)) return false;
            mp_program_push_opcode(p, MP_OP_ADD);
        } break;

        case MP_NODE_SUBTRACT: {
            if (!mp_program_compile_node(p, node->binop.lhs)) return false;
            if (!mp_program_compile_node(p, node->binop.rhs)) return false;
            mp_program_push_opcode(p, MP_OP_SUB);
        } break;

        case MP_NODE_MULTIPLY: {
            if (!mp_program_compile_node(p, node->binop.lhs)) return false;
            if (!mp_program_compile_node(p, node->binop.rhs)) return false;
            mp_program_push_opcode(p, MP_OP_MUL);
        } break;

        case MP_NODE_DIVIDE: {
            if (!mp_program_compile_node(p, node->binop.lhs)) return false;
            if (!mp_program_compile_node(p, node->binop.rhs)) return false;
            mp_program_push_opcode(p, MP_OP_DIV);
        } break;

        case MP_NODE_POWER: {
            if (!mp_program_compile_node(p, node->binop.lhs)) return false;
            if (!mp_program_compile_node(p, node->binop.rhs)) return false;
            mp_program_push_opcode(p, MP_OP_POW);
        } break;

        case MP_NODE_PLUS: {
            if (!mp_program_compile_node(p, node->unary.node)) return false;
        } break;

        case MP_NODE_MINUS: {
            if (!mp_program_compile_node(p, node->unary.node)) return false;
            mp_program_push_opcode(p, MP_OP_NEG);
        } break;

        default: {
            return false;
        } break;
    }

    return true;
}

void mp_program_push_opcode(MP_Program *p, MP_Opcode op)
{
    if (p == NULL)
        return;

    mp_da_append(p, op);
}

void mp_program_push_const(MP_Program *p, double value)
{
    if (p == NULL)
        return;

    for (size_t i = 0; i < sizeof(value); ++i) {
        mp_da_append(p, 0);
    }
    double *loc = (double*)((p->items + p->count) - sizeof(*loc));
    *loc = value;
}

void mp_program_push_var(MP_Program *p, char var)
{
    if (p == NULL)
        return;

    mp_da_append(p, var);
}

void mp_print_program(MP_Program p)
{
    size_t ip = 0;
    for (size_t i = 0; i < p.count; ++i) {
        MP_Opcode op = p.items[i];

        switch (op) {
            case MP_OP_PUSH_NUM: {
                printf("%ld: PUSH_NUM ", ip++);
                double num = 0.0;

                if (i + sizeof(num) >= p.count)
                    continue;

                ++i;
                num = *(double*)&p.items[i];
                i += sizeof(num) - 1;

                printf("%f\n", num);
            } break;

            case MP_OP_PUSH_VAR: {
                printf("%ld: PUSH_VAR ", ip++);
                char var = 0;

                if (i + sizeof(var) >= p.count)
                    continue;

                ++i;
                var = p.items[i] + 'a';
                // i += sizeof(var) - 1; // Increment by 0

                printf("%c\n", var);
            } break;

            case MP_OP_ADD: printf("%ld: ADD\n", ip++); break;
            case MP_OP_SUB: printf("%ld: SUB\n", ip++); break;
            case MP_OP_MUL: printf("%ld: MUL\n", ip++); break;
            case MP_OP_DIV: printf("%ld: DIV\n", ip++); break;
            case MP_OP_POW: printf("%ld: POW\n", ip++); break;
            case MP_OP_NEG: printf("%ld: NEG\n", ip++); break;

            default: {
                printf("%ld: ?\n", ip++);
            } break;
        }
    }
}

void mp_stack_push(MP_Stack *stack, double n)
{
    if (stack == NULL)
        return;

    mp_da_append(stack, n);
}

MP_Optional mp_stack_pop(MP_Stack *stack)
{
    MP_Optional result = mp_stack_peek(stack);
    if (result.present) {
        stack->count--;
    }

    return result;
}

MP_Optional mp_stack_peek(MP_Stack *stack)
{
    MP_Optional result = {0};
    if (stack == NULL || stack->count == 0) {
        return result;
    }

    result.present = true;
    result.value = stack->items[stack->count - 1];
    return result;
}

MP_Vm mp_vm_init(MP_Program program)
{
    MP_Vm vm = {0};
    vm.program = program;
    return vm;
}

void mp_vm_var(MP_Vm *vm, char var, double value)
{
    if (vm == NULL)
        return;

    assert('a' <= var && var <= 'z');
    vm->vars[var - 'a'] = value;
}

bool mp_vm_run(MP_Vm *vm)
{
#define ASSERT_PRESENT(o) if (!(o).present) return false

    if (vm == NULL)
        return false;;

    MP_Stack *stack = &vm->stack;
    MP_Program *program = &vm->program;
    vm->ip = 0;

    while (vm->ip < program->count) {
        MP_Opcode op = program->items[vm->ip];

        switch (op) {
            case MP_OP_PUSH_NUM: {
                ++vm->ip;
                double operand = *(double*)&program->items[vm->ip];
                mp_stack_push(stack, operand);
                vm->ip += sizeof(operand);
            } break;

            case MP_OP_PUSH_VAR: {
                ++vm->ip;
                char var = *(char*)&program->items[vm->ip];
                mp_stack_push(stack, vm->vars[(int)var]);
                vm->ip += sizeof(var);
            } break;

            case MP_OP_ADD: {
                MP_Optional b = mp_stack_pop(stack); ASSERT_PRESENT(b);
                MP_Optional a = mp_stack_pop(stack); ASSERT_PRESENT(a);
                mp_stack_push(stack, a.value + b.value);
                ++vm->ip;
            } break;

            case MP_OP_SUB: {
                MP_Optional b = mp_stack_pop(stack); ASSERT_PRESENT(b);
                MP_Optional a = mp_stack_pop(stack); ASSERT_PRESENT(a);
                mp_stack_push(stack, a.value - b.value);
                ++vm->ip;
            } break;

            case MP_OP_MUL: {
                MP_Optional b = mp_stack_pop(stack); ASSERT_PRESENT(b);
                MP_Optional a = mp_stack_pop(stack); ASSERT_PRESENT(a);
                mp_stack_push(stack, a.value * b.value);
                ++vm->ip;
            } break;

            case MP_OP_DIV: {
                MP_Optional b = mp_stack_pop(stack); ASSERT_PRESENT(b);
                MP_Optional a = mp_stack_pop(stack); ASSERT_PRESENT(a);
                mp_stack_push(stack, a.value / b.value);
                ++vm->ip;
            } break;

            case MP_OP_POW: {
                MP_Optional b = mp_stack_pop(stack); ASSERT_PRESENT(b);
                MP_Optional a = mp_stack_pop(stack); ASSERT_PRESENT(a);
                mp_stack_push(stack, pow(a.value, b.value));
                ++vm->ip;
            } break;

            case MP_OP_NEG: {
                MP_Optional n = mp_stack_pop(stack); ASSERT_PRESENT(n);
                mp_stack_push(stack, -n.value);
                ++vm->ip;
            } break;

            default: {
                return false;
            } break;
        }
    }

    return true;

#undef ASSERT_PRESENT
}

double mp_vm_result(MP_Vm *vm)
{
    if (vm == NULL)
        return 0.0;

    return mp_stack_peek(&vm->stack).value;
}

void mp_vm_free(MP_Vm *vm)
{
    if (vm == NULL)
        return;

    mp_da_free(&vm->stack);
    mp_da_free(&vm->program);
}

//----------------
// Simplified API
//----------------

// TODO: In this API, MP_Result's won't contain detailed data about errors

MP_Env *mp_init(const char *expression)
{
    return mp_init_mode(expression, MP_MODE_INTERPRET);
}

MP_Env *mp_init_mode(const char *expression, MP_Mode mode)
{
    if (expression == NULL) {
        return NULL;
    }

    MP_Env *env = malloc(sizeof(*env));
    if (env == NULL) {
        return NULL;
    }
    memset(env, 0, sizeof(*env));

    env->mode = mode;

    MP_Token_List token_list = {0};

    MP_Result tr = mp_tokenize(&token_list, expression);
    if (tr.error) {
        free(env);
        mp_da_free(&token_list);
        return NULL;
    }

    MP_Arena arena = {0};
    MP_Parse_Tree parse_tree = {0};

    MP_Result pr = mp_parse(&arena, &parse_tree, token_list);
    if (pr.error) {
        free(env);
        mp_da_free(&token_list);
        mp_arena_free(&arena);
        return NULL;
    }

    mp_da_free(&token_list);

    switch (env->mode) {
        case MP_MODE_INTERPRET: {
            env->interpreter = mp_interpreter_init(parse_tree, arena);
        } break;

        case MP_MODE_COMPILE: {
            MP_Program program = {0};

            if (!mp_program_compile(&program, parse_tree)) {
                free(env);
                mp_arena_free(&arena);
                mp_da_free(&program);
                return NULL;
            }

            mp_arena_free(&arena);

            env->vm = mp_vm_init(program);
        } break;

        default: {
            assert(false && "Unreachable MP_MODE");
        } break;
    }

    mp_variable(env, 'p', MP_PI);
    mp_variable(env, 'e', MP_E);

    return env;

}

void mp_variable(MP_Env *env, char var, double value)
{
    if (env == NULL)
        return;

    switch (env->mode) {
        case MP_MODE_INTERPRET: {
            mp_interpreter_var(&env->interpreter, var, value);
        } break;

        case MP_MODE_COMPILE: {
            mp_vm_var(&env->vm, var, value);
        } break;

        default: {
            assert(false && "Unreachable MP_MODE");
        } break;
    }
}

MP_Result mp_evaluate(MP_Env *env)
{
    MP_Result result = {0};

    if (env == NULL) {
        result.error = true;
        return result;
    }

    switch (env->mode) {
        case MP_MODE_INTERPRET: {
            result = mp_interpret(&env->interpreter);
        } break;

        case MP_MODE_COMPILE: {
            if (!mp_vm_run(&env->vm)) {
                result.error = true;
                return result;
            }

            result.value = mp_vm_result(&env->vm);
        } break;

        default: {
            assert(false && "Unreachable MP_MODE");
        } break;
    }

    return result;
}

void mp_free(MP_Env *env)
{
    if (env == NULL)
        return;

    switch (env->mode) {
        case MP_MODE_INTERPRET: {
            mp_interpreter_free(&env->interpreter);
        } break;

        case MP_MODE_COMPILE: {
            mp_vm_free(&env->vm);
        } break;

        default: {
            assert(false && "Unreachable MP_MODE");
        } break;
    }

    free(env);
}

#endif // MP_IMPLEMENTATION

/*
    Revision history:

        1.4.0 (2025-06-01) Add functions log(), cos(), tan(), sqrt()
        1.3.0 (2025-06-01) Add function support (ln, sin) to the interpreter
        1.2.0 (2025-06-01) Now interpreter supports variables. Various fixes. Improved modularity
        1.1.4 (2025-05-23) Set mathematical constants in mp_init such as PI and E
        1.1.3 (2025-05-23) Fix operator precedence for exponentiation
        1.1.2 (2025-05-19) Check if input MP_Env is NULL in mp_free
        1.1.1 (2025-05-19) Check if input expression is NULL in mp_init
        1.1.0 (2025-03-12) Implement exponentiation
        1.0.2 (2025-03-12) Remove unused macro
        1.0.1 (2025-03-12) Fix inconsistency of MP_Env memory on initialization
        1.0.0 (2025-03-12) Initial release
*/

/*
 * MIT License
 * 
 * Copyright (c) 2025 seajee
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
