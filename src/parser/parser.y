%{
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/ast.h"
#include "../include/compiler.h"
extern int yylex();
extern int yyparse();
extern FILE* yyin;
extern int yylineno;
extern char* yytext;
extern const char* current_input_filename; 
void yyerror(const char* s);
ASTNode* root;
%}

%union {
    long long num_int;
    double num_float;
    char* str;
    struct ASTNode* node;
}

%token <str> IDENTIFIER STRING CHAR_LITERAL
%token STRUCT COLON
%token CONST MUT GLOBAL
%token IMPORT PUB
%token <num_int> NUMBER_INT
%token <num_float> NUMBER_FLOAT
%token PRINT INPUT TOINT TOFLOAT TYPE_I32 TYPE_I64 TYPE_I8 TYPE_F32 TYPE_F64 TYPE_STR TYPE_PTR FN ARROW RETURN TYPE_VOID NIL EXTERN DOTDOTDOT
%token AND OR
%token AT AMPERSAND
%token IF ELSE ELIF WHILE FOR BREAK CONTINUE IN
%token ASSIGN PLUS_ASSIGN MINUS_ASSIGN MULTIPLY_ASSIGN DIVIDE_ASSIGN MODULO_ASSIGN
%token PLUS MINUS MULTIPLY DIVIDE MODULO POWER
%token EQ NE LT LE GT GE
%token DOT DOTDOT LBRACKET RBRACKET
%token LPAREN RPAREN LBRACE RBRACE SEMICOLON COMMA
%token ERROR
%left OR
%left AND
%nonassoc EQ NE LT LE GT GE
%left PLUS MINUS
%left MULTIPLY DIVIDE MODULO
%right POWER

%type <node> program statement_list statement 
%type <node> extern_block extern_decl extern_decl_list
%type <node> struct_fields struct_field struct_init_fields struct_init_field
%type <node> type param_list function_definition pub_function_definition
%type <node> print_statement assignment_statement compound_assignment_statement
%type <node> input_statement if_statement while_statement for_statement
%type <node> expression logical_expression comparison_expression term factor power factor_unary
%type <node> literal identifier toint_expression tofloat_expression input_expression
%type <node> block_statement if_rest expression_list
%type <node> lvalue

%nonassoc IF
%nonassoc ELSE

%start program

%%

program
    : statement_list { root = $$ = $1; }
    ;

toint_expression
    : TOINT LPAREN expression RPAREN { $$ = create_toint_node_with_yyltype($3, (YYLTYPE*) &@$); }
    ;

tofloat_expression
    : TOFLOAT LPAREN expression RPAREN { $$ = create_tofloat_node_with_yyltype($3, (YYLTYPE*) &@$); }
    ;

statement_list
    : statement                  { 
                                   $$ = create_program_node_with_yyltype((YYLTYPE*) &@$);
                                   add_statement_to_program($$, $1);
                               }
    | statement_list statement   {
                                   add_statement_to_program($1, $2);
                                   $$ = $1;
                               }
    ;

