#include "../include/ast.h"
#ifdef HAVE_PARSER_TAB_H//tips : 别删，用来取消警告
#include "../parser/parser.tab.h"//tips : 这个头文件按编译顺序编译
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ASTNode* create_program_node_with_location(Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_PROGRAM;
    node->location = location;
    node->data.program.statements = NULL;
    node->data.program.statement_count = 0;
    return node;
}

ASTNode* create_program_node() {
    Location loc = {1, 1, 1, 1};
    return create_program_node_with_location(loc);
}

void add_statement_to_program(ASTNode* program, ASTNode* statement) {
    if (program->type != AST_PROGRAM) return;
    
    program->data.program.statement_count++;
    program->data.program.statements = realloc(
        program->data.program.statements,
        sizeof(ASTNode*) * program->data.program.statement_count
    );
    program->data.program.statements[program->data.program.statement_count - 1] = statement;
}

ASTNode* create_print_node_with_location(ASTNode* expr, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_PRINT;
    node->location = location;
    node->data.print.expr = expr;
    return node;
}

ASTNode* create_print_node(ASTNode* expr) {
    return create_print_node_with_location(expr, expr->location);
}

ASTNode* create_input_node_with_location(ASTNode* prompt, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_INPUT;
    node->location = location;
    node->data.input.prompt = prompt;
    return node;
}

ASTNode* create_input_node(ASTNode* prompt) {
    return create_input_node_with_location(prompt, prompt->location);
}

ASTNode* create_input_node_with_yyltype(ASTNode* prompt, void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_input_node_with_location(prompt, location);
}

ASTNode* create_toint_node_with_location(ASTNode* expr, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_TOINT;
    node->location = location;
    node->data.toint.expr = expr;
    return node;
}

ASTNode* create_toint_node(ASTNode* expr) {
    return create_toint_node_with_location(expr, expr->location);
}

ASTNode* create_tofloat_node_with_location(ASTNode* expr, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_TOFLOAT;
    node->location = location;
    node->data.tofloat.expr = expr;
    return node;
}

ASTNode* create_tofloat_node(ASTNode* expr) {
    return create_tofloat_node_with_location(expr, expr->location);
}

ASTNode* create_expression_list_node_with_location(Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_EXPRESSION_LIST;
    node->location = location;
    node->data.expression_list.expressions = NULL;
    node->data.expression_list.expression_count = 0;
    return node;
}

ASTNode* create_expression_list_node() {
    Location loc = {1, 1, 1, 1};
    return create_expression_list_node_with_location(loc);
}

void add_expression_to_list(ASTNode* list, ASTNode* expr) {
    if (!list || list->type != AST_EXPRESSION_LIST || !expr) return;
    
    list->data.expression_list.expression_count++;
    list->data.expression_list.expressions = realloc(
        list->data.expression_list.expressions,
        sizeof(ASTNode*) * list->data.expression_list.expression_count
    );
    list->data.expression_list.expressions[list->data.expression_list.expression_count - 1] = expr;
}

ASTNode* create_assign_node_with_location(ASTNode* left, ASTNode* right, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_ASSIGN;
    node->location = location;
    node->data.assign.left = left;
    node->data.assign.right = right;
    return node;
}

ASTNode* create_const_node_with_location(ASTNode* left, ASTNode* right, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_CONST;
    node->location = location;
    node->data.assign.left = left;
    node->data.assign.right = right;
    return node;
}

ASTNode* create_const_node(ASTNode* left, ASTNode* right) {
    Location loc = {
        left->location.first_line,
        left->location.first_column,
        right->location.last_line,
        right->location.last_column
    };
    return create_const_node_with_location(left, right, loc);
}

ASTNode* create_assign_node(ASTNode* left, ASTNode* right) {
    Location loc = {
        left->location.first_line,
        left->location.first_column,
        right->location.last_line,
        right->location.last_column
    };
    return create_assign_node_with_location(left, right, loc);
}

ASTNode* create_binop_node_with_location(BinOpType op, ASTNode* left, ASTNode* right, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_BINOP;
    node->location = location;
    node->data.binop.op = op;
    node->data.binop.left = left;
    node->data.binop.right = right;
    return node;
}

ASTNode* create_binop_node(BinOpType op, ASTNode* left, ASTNode* right) {
    Location loc = {
        left->location.first_line,
        left->location.first_column,
        right->location.last_line,
        right->location.last_column
    };
    return create_binop_node_with_location(op, left, right, loc);
}

