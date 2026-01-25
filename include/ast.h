#ifndef AST_H
#define AST_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    int first_line;
    int first_column;
    int last_line;
    int last_column;
} Location;
#ifndef YYLTYPE_IS_DECLARED
#define YYLTYPE_IS_DECLARED
typedef struct YYLTYPE {
    int first_line;
    int first_column;
    int last_line;
    int last_column;
} YYLTYPE;
#endif

typedef struct ASTNode ASTNode;

typedef enum {
    AST_PROGRAM,
    AST_PRINT,
    AST_ASSIGN,
    AST_CONST,
    AST_BINOP,
    AST_UNARYOP,
    AST_NUM_INT,
    AST_NUM_FLOAT,
    AST_STRING,
    AST_IDENTIFIER,
    AST_TYPE_INT32,
    AST_TYPE_INT64,
    AST_TYPE_FLOAT32,
    AST_TYPE_FLOAT64,
    AST_TYPE_STRING,
    AST_TYPE_VOID,
    AST_EXPRESSION_LIST,
    AST_INDEX,
    AST_INPUT,
    AST_TOINT,
    AST_TOFLOAT,
    AST_IF,
    AST_WHILE,
    AST_BREAK,
    AST_CONTINUE,
    AST_FOR,
    AST_FUNCTION,
    AST_RETURN,
    AST_CALL
} NodeType;

typedef enum {
    OP_ADD,      // +
    OP_SUB,      // -
    OP_MUL,      // *
    OP_DIV,      // /
    OP_MOD,      // %
    OP_POW,      // **
    OP_CONCAT,   // +
    OP_REPEAT,   // *
    OP_EQ,       // ==
    OP_NE,       // !=
    OP_LT,       // <
    OP_LE,       // <=
    OP_GT,       // >
    OP_GE        // >=
} BinOpType;

typedef enum {
    OP_MINUS,    // -
    OP_PLUS      // +
} UnaryOpType;

typedef struct ASTNode {
    NodeType type;
    Location location;// 位置信息
    union {
        struct {
            struct ASTNode** statements;
            int statement_count;
        } program;
        struct {
            struct ASTNode* expr;
        } print;
        struct {
            struct ASTNode** expressions;
            int expression_count;
        } expression_list;
        struct {
            struct ASTNode* target;
            struct ASTNode* index;
        } index;
        struct {
            struct ASTNode* left;
            struct ASTNode* right;
        } assign;
        struct {
            struct ASTNode* left;
            struct ASTNode* right;
            BinOpType op;
        } binop;
        struct {
            struct ASTNode* expr;
            UnaryOpType op;
        } unaryop;
        struct {
            long long value;
        } num_int;
        struct {
            double value;
        } num_float;
        struct {
            char* value;
        } string;
        struct {
            char* name;
        } identifier;
        struct {
            struct ASTNode* prompt;
        } input;
        struct {
            struct ASTNode* expr;
        } toint;
        struct {
            struct ASTNode* expr;
        } tofloat;
        struct {
            struct ASTNode* condition;
            struct ASTNode* then_body;
            struct ASTNode* else_body;  // 可以为NULL
        } if_stmt;
        struct {
            struct ASTNode* condition;
            struct ASTNode* body;
        } while_stmt;
        struct {
            struct ASTNode* var;
            struct ASTNode* start;
            struct ASTNode* end;
            struct ASTNode* body;
        } for_stmt;
        struct {
            char* name;
            struct ASTNode* params;
            struct ASTNode* return_type;
            struct ASTNode* body;
        } function;
        struct {
            struct ASTNode* func;
            struct ASTNode* args;
        } call;
        struct {
            struct ASTNode* expr;
        } return_stmt;
    } data;
} ASTNode;