statement
    : print_statement SEMICOLON     { $$ = $1; }
    | assignment_statement SEMICOLON { $$ = $1; }
    | lvalue ASSIGN expression SEMICOLON { $$ = create_assign_node_with_yyltype($1, $3, (YYLTYPE*) &@$); }
    | identifier COLON expression SEMICOLON { $$ = create_assign_node_with_yyltype($1, $3, (YYLTYPE*) &@$); }
    | identifier COLON type ASSIGN expression SEMICOLON { 
        $$ = create_assign_node_with_yyltype($1, $5, (YYLTYPE*) &@$); 
    }
    | MUT identifier ASSIGN expression SEMICOLON { 
        $$ = create_assign_node_with_yyltype($2, $4, (YYLTYPE*) &@$); 
        $2->mutability = MUTABILITY_MUTABLE;
    }
    | MUT identifier COLON type ASSIGN expression SEMICOLON { 
        $$ = create_assign_node_with_yyltype($2, $6, (YYLTYPE*) &@$); 
        $2->mutability = MUTABILITY_MUTABLE;
    }
    | IMPORT STRING SEMICOLON { $$ = create_import_node_with_yyltype($2, (YYLTYPE*) &@$); }
    | IMPORT STRING { $$ = create_import_node_with_yyltype($2, (YYLTYPE*) &@$); }
    | CONST identifier ASSIGN expression SEMICOLON { $$ = create_const_node_with_yyltype($2, $4, (YYLTYPE*) &@$); }
    | GLOBAL identifier ASSIGN expression SEMICOLON { $$ = create_global_node_with_yyltype($2, NULL, $4, (YYLTYPE*) &@$); }
    | GLOBAL identifier COLON type ASSIGN expression SEMICOLON { $$ = create_global_node_with_yyltype($2, $4, $6, (YYLTYPE*) &@$); }
    | compound_assignment_statement SEMICOLON { $$ = $1; }
    | input_statement SEMICOLON     { $$ = $1; }
    | if_statement                  { $$ = $1; }
    | while_statement               { $$ = $1; }
    | for_statement               { $$ = $1; }
    | print_statement               { $$ = $1; }
    | assignment_statement          { $$ = $1; }
    | lvalue ASSIGN expression { $$ = create_assign_node_with_yyltype($1, $3, (YYLTYPE*) &@$); }
    | identifier COLON expression { $$ = create_assign_node_with_yyltype($1, $3, (YYLTYPE*) &@$); }
    | identifier COLON type ASSIGN expression { 
        $$ = create_assign_node_with_yyltype($1, $5, (YYLTYPE*) &@$); 
    }
    | compound_assignment_statement { $$ = $1; }
    | input_statement               { $$ = $1; }
    | pub_function_definition           { $$ = $1; }
    | function_definition           { $$ = $1; }
    | extern_block                 { $$ = $1; }
    | STRUCT IDENTIFIER LBRACE struct_fields RBRACE { $$ = create_struct_def_node_with_yyltype($2, $4, (YYLTYPE*) &@$); }
    | RETURN expression SEMICOLON  { $$ = create_return_node_with_yyltype($2, (YYLTYPE*) &@$); }
    | RETURN expression            { $$ = create_return_node_with_yyltype($2, (YYLTYPE*) &@$); }
    | expression SEMICOLON         { $$ = $1; }
    | expression                   { $$ = $1; }
    | CONST identifier ASSIGN expression { $$ = create_const_node_with_yyltype($2, $4, (YYLTYPE*) &@$); }
    | GLOBAL identifier ASSIGN expression { $$ = create_global_node_with_yyltype($2, NULL, $4, (YYLTYPE*) &@$); }
    | GLOBAL identifier COLON type ASSIGN expression { $$ = create_global_node_with_yyltype($2, $4, $6, (YYLTYPE*) &@$); }
    | identifier COLON expression { $$ = create_assign_node_with_yyltype($1, $3, (YYLTYPE*) &@$); }
    | MUT identifier ASSIGN expression { 
        $$ = create_assign_node_with_yyltype($2, $4, (YYLTYPE*) &@$); 
        $2->mutability = MUTABILITY_MUTABLE;
    }
    | MUT identifier COLON type ASSIGN expression { 
        $$ = create_assign_node_with_yyltype($2, $6, (YYLTYPE*) &@$); 
        $2->mutability = MUTABILITY_MUTABLE;
    }
    | BREAK SEMICOLON               { $$ = create_break_node_with_yyltype((YYLTYPE*) &@$); }
    | CONTINUE SEMICOLON            { $$ = create_continue_node_with_yyltype((YYLTYPE*) &@$); }
    | BREAK                         { $$ = create_break_node_with_yyltype((YYLTYPE*) &@$); }
    | CONTINUE                      { $$ = create_continue_node_with_yyltype((YYLTYPE*) &@$); }
    ;