ASTNode* create_unaryop_node_with_location(UnaryOpType op, ASTNode* expr, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_UNARYOP;
    node->location = location;
    node->data.unaryop.op = op;
    node->data.unaryop.expr = expr;
    return node;
}

ASTNode* create_unaryop_node(UnaryOpType op, ASTNode* expr) {
    return create_unaryop_node_with_location(op, expr, expr->location);
}

ASTNode* create_num_int_node_with_location(long long value, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NUM_INT;
    node->location = location;
    node->data.num_int.value = value;
    return node;
}

ASTNode* create_num_int_node(long long value) {
    Location loc = {1, 1, 1, 1};
    return create_num_int_node_with_location(value, loc);
}

ASTNode* create_num_float_node_with_location(double value, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NUM_FLOAT;
    node->location = location;
    node->data.num_float.value = value;
    return node;
}

ASTNode* create_num_float_node(double value) {
    Location loc = {1, 1, 1, 1};
    return create_num_float_node_with_location(value, loc);
}

ASTNode* create_string_node_with_location(const char* value, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_STRING;
    node->location = location;
    node->data.string.value = malloc(strlen(value) + 1);
    strcpy(node->data.string.value, value);
    return node;
}

ASTNode* create_string_node(const char* value) {
    Location loc = {1, 1, 1, 1};
    return create_string_node_with_location(value, loc);
}

ASTNode* create_identifier_node_with_location(const char* name, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_IDENTIFIER;
    node->location = location;
    node->data.identifier.name = malloc(strlen(name) + 1);
    strcpy(node->data.identifier.name, name);
    return node;
}

ASTNode* create_identifier_node(const char* name) {
    Location loc = {1, 1, 1, 1};
    return create_identifier_node_with_location(name, loc);
}

ASTNode* create_type_node_with_location(NodeType type, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = type;
    node->location = location;
    return node;
}

ASTNode* create_type_node(NodeType type) {
    Location loc = {1, 1, 1, 1};
    return create_type_node_with_location(type, loc);
}

ASTNode* create_if_node_with_location(ASTNode* condition, ASTNode* then_body, ASTNode* else_body, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_IF;
    node->location = location;
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.then_body = then_body;
    node->data.if_stmt.else_body = else_body;
    return node;
}

ASTNode* create_if_node(ASTNode* condition, ASTNode* then_body, ASTNode* else_body) {
    Location loc = {
        condition->location.first_line,
        condition->location.first_column,
        else_body ? else_body->location.last_line : then_body->location.last_line,
        else_body ? else_body->location.last_column : then_body->location.last_column
    };
    return create_if_node_with_location(condition, then_body, else_body, loc);
}

ASTNode* create_while_node_with_location(ASTNode* condition, ASTNode* body, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_WHILE;
    node->location = location;
    node->data.while_stmt.condition = condition;
    node->data.while_stmt.body = body;
    return node;
}

ASTNode* create_while_node(ASTNode* condition, ASTNode* body) {
    Location loc = {
        condition->location.first_line,
        condition->location.first_column,
        body->location.last_line,
        body->location.last_column
    };
    return create_while_node_with_location(condition, body, loc);
}

ASTNode* create_for_node_with_location(ASTNode* var, ASTNode* start, ASTNode* end, ASTNode* body, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_FOR;
    node->location = location;
    node->data.for_stmt.var = var;
    node->data.for_stmt.start = start;
    node->data.for_stmt.end = end;
    node->data.for_stmt.body = body;
    return node;
}

ASTNode* create_for_node(ASTNode* var, ASTNode* start, ASTNode* end, ASTNode* body) {
    Location loc = {
        var->location.first_line,
        var->location.first_column,
        body->location.last_line,
        body->location.last_column
    };
    return create_for_node_with_location(var, start, end, body, loc);
}

ASTNode* create_function_node_with_location(const char* name, ASTNode* params, ASTNode* return_type, ASTNode* body, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_FUNCTION;
    node->location = location;
    node->data.function.name = malloc(strlen(name) + 1);
    strcpy(node->data.function.name, name);
    node->data.function.params = params;
    node->data.function.return_type = return_type;
    node->data.function.body = body;
    return node;
}

ASTNode* create_function_node(const char* name, ASTNode* params, ASTNode* return_type, ASTNode* body) {
    Location loc = {0, 0, 0, 0};
    if (body) {
        loc.first_line = 0;
        loc.first_column = 0;
        loc.last_line = body->location.last_line;
        loc.last_column = body->location.last_column;
    }
    return create_function_node_with_location(name, params, return_type, body, loc);
}