ASTNode* create_program_node();
ASTNode* create_program_node_with_location(Location location);
ASTNode* create_program_node_with_yyltype(void* yylloc);
void add_statement_to_program(ASTNode* program, ASTNode* statement);
ASTNode* create_print_node(ASTNode* expr);
ASTNode* create_print_node_with_location(ASTNode* expr, Location location);
ASTNode* create_print_node_with_yyltype(ASTNode* expr, void* yylloc);
ASTNode* create_input_node(ASTNode* prompt);
ASTNode* create_input_node_with_location(ASTNode* prompt, Location location);
ASTNode* create_toint_node(ASTNode* expr);
ASTNode* create_toint_node_with_location(ASTNode* expr, Location location);
ASTNode* create_toint_node_with_yyltype(ASTNode* expr, void* yylloc);
ASTNode* create_tofloat_node(ASTNode* expr);
ASTNode* create_tofloat_node_with_location(ASTNode* expr, Location location);
ASTNode* create_tofloat_node_with_yyltype(ASTNode* expr, void* yylloc);
ASTNode* create_expression_list_node();
ASTNode* create_expression_list_node_with_location(Location location);
ASTNode* create_expression_list_node_with_yyltype(void* yylloc);
void add_expression_to_list(ASTNode* list, ASTNode* expr);
ASTNode* create_assign_node(ASTNode* left, ASTNode* right);
ASTNode* create_assign_node_with_location(ASTNode* left, ASTNode* right, Location location);
ASTNode* create_assign_node_with_yyltype(ASTNode* left, ASTNode* right, void* yylloc);
ASTNode* create_const_node(ASTNode* left, ASTNode* right);
ASTNode* create_const_node_with_location(ASTNode* left, ASTNode* right, Location location);
ASTNode* create_const_node_with_yyltype(ASTNode* left, ASTNode* right, void* yylloc);
ASTNode* create_binop_node(BinOpType op, ASTNode* left, ASTNode* right);
ASTNode* create_binop_node_with_location(BinOpType op, ASTNode* left, ASTNode* right, Location location);
ASTNode* create_binop_node_with_yyltype(BinOpType op, ASTNode* left, ASTNode* right, void* yylloc);
ASTNode* create_unaryop_node(UnaryOpType op, ASTNode* expr);
ASTNode* create_unaryop_node_with_location(UnaryOpType op, ASTNode* expr, Location location);
ASTNode* create_num_int_node(long long value);
ASTNode* create_num_int_node_with_location(long long value, Location location);
ASTNode* create_num_int_node_with_yyltype(long long value, void* yylloc);
ASTNode* create_num_float_node(double value);
ASTNode* create_num_float_node_with_location(double value, Location location);
ASTNode* create_num_float_node_with_yyltype(double value, void* yylloc);
ASTNode* create_string_node(const char* value);
ASTNode* create_string_node_with_location(const char* value, Location location);
ASTNode* create_string_node_with_yyltype(const char* value, void* yylloc);
ASTNode* create_identifier_node(const char* name);
ASTNode* create_identifier_node_with_location(const char* name, Location location);
ASTNode* create_identifier_node_with_yyltype(const char* name, void* yylloc);
ASTNode* create_type_node(NodeType type);
ASTNode* create_type_node_with_location(NodeType type, Location location);
ASTNode* create_if_node(ASTNode* condition, ASTNode* then_body, ASTNode* else_body);
ASTNode* create_if_node_with_location(ASTNode* condition, ASTNode* then_body, ASTNode* else_body, Location location);
ASTNode* create_if_node_with_yyltype(ASTNode* condition, ASTNode* then_body, ASTNode* else_body, void* yylloc);
ASTNode* create_while_node(ASTNode* condition, ASTNode* body);
ASTNode* create_while_node_with_location(ASTNode* condition, ASTNode* body, Location location);
ASTNode* create_while_node_with_yyltype(ASTNode* condition, ASTNode* body, void* yylloc);
ASTNode* create_for_node(ASTNode* var, ASTNode* start, ASTNode* end, ASTNode* body);
ASTNode* create_for_node_with_location(ASTNode* var, ASTNode* start, ASTNode* end, ASTNode* body, Location location);
ASTNode* create_for_node_with_yyltype(ASTNode* var, ASTNode* start, ASTNode* end, ASTNode* body, void* yylloc);
ASTNode* create_break_node();
ASTNode* create_break_node_with_location(Location location);
ASTNode* create_break_node_with_yyltype(void* yylloc);
ASTNode* create_continue_node();
ASTNode* create_continue_node_with_location(Location location);
ASTNode* create_continue_node_with_yyltype(void* yylloc);
ASTNode* create_function_node(const char* name, ASTNode* params, ASTNode* return_type, ASTNode* body);
ASTNode* create_function_node_with_location(const char* name, ASTNode* params, ASTNode* return_type, ASTNode* body, Location location);
ASTNode* create_return_node(ASTNode* expr);
ASTNode* create_return_node_with_location(ASTNode* expr, Location location);
ASTNode* create_return_node_with_yyltype(ASTNode* expr, void* yylloc);
ASTNode* create_call_node(ASTNode* func, ASTNode* args);
ASTNode* create_call_node_with_location(ASTNode* func, ASTNode* args, Location location);
ASTNode* create_call_node_with_yyltype(ASTNode* func, ASTNode* args, void* yylloc);
ASTNode* create_index_node(ASTNode* target, ASTNode* index);
ASTNode* create_index_node_with_location(ASTNode* target, ASTNode* index, Location location);
ASTNode* create_index_node_with_yyltype(ASTNode* target, ASTNode* index, void* yylloc);
void free_ast(ASTNode* node);
void print_ast(ASTNode* node, int indent);

#endif//AST_H