input_expression
    : INPUT LPAREN STRING RPAREN { 
        ASTNode* prompt = create_string_node_with_yyltype($3, (YYLTYPE*) &@$);
        $$ = create_input_node_with_yyltype(prompt, (YYLTYPE*) &@$); 
    }
    | INPUT LPAREN RPAREN { 
        ASTNode* prompt = create_string_node_with_yyltype("", (YYLTYPE*) &@$);
        $$ = create_input_node_with_yyltype(prompt, (YYLTYPE*) &@$); 
    }
    ;
struct_field
    : IDENTIFIER COLON type { $$ = create_assign_node_with_yyltype(create_identifier_node_with_yyltype($1, (YYLTYPE*) &@$), $3, (YYLTYPE*) &@$); }
    ;

struct_fields
    : /* empty */ { $$ = create_expression_list_node_with_yyltype((YYLTYPE*) &@$); }
    | struct_field { ASTNode* list = create_expression_list_node_with_yyltype((YYLTYPE*) &@$); add_expression_to_list(list, $1); $$ = list; }
    | struct_fields COMMA struct_field { add_expression_to_list($1, $3); $$ = $1; }
    | struct_fields struct_field { add_expression_to_list($1, $2); $$ = $1; }
    ;
struct_init_field
    : IDENTIFIER COLON expression { $$ = create_assign_node_with_yyltype(create_identifier_node_with_yyltype($1, (YYLTYPE*) &@$), $3, (YYLTYPE*) &@$); }
    ;

struct_init_fields
    : /* empty */ { $$ = create_expression_list_node_with_yyltype((YYLTYPE*) &@$); }
    | struct_init_field { ASTNode* list = create_expression_list_node_with_yyltype((YYLTYPE*) &@$); add_expression_to_list(list, $1); $$ = list; }
    | struct_init_fields COMMA struct_init_field { add_expression_to_list($1, $3); $$ = $1; }
    | struct_init_fields struct_init_field { add_expression_to_list($1, $2); $$ = $1; }
    ;

type
    : TYPE_I32 { $$ = create_type_node(AST_TYPE_INT32); }
    | TYPE_I64 { $$ = create_type_node(AST_TYPE_INT64); }
    | TYPE_I8 { $$ = create_type_node(AST_TYPE_INT8); }
    | TYPE_F32 { $$ = create_type_node(AST_TYPE_FLOAT32); }
    | TYPE_F64 { $$ = create_type_node(AST_TYPE_FLOAT64); }
    | TYPE_STR { $$ = create_type_node(AST_TYPE_STRING); }
    | TYPE_VOID { $$ = create_type_node(AST_TYPE_VOID); }
    | TYPE_PTR LPAREN type RPAREN { $$ = create_type_node(AST_TYPE_POINTER); }
    | AMPERSAND TYPE_I32 { $$ = create_type_node(AST_TYPE_POINTER); }
    | AMPERSAND TYPE_I64 { $$ = create_type_node(AST_TYPE_POINTER); }
    | AMPERSAND TYPE_I8 { $$ = create_type_node(AST_TYPE_POINTER); }
    | AMPERSAND TYPE_F32 { $$ = create_type_node(AST_TYPE_POINTER); }
    | AMPERSAND TYPE_F64 { $$ = create_type_node(AST_TYPE_POINTER); }
    | AMPERSAND TYPE_STR { $$ = create_type_node(AST_TYPE_POINTER); }
    | AMPERSAND IDENTIFIER { $$ = create_type_node(AST_TYPE_POINTER); }
    | LBRACKET type MULTIPLY NUMBER_INT RBRACKET { $$ = create_fixed_size_list_type_node($2, $4); }
    | LBRACKET type RBRACKET { $$ = create_list_type_node($2); }
    | IDENTIFIER {
        $$ = create_identifier_node_with_yyltype($1, (YYLTYPE*) &@$); 
    }
    ;