ASTNode* create_break_node_with_location(Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_BREAK;
    node->location = location;
    return node;
}

ASTNode* create_break_node() {
    Location loc = {1, 1, 1, 1};
    return create_break_node_with_location(loc);
}

ASTNode* create_continue_node_with_location(Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_CONTINUE;
    node->location = location;
    return node;
}

ASTNode* create_continue_node() {
    Location loc = {1, 1, 1, 1};
    return create_continue_node_with_location(loc);
}

ASTNode* create_return_node_with_location(ASTNode* expr, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_RETURN;
    node->location = location;
    node->data.return_stmt.expr = expr;
    return node;
}

ASTNode* create_return_node(ASTNode* expr) {
    if (expr) {
        return create_return_node_with_location(expr, expr->location);
    } else {
        Location loc = {1, 1, 1, 1};
        return create_return_node_with_location(expr, loc);
    }
}

ASTNode* create_call_node(ASTNode* func, ASTNode* args) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_CALL;
    node->location = func->location;// 使用函数的位置
    node->data.call.func = func;
    node->data.call.args = args;
    return node;
}
ASTNode* create_call_node_with_location(ASTNode* func, ASTNode* args, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_CALL;
    node->location = location;
    node->data.call.func = func;
    node->data.call.args = args;
    return node;
}
ASTNode* create_call_node_with_yyltype(ASTNode* func, ASTNode* args, void* yylloc) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_CALL;
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    node->location.first_line = loc->first_line;
    node->location.first_column = loc->first_column;
    node->location.last_line = loc->last_line;
    node->location.last_column = loc->last_column;
    node->data.call.func = func;
    node->data.call.args = args;
    return node;
}
ASTNode* create_index_node_with_location(ASTNode* target, ASTNode* index, Location location) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_INDEX;
    node->location = location;
    node->data.index.target = target;
    node->data.index.index = index;
    return node;
}

ASTNode* create_index_node(ASTNode* target, ASTNode* index) {
    Location loc = {
        target->location.first_line,
        target->location.first_column,
        index->location.last_line,
        index->location.last_column
    };
    return create_index_node_with_location(target, index, loc);
}

ASTNode* create_index_node_with_yyltype(ASTNode* target, ASTNode* index, void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_index_node_with_location(target, index, location);
}
ASTNode* create_toint_node_with_yyltype(ASTNode* expr, void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_toint_node_with_location(expr, location);
}

ASTNode* create_tofloat_node_with_yyltype(ASTNode* expr, void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_tofloat_node_with_location(expr, location);
}

ASTNode* create_program_node_with_yyltype(void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_program_node_with_location(location);
}

ASTNode* create_return_node_with_yyltype(ASTNode* expr, void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_return_node_with_location(expr, location);
}

ASTNode* create_break_node_with_yyltype(void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_break_node_with_location(location);
}

ASTNode* create_continue_node_with_yyltype(void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_continue_node_with_location(location);
}

ASTNode* create_print_node_with_yyltype(ASTNode* expr, void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_print_node_with_location(expr, location);
}

ASTNode* create_assign_node_with_yyltype(ASTNode* left, ASTNode* right, void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_assign_node_with_location(left, right, location);
}

ASTNode* create_const_node_with_yyltype(ASTNode* left, ASTNode* right, void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_const_node_with_location(left, right, location);
}

ASTNode* create_binop_node_with_yyltype(BinOpType op, ASTNode* left, ASTNode* right, void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_binop_node_with_location(op, left, right, location);
}

ASTNode* create_if_node_with_yyltype(ASTNode* condition, ASTNode* then_body, ASTNode* else_body, void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_if_node_with_location(condition, then_body, else_body, location);
}

ASTNode* create_while_node_with_yyltype(ASTNode* condition, ASTNode* body, void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_while_node_with_location(condition, body, location);
}

ASTNode* create_for_node_with_yyltype(ASTNode* var, ASTNode* start, ASTNode* end, ASTNode* body, void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_for_node_with_location(var, start, end, body, location);
}

ASTNode* create_expression_list_node_with_yyltype(void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_expression_list_node_with_location(location);
}

ASTNode* create_num_int_node_with_yyltype(long long value, void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_num_int_node_with_location(value, location);
}

ASTNode* create_num_float_node_with_yyltype(double value, void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_num_float_node_with_location(value, location);
}

ASTNode* create_string_node_with_yyltype(const char* value, void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_string_node_with_location(value, location);
}