param_list
    : identifier { ASTNode* list = create_expression_list_node_with_yyltype((YYLTYPE*) &@$); add_expression_to_list(list, $1); $$ = list; }
    | identifier COLON type {
        ASTNode* id_node = create_identifier_node_with_yyltype($1->data.identifier.name, (YYLTYPE*) &@$);
        ASTNode* annotated_param = create_assign_node_with_yyltype(id_node, $3, (YYLTYPE*) &@$);
        ASTNode* list = create_expression_list_node_with_yyltype((YYLTYPE*) &@$);
        add_expression_to_list(list, annotated_param);
        $$ = list;
    }
    | MUT identifier {
        ASTNode* list = create_expression_list_node_with_yyltype((YYLTYPE*) &@$);
        ASTNode* id_node = create_identifier_node_with_yyltype($2->data.identifier.name, (YYLTYPE*) &@$);
        add_expression_to_list(list, id_node);
        $$ = list;
    }
    | MUT identifier COLON type {
        ASTNode* list = create_expression_list_node_with_yyltype((YYLTYPE*) &@$);
        ASTNode* id_node = create_identifier_node_with_yyltype($2->data.identifier.name, (YYLTYPE*) &@$);
        ASTNode* annotated_param = create_assign_node_with_mutability(id_node, $4, MUTABILITY_MUTABLE);
        add_expression_to_list(list, annotated_param);
        $$ = list;
    }
    | param_list COMMA identifier { add_expression_to_list($1, $3); $$ = $1; }
    | param_list COMMA identifier COLON type {
        ASTNode* id_node = create_identifier_node_with_yyltype($3->data.identifier.name, (YYLTYPE*) &@$);
        ASTNode* annotated_param = create_assign_node_with_yyltype(id_node, $5, (YYLTYPE*) &@$);
        add_expression_to_list($1, annotated_param);
        $$ = $1;
    }
    | param_list COMMA MUT identifier {
        ASTNode* id_node = create_identifier_node_with_yyltype($4->data.identifier.name, (YYLTYPE*) &@$);
        add_expression_to_list($1, id_node);
        $$ = $1;
    }
    | param_list COMMA MUT identifier COLON type {
        ASTNode* id_node = create_identifier_node_with_yyltype($4->data.identifier.name, (YYLTYPE*) &@$);
        ASTNode* annotated_param = create_assign_node_with_mutability(id_node, $6, MUTABILITY_MUTABLE);
        add_expression_to_list($1, annotated_param);
        $$ = $1;
    }
    ;

pub_function_definition
    : PUB FN IDENTIFIER LPAREN RPAREN ARROW type LBRACE statement_list RBRACE {
        $$ = create_public_function_node($3, NULL, $7, $9);
    }
    | PUB FN IDENTIFIER LPAREN param_list RPAREN ARROW type LBRACE statement_list RBRACE {
        $$ = create_public_function_node($3, $5, $8, $10);
    }
    | PUB FN IDENTIFIER LPAREN RPAREN LBRACE statement_list RBRACE {
        ASTNode* void_type = create_type_node(AST_TYPE_VOID);
        $$ = create_public_function_node($3, NULL, void_type, $7);
    }
    | PUB FN IDENTIFIER LPAREN param_list RPAREN LBRACE statement_list RBRACE {
        ASTNode* void_type = create_type_node(AST_TYPE_VOID);
        $$ = create_public_function_node($3, $5, void_type, $8);
    }
    ;

function_definition
    : FN IDENTIFIER LPAREN RPAREN ARROW type LBRACE statement_list RBRACE {
        $$ = create_function_node($2, NULL, $6, $8);
        $$->data.function.is_public = 0;
    }
    | FN IDENTIFIER LPAREN param_list RPAREN ARROW type LBRACE statement_list RBRACE {
        $$ = create_function_node($2, $4, $7, $9);
        $$->data.function.is_public = 0;
    }
    | FN IDENTIFIER LPAREN RPAREN LBRACE statement_list RBRACE {
        ASTNode* void_type = create_type_node(AST_TYPE_VOID);
        $$ = create_function_node($2, NULL, void_type, $6);
        $$->data.function.is_public = 0;
    }
    | FN IDENTIFIER LPAREN param_list RPAREN LBRACE statement_list RBRACE {
        ASTNode* void_type = create_type_node(AST_TYPE_VOID);
        $$ = create_function_node($2, $4, void_type, $7);
        $$->data.function.is_public = 0;
    }
    ;

extern_decl
    : FN IDENTIFIER LPAREN RPAREN ARROW type {
        $$ = create_extern_function_node($2, NULL, $6, NULL);
    }
    | FN IDENTIFIER LPAREN param_list RPAREN ARROW type {
        $$ = create_extern_function_node($2, $4, $7, NULL);
    }
    | FN IDENTIFIER LPAREN param_list COMMA DOTDOTDOT RPAREN ARROW type {
        ASTNode* fn = create_extern_function_node($2, $4, $9, NULL);
        fn->data.function.vararg = 1;
        $$ = fn;
    }
    | FN IDENTIFIER LPAREN DOTDOTDOT RPAREN ARROW type {
        ASTNode* fn = create_extern_function_node($2, NULL, $7, NULL);
        fn->data.function.vararg = 1;
        $$ = fn;
    }

extern_decl_list
    : /* empty */ { $$ = create_expression_list_node(); }
    | extern_decl { ASTNode* list = create_expression_list_node(); add_expression_to_list(list, $1); $$ = list; }
    | extern_decl_list extern_decl { add_expression_to_list($1, $2); $$ = $1; }

extern_block
    : EXTERN STRING LBRACE extern_decl_list RBRACE {
        ASTNode* prog = create_program_node();
        if ($4 && $4->type == AST_EXPRESSION_LIST) {
            int cnt = $4->data.expression_list.expression_count;
            for (int i = 0; i < cnt; i++) {
                ASTNode* fn = $4->data.expression_list.expressions[i];
                if (fn && fn->type == AST_FUNCTION) {
                    fn->data.function.is_extern = 1;
                    if ($2) {
                        if (fn->data.function.linkage) free(fn->data.function.linkage);
                        fn->data.function.linkage = malloc(strlen($2) + 1);
                        strcpy(fn->data.function.linkage, $2);
                    }
                    add_statement_to_program(prog, fn);
                }
            }
        }
        $$ = prog;
    }

print_statement
    : PRINT expression              { $$ = create_print_node_with_yyltype($2, (YYLTYPE*) &@$); }
    | PRINT LPAREN expression RPAREN { $$ = create_print_node_with_yyltype($3, (YYLTYPE*) &@$); }
    | PRINT LPAREN expression_list RPAREN { $$ = create_print_node_with_yyltype($3, (YYLTYPE*) &@$); }
    ;

assignment_statement
    : identifier ASSIGN expression  { $$ = create_assign_node_with_yyltype($1, $3, (YYLTYPE*) &@$); }
    ;

lvalue
    : identifier                    { $$ = $1; }
    | factor_unary DOT identifier   { $$ = create_member_access_node_with_yyltype($1, $3, (YYLTYPE*) &@$); }
    | factor_unary LBRACKET expression RBRACKET { $$ = create_index_node_with_yyltype($1, $3, (YYLTYPE*) &@$); }
    | AT factor_unary               { $$ = create_unaryop_node(OP_DEREF, $2); }
    ;