ASTNode* create_identifier_node_with_yyltype(const char* name, void* yylloc) {
    YYLTYPE* loc = (YYLTYPE*)yylloc;
    Location location = {
        loc->first_line,
        loc->first_column,
        loc->last_line,
        loc->last_column
    };
    return create_identifier_node_with_location(name, location);
}

void free_ast(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->data.program.statement_count; i++) {
                free_ast(node->data.program.statements[i]);
            }
            free(node->data.program.statements);
            break;
            
        case AST_PRINT:
            free_ast(node->data.print.expr);
            break;
            
        case AST_INPUT:
            free_ast(node->data.input.prompt);
            break;
            
        case AST_TOINT:
            free_ast(node->data.toint.expr);
            break;
            
        case AST_TOFLOAT:
            free_ast(node->data.tofloat.expr);
            break;
            
        case AST_EXPRESSION_LIST:
            for (int i = 0; i < node->data.expression_list.expression_count; i++) {
                free_ast(node->data.expression_list.expressions[i]);
            }
            free(node->data.expression_list.expressions);
            break;
            
        case AST_ASSIGN:
            free_ast(node->data.assign.left);
            free_ast(node->data.assign.right);
            break;
        case AST_CONST:
            free_ast(node->data.assign.left);
            free_ast(node->data.assign.right);
            break;
            
        case AST_BINOP:
            free_ast(node->data.binop.left);
            free_ast(node->data.binop.right);
            break;
            
        case AST_UNARYOP:
            free_ast(node->data.unaryop.expr);
            break;
            
        case AST_STRING:
            free(node->data.string.value);
            break;
            
        case AST_IDENTIFIER:
            free(node->data.identifier.name);
            break;
            
        case AST_TYPE_INT32:
        case AST_TYPE_INT64:
        case AST_TYPE_FLOAT32:
        case AST_TYPE_FLOAT64:
        case AST_TYPE_STRING:
        case AST_TYPE_VOID:
            break;
            
        case AST_IF:
            free_ast(node->data.if_stmt.condition);
            free_ast(node->data.if_stmt.then_body);
            if (node->data.if_stmt.else_body) {
                free_ast(node->data.if_stmt.else_body);
            }
            break;
            
        case AST_WHILE:
            free_ast(node->data.while_stmt.condition);
            free_ast(node->data.while_stmt.body);
            break;
            
        case AST_FOR:
            free_ast(node->data.for_stmt.var);
            free_ast(node->data.for_stmt.start);
            free_ast(node->data.for_stmt.end);
            free_ast(node->data.for_stmt.body);
            break;
        case AST_INDEX:
            free_ast(node->data.index.target);
            free_ast(node->data.index.index);
            break;
            
        case AST_FUNCTION:
            free(node->data.function.name);
            if (node->data.function.params) {
                free_ast(node->data.function.params);
            }
            if (node->data.function.return_type) {
                free_ast(node->data.function.return_type);
            }
            if (node->data.function.body) {
                free_ast(node->data.function.body);
            }
            break;
            
        case AST_CALL:
            free_ast(node->data.call.func);
            if (node->data.call.args) {
                free_ast(node->data.call.args);
            }
            break;
            
        case AST_RETURN:
            if (node->data.return_stmt.expr) {
                free_ast(node->data.return_stmt.expr);
            }
            break;

            
        case AST_NUM_INT:
        case AST_NUM_FLOAT:
        case AST_BREAK:
        case AST_CONTINUE:
            break;
    }
    
    free(node);
}