compound_assignment_statement
    : identifier PLUS_ASSIGN expression      { 
                                               ASTNode* left = create_identifier_node_with_yyltype($1->data.identifier.name, (YYLTYPE*) &@$);
                                               ASTNode* binop = create_binop_node_with_yyltype(OP_ADD, left, $3, (YYLTYPE*) &@$);
                                               $$ = create_assign_node_with_yyltype($1, binop, (YYLTYPE*) &@$);
                                             }
    | identifier MINUS_ASSIGN expression     { 
                                               ASTNode* left = create_identifier_node_with_yyltype($1->data.identifier.name, (YYLTYPE*) &@$);
                                               ASTNode* binop = create_binop_node_with_yyltype(OP_SUB, left, $3, (YYLTYPE*) &@$);
                                               $$ = create_assign_node_with_yyltype($1, binop, (YYLTYPE*) &@$);
                                             }
    | identifier MULTIPLY_ASSIGN expression  { 
                                               ASTNode* left = create_identifier_node_with_yyltype($1->data.identifier.name, (YYLTYPE*) &@$);
                                               ASTNode* binop = create_binop_node_with_yyltype(OP_MUL, left, $3, (YYLTYPE*) &@$);
                                               $$ = create_assign_node_with_yyltype($1, binop, (YYLTYPE*) &@$);
                                             }
    | identifier DIVIDE_ASSIGN expression    { 
                                               ASTNode* left = create_identifier_node_with_yyltype($1->data.identifier.name, (YYLTYPE*) &@$);
                                               ASTNode* binop = create_binop_node_with_yyltype(OP_DIV, left, $3, (YYLTYPE*) &@$);
                                               $$ = create_assign_node_with_yyltype($1, binop, (YYLTYPE*) &@$);
                                             }
    | identifier MODULO_ASSIGN expression    { 
                                               ASTNode* left = create_identifier_node_with_yyltype($1->data.identifier.name, (YYLTYPE*) &@$);
                                               ASTNode* binop = create_binop_node_with_yyltype(OP_MOD, left, $3, (YYLTYPE*) &@$);
                                               $$ = create_assign_node_with_yyltype($1, binop, (YYLTYPE*) &@$);
                                             }
    ;

input_statement
    : identifier ASSIGN input_expression { 
        $$ = create_assign_node_with_yyltype($1, $3, (YYLTYPE*) &@$); 
    }
    ;

block_statement
    : LBRACE statement_list RBRACE { $$ = $2; }
    | statement                    { $$ = $1; }
    ;

if_statement
    : IF LPAREN expression RPAREN block_statement if_rest {
        /* $6 为 else/body（可能为 NULL），if_rest 构造最终 else_body */
        if ($6) {
            $$ = create_if_node_with_yyltype($3, $5, $6, (YYLTYPE*) &@$);
        } else {
            $$ = create_if_node_with_yyltype($3, $5, NULL, (YYLTYPE*) &@$);
        }
    }
    ;

if_rest
    : /* empty */ { $$ = NULL; }
    | ELSE block_statement { $$ = $2; }
    | ELIF LPAREN expression RPAREN block_statement if_rest {
        /* 将 ELIF 转换为 else: if(expr) then block else rest */
        ASTNode* nested_if = create_if_node($3, $5, $6);
        $$ = nested_if;
    }
    ;

while_statement
    : WHILE LPAREN expression RPAREN block_statement {
        $$ = create_while_node_with_yyltype($3, $5, (YYLTYPE*) &@$);
    }
    ;

for_statement
    : FOR LPAREN identifier SEMICOLON expression DOTDOT expression RPAREN block_statement {
        ASTNode* start = $5;
        ASTNode* end = $7;
        $$ = create_for_node_with_yyltype($3, start, end, $9, (YYLTYPE*) &@$);
    }
    | FOR LPAREN identifier IN expression DOTDOT expression RPAREN block_statement {
        ASTNode* start = $5;
        ASTNode* end = $7;
        $$ = create_for_node_with_yyltype($3, start, end, $9, (YYLTYPE*) &@$);
    }
    | FOR LPAREN identifier SEMICOLON expression RPAREN block_statement {
        ASTNode* var = $3;
        ASTNode* iterable = $5;
        $$ = create_for_node_with_yyltype(var, iterable, NULL, $7, (YYLTYPE*) &@$);
    }
    | FOR LPAREN IDENTIFIER identifier SEMICOLON expression RPAREN block_statement {
        ASTNode* var = $4;
        ASTNode* iterable = $6;
        $$ = create_for_node_with_yyltype(var, iterable, NULL, $8, (YYLTYPE*) &@$);
    }
    ;

expression_list
    : expression                    { 
                                      ASTNode* list = create_expression_list_node_with_yyltype((YYLTYPE*) &@$);
                                      add_expression_to_list(list, $1);
                                      $$ = list;
                                    }
    | expression COMMA expression   { 
                                      ASTNode* list = create_expression_list_node_with_yyltype((YYLTYPE*) &@$);
                                      add_expression_to_list(list, $1);
                                      add_expression_to_list(list, $3);
                                      $$ = list;
                                    }
    | expression_list COMMA expression { 
                                      add_expression_to_list($1, $3);
                                      $$ = $1;
                                    }
    ;

logical_expression
    : comparison_expression                 { $$ = $1; }
    | logical_expression AND comparison_expression { $$ = create_binop_node_with_yyltype(OP_AND, $1, $3, (YYLTYPE*) &@$); }
    | logical_expression OR comparison_expression  { $$ = create_binop_node_with_yyltype(OP_OR, $1, $3, (YYLTYPE*) &@$); }
    ;

comparison_expression
    : term                                  { $$ = $1; }
    | term EQ term                          { $$ = create_binop_node_with_yyltype(OP_EQ, $1, $3, (YYLTYPE*) &@$); }
    | term NE term                          { $$ = create_binop_node_with_yyltype(OP_NE, $1, $3, (YYLTYPE*) &@$); }
    | term LT term                          { $$ = create_binop_node_with_yyltype(OP_LT, $1, $3, (YYLTYPE*) &@$); }
    | term LE term                          { $$ = create_binop_node_with_yyltype(OP_LE, $1, $3, (YYLTYPE*) &@$); }
    | term GT term                          { $$ = create_binop_node_with_yyltype(OP_GT, $1, $3, (YYLTYPE*) &@$); }
    | term GE term                          { $$ = create_binop_node_with_yyltype(OP_GE, $1, $3, (YYLTYPE*) &@$); }
    ;

expression
    : logical_expression                    { $$ = $1; }
    | expression PLUS logical_expression    { $$ = create_binop_node_with_yyltype(OP_ADD, $1, $3, (YYLTYPE*) &@$); }
    | expression MINUS logical_expression   { $$ = create_binop_node_with_yyltype(OP_SUB, $1, $3, (YYLTYPE*) &@$); }
    ;

term
    : factor                        { $$ = $1; }
    | term MULTIPLY factor          { $$ = create_binop_node_with_yyltype(OP_MUL, $1, $3, (YYLTYPE*) &@$); }
    | term DIVIDE factor            { $$ = create_binop_node_with_yyltype(OP_DIV, $1, $3, (YYLTYPE*) &@$); }
    | term MODULO factor            { $$ = create_binop_node_with_yyltype(OP_MOD, $1, $3, (YYLTYPE*) &@$); }
    ;

factor
    : power                         { $$ = $1; }
    ;

power
    : factor_unary                  { $$ = $1; }
    | factor_unary POWER factor     { $$ = create_binop_node_with_yyltype(OP_POW, $1, $3, (YYLTYPE*) &@$); }
    ;