void print_ast(ASTNode* node, int indent) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) printf("  ");
    
    switch (node->type) {
        case AST_PROGRAM:
            printf("Program:\n");
            for (int i = 0; i < node->data.program.statement_count; i++) {
                print_ast(node->data.program.statements[i], indent + 1);
            }
            break;
        case AST_PRINT:
            printf("Print:\n");
            print_ast(node->data.print.expr, indent + 1);
            break;
        case AST_ASSIGN:
            printf("Assign:\n");
            print_ast(node->data.assign.left, indent + 1);
            print_ast(node->data.assign.right, indent + 1);
            break;
        case AST_CONST:
            printf("Const:\n");
            print_ast(node->data.assign.left, indent + 1);
            print_ast(node->data.assign.right, indent + 1);
            break;
        case AST_IF:
            printf("If:\n");
            print_ast(node->data.if_stmt.condition, indent + 1);
            print_ast(node->data.if_stmt.then_body, indent + 1);
            if (node->data.if_stmt.else_body) {
                for (int i = 0; i < indent; i++) printf("  ");
                printf("Else:\n");
                print_ast(node->data.if_stmt.else_body, indent + 1);
            }
            break;
        case AST_WHILE:
            printf("While:\n");
            print_ast(node->data.while_stmt.condition, indent + 1);
            print_ast(node->data.while_stmt.body, indent + 1);
            break;
        case AST_FOR:
            printf("For:\n");
            print_ast(node->data.for_stmt.var, indent + 1);
            print_ast(node->data.for_stmt.start, indent + 1);
            print_ast(node->data.for_stmt.end, indent + 1);
            print_ast(node->data.for_stmt.body, indent + 1);
            break;
        case AST_INDEX:
            printf("Index:\n");
            print_ast(node->data.index.target, indent + 1);
            print_ast(node->data.index.index, indent + 1);
            break;
        case AST_BREAK:
            printf("Break\n");
            break;
        case AST_CONTINUE:
            printf("Continue\n");
            break;
        case AST_BINOP:
            printf("BinaryOp: ");
            switch (node->data.binop.op) {
                case OP_ADD: printf("+\n"); break;
                case OP_SUB: printf("-\n"); break;
                case OP_MUL: printf("*\n"); break;
                case OP_DIV: printf("/\n"); break;
                case OP_MOD: printf("%%\n"); break;
                case OP_POW: printf("**\n"); break;
                case OP_CONCAT: printf("concat+\n"); break;
                case OP_REPEAT: printf("repeat*\n"); break;
                case OP_EQ: printf("==\n"); break;
                case OP_NE: printf("!=\n"); break;
                case OP_LT: printf("<\n"); break;
                case OP_LE: printf("<=\n"); break;
                case OP_GT: printf(">\n"); break;
                case OP_GE: printf(">=\n"); break;
            }
            print_ast(node->data.binop.left, indent + 1);
            print_ast(node->data.binop.right, indent + 1);
            break;
        case AST_UNARYOP:
            printf("UnaryOp: ");
            switch (node->data.unaryop.op) {
                case OP_MINUS: printf("-\n"); break;
                case OP_PLUS: printf("+\n"); break;
            }
            print_ast(node->data.unaryop.expr, indent + 1);
            break;
        case AST_NUM_INT:
            printf("Integer: %lld\n", node->data.num_int.value);
            break;
        case AST_NUM_FLOAT:
            printf("Float: %f\n", node->data.num_float.value);
            break;
        case AST_STRING:
            printf("String: \"%s\"\n", node->data.string.value);
            break;
        case AST_IDENTIFIER:
            printf("Identifier: %s\n", node->data.identifier.name);
            break;
        case AST_INPUT:
            printf("Input:\n");
            print_ast(node->data.input.prompt, indent + 1);
            break;
        case AST_TOINT:
            printf("ToInt:\n");
            print_ast(node->data.toint.expr, indent + 1);
            break;
        case AST_TOFLOAT:
            printf("ToFloat:\n");
            print_ast(node->data.tofloat.expr, indent + 1);
            break;
        case AST_EXPRESSION_LIST:
            printf("ExpressionList:\n");
            for (int i = 0; i < node->data.expression_list.expression_count; i++) {
                print_ast(node->data.expression_list.expressions[i], indent + 1);
            }
            break;
        case AST_TYPE_INT32:
            printf("Type: int32\n");
            break;
        case AST_TYPE_INT64:
            printf("Type: int64\n");
            break;
        case AST_TYPE_FLOAT32:
            printf("Type: float32\n");
            break;
        case AST_TYPE_FLOAT64:
            printf("Type: float64\n");
            break;
        case AST_TYPE_STRING:
            printf("Type: string\n");
            break;
        case AST_TYPE_VOID:
            printf("Type: void\n");
            break;
        case AST_FUNCTION:
            printf("Function: %s\n", node->data.function.name);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Params:\n");
            if (node->data.function.params) {
                print_ast(node->data.function.params, indent + 2);
            }
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Return Type:\n");
            if (node->data.function.return_type) print_ast(node->data.function.return_type, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Body:\n");
            if (node->data.function.body) print_ast(node->data.function.body, indent + 2);
            break;
        case AST_CALL:
            printf("Call:\n");
            if (node->data.call.func) print_ast(node->data.call.func, indent + 1);
            if (node->data.call.args) print_ast(node->data.call.args, indent + 1);
            break;
        case AST_RETURN:
            printf("Return:\n");
            if (node->data.return_stmt.expr) print_ast(node->data.return_stmt.expr, indent + 1);
            break;
        default:
            printf("Unknown node type!!!: %d\n", node->type);
            break;
    }
}