factor_unary
    : IDENTIFIER LPAREN RPAREN      { 
        ASTNode* id = create_identifier_node_with_yyltype($1, (YYLTYPE*) &@$); 
        $$ = create_call_node_with_yyltype(id, NULL, (YYLTYPE*) &@$); 
    }
    | IDENTIFIER LPAREN expression_list RPAREN { 
        ASTNode* id = create_identifier_node_with_yyltype($1, (YYLTYPE*) &@$); 
        $$ = create_call_node_with_yyltype(id, $3, (YYLTYPE*) &@$); 
    }
    | IDENTIFIER LBRACE RBRACE { ASTNode* type_id = create_identifier_node_with_yyltype($1, (YYLTYPE*) &@$); ASTNode* list = create_expression_list_node_with_yyltype((YYLTYPE*) &@$); $$ = create_struct_literal_node_with_yyltype(type_id, list, (YYLTYPE*) &@$); }
    | IDENTIFIER LBRACE struct_init_fields RBRACE { ASTNode* type_id = create_identifier_node_with_yyltype($1, (YYLTYPE*) &@$); $$ = create_struct_literal_node_with_yyltype(type_id, $3, (YYLTYPE*) &@$); }
    | literal                       { $$ = $1; }
    | identifier                    { $$ = $1; }
    | toint_expression              { $$ = $1; }
    | tofloat_expression            { $$ = $1; }
    | input_expression              { $$ = $1; }
    | PLUS factor_unary             { $$ = create_unaryop_node(OP_PLUS, $2); }
    | MINUS factor_unary            { $$ = create_unaryop_node(OP_MINUS, $2); }
    | MULTIPLY factor_unary         { $$ = create_unaryop_node(OP_DEREF, $2); }
    | AMPERSAND factor_unary        { $$ = create_unaryop_node(OP_ADDRESS, $2); }
    | AT factor_unary               { $$ = create_unaryop_node(OP_DEREF, $2); }
    | LPAREN expression RPAREN      { $$ = $2; }
    | factor_unary DOT IDENTIFIER { $$ = create_member_access_node_with_yyltype($1, create_identifier_node_with_yyltype($3, (YYLTYPE*) &@$), (YYLTYPE*) &@$); }
    | factor_unary DOT IDENTIFIER LPAREN RPAREN { 
        ASTNode* method = create_identifier_node_with_yyltype($3, (YYLTYPE*) &@$);
        ASTNode* mem = create_member_access_node_with_yyltype($1, method, (YYLTYPE*) &@$);
        $$ = create_call_node_with_yyltype(mem, NULL, (YYLTYPE*) &@$);
    }
    | factor_unary DOT IDENTIFIER LPAREN expression_list RPAREN { 
        ASTNode* method = create_identifier_node_with_yyltype($3, (YYLTYPE*) &@$);
        ASTNode* mem = create_member_access_node_with_yyltype($1, method, (YYLTYPE*) &@$);
        $$ = create_call_node_with_yyltype(mem, $5, (YYLTYPE*) &@$);
    }
    | factor_unary LBRACKET expression RBRACKET { $$ = create_index_node_with_yyltype($1, $3, (YYLTYPE*) &@$); }
    ;

literal
    : NUMBER_INT                    { $$ = create_num_int_node_with_yyltype($1, (YYLTYPE*) &@$); }
    | NUMBER_FLOAT                  { $$ = create_num_float_node_with_yyltype($1, (YYLTYPE*) &@$); }
    | STRING                        { $$ = create_string_node_with_yyltype($1, (YYLTYPE*) &@$); }
    | CHAR_LITERAL                  { $$ = create_char_node_with_yyltype($1[0], (YYLTYPE*) &@$); }
    | NIL                           { $$ = create_nil_node_with_yyltype((YYLTYPE*) &@$); }
    | LBRACKET RBRACKET             { $$ = create_expression_list_node_with_yyltype((YYLTYPE*) &@$); }
    | LBRACKET expression_list RBRACKET { $$ = $2; }
    ;

identifier
    : IDENTIFIER                    { $$ = create_identifier_node_with_yyltype($1, (YYLTYPE*) &@$); }
    ;

%%

void yyerror(const char* s) {
    const char* filename = current_input_filename ? current_input_filename : "unknown";
    report_syntax_error_with_location(s, filename, yylineno);
}
