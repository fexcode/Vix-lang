#include "../include/semantic.h"
#include "../include/compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern const char* current_input_filename;
SymbolTable* create_symbol_table(SymbolTable* parent) {
    SymbolTable* table = malloc(sizeof(SymbolTable));
    if (!table) return NULL;
    table->head = NULL;
    table->parent = parent;
    return table;
}

static Symbol* create_symbol(const char* name, SymbolType type, InferredType inferred_type) {
    Symbol* sym = malloc(sizeof(Symbol));
    if (!sym) return NULL;
    
    sym->name = malloc(strlen(name) + 1);
    if (!sym->name) {
        free(sym);
        return NULL;
    }
    strcpy(sym->name, name);
    
    sym->type = type;
    sym->inferred_type = inferred_type;
    sym->next = NULL;
    
    return sym;
}

int add_symbol(SymbolTable* table, const char* name, SymbolType type, InferredType inferred_type) {
    if (!table || !name) return 0;
    
    Symbol* sym = create_symbol(name, type, inferred_type);
    if (!sym) return 0;
    
    sym->next = table->head;
    table->head = sym;
    
    return 1;
}

Symbol* lookup_symbol(SymbolTable* table, const char* name) {
    if (!table || !name) return NULL;
    
    Symbol* current = table->head;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    if (table->parent) {
        return lookup_symbol(table->parent, name);
    }
    
    return NULL;
}

void destroy_symbol_table(SymbolTable* symbol_table) {
    if (!symbol_table) return;
    
    Symbol* current = symbol_table->head;
    while (current) {
        Symbol* next = current->next;
        free(current->name);
        free(current);
        current = next;
    }
    
    free(symbol_table);
}
typedef struct VisitedNode {
    ASTNode* node;
    struct VisitedNode* next;
} VisitedNode;

static int is_node_visited(ASTNode* node, VisitedNode* visited_list) {
    VisitedNode* current = visited_list;
    while (current) {
        if (current->node == node) {
            return 1; 
        }
        current = current->next;
    }
    return 0;
}

static VisitedNode* add_visited_node(ASTNode* node, VisitedNode* visited_list) {
    VisitedNode* new_visited = malloc(sizeof(VisitedNode));
    if (!new_visited) return visited_list;
    new_visited->node = node;
    new_visited->next = visited_list;
    return new_visited;
}

static VisitedNode* remove_visited_node(ASTNode* node, VisitedNode* visited_list) {
    if (!visited_list) return NULL;
    
    if (visited_list->node == node) {
        VisitedNode* next = visited_list->next;
        free(visited_list);
        return next;
    }
    return visited_list;
}

static int check_undefined_symbols_in_node_with_visited(ASTNode* node, SymbolTable* table, VisitedNode* visited_list);

int check_undefined_symbols(ASTNode* node) {
    SymbolTable* global_table = create_symbol_table(NULL);
    if (!global_table) return 1;
    int result = check_undefined_symbols_in_node_with_visited(node, global_table, NULL);
    destroy_symbol_table(global_table);
    return result;
}

static int check_undefined_symbols_in_node_with_visited(ASTNode* node, SymbolTable* table, VisitedNode* visited_list) {
    if (!node) return 0;
    if (is_node_visited(node, visited_list)) {
        return 0;
    }
    VisitedNode* new_visited_list = add_visited_node(node, visited_list);
    if (!new_visited_list) {
        /*如果无法添加到访问列表，可能是因为内存分配失败
            在这种情况下，我们仍然可以继续处理，但不进行递归保护
            但为了安全，我们返回0避免进一步处理
        */
        return 0;
    }
    
    int errors_found = 0;
    
    switch (node->type) {
        case AST_PROGRAM: {
            for (int i = 0; i < node->data.program.statement_count; i++) {
                errors_found += check_undefined_symbols_in_node_with_visited(node->data.program.statements[i], table, new_visited_list);
            }
            break;
        }
        
        case AST_ASSIGN: {
            errors_found += check_undefined_symbols_in_node_with_visited(node->data.assign.right, table, new_visited_list);
            if (node->data.assign.left && node->data.assign.left->type == AST_IDENTIFIER) {
                Symbol* existing = lookup_symbol(table, node->data.assign.left->data.identifier.name);
                if (existing && existing->type == SYMBOL_CONSTANT) {
                    const char* filename = current_input_filename ? current_input_filename : "unknown";
                    int line = (node->data.assign.left->location.first_line > 0) ? node->data.assign.left->location.first_line : 1;
                    char buf[256];
                    snprintf(buf, sizeof(buf), "cannot assign to constant: '%s'", node->data.assign.left->data.identifier.name);
                    report_semantic_error_with_location(buf, filename, line);
                    errors_found++;
                } else {
                    add_symbol(table, node->data.assign.left->data.identifier.name, SYMBOL_VARIABLE, TYPE_UNKNOWN);
                }
            }
            break;
        }

        case AST_CONST: {
            errors_found += check_undefined_symbols_in_node_with_visited(node->data.assign.right, table, new_visited_list);
            if (node->data.assign.left && node->data.assign.left->type == AST_IDENTIFIER) {
                Symbol* existing = lookup_symbol(table, node->data.assign.left->data.identifier.name);
                if (existing) {
                    const char* filename = current_input_filename ? current_input_filename : "unknown";
                    int line = (node->data.assign.left->location.first_line > 0) ? node->data.assign.left->location.first_line : 1;
                    report_redefinition_error_with_location(node->data.assign.left->data.identifier.name, filename, line);
                    errors_found++;
                } else {
                    add_symbol(table, node->data.assign.left->data.identifier.name, SYMBOL_CONSTANT, TYPE_UNKNOWN);
                }
            }
            break;
        }
        
        case AST_IDENTIFIER: {
            Symbol* sym = lookup_symbol(table, node->data.identifier.name);
            if (!sym) {
                const char* filename = current_input_filename ? current_input_filename : "unknown";
                int line = (node->location.first_line > 0) ? node->location.first_line : 1;
                int column = (node->location.first_column > 0) ? node->location.first_column : 1;
                report_undefined_identifier_with_location_and_column(
                    node->data.identifier.name, 
                    filename, 
                    line,
                    column);
                errors_found++;
            }
            break;
        }
        
        case AST_FUNCTION: {
            add_symbol(table, node->data.function.name, SYMBOL_FUNCTION, TYPE_UNKNOWN);
            SymbolTable* func_scope = create_symbol_table(table);
            if (node->data.function.params && node->data.function.params->type == AST_EXPRESSION_LIST) {
                for (int i = 0; i < node->data.function.params->data.expression_list.expression_count; i++) {
                    ASTNode* param = node->data.function.params->data.expression_list.expressions[i];
                    if (param->type == AST_IDENTIFIER) {
                        add_symbol(func_scope, param->data.identifier.name, SYMBOL_VARIABLE, TYPE_UNKNOWN);
                    }
                }
            }
            if (node->data.function.body) {
                errors_found += check_undefined_symbols_in_node_with_visited(node->data.function.body, func_scope, new_visited_list);
            }
            destroy_symbol_table(func_scope);
            break;
        }
        
        case AST_CALL: {
            if (node->data.call.func && node->data.call.func->type == AST_IDENTIFIER) {
                Symbol* sym = lookup_symbol(table, node->data.call.func->data.identifier.name);
                if (!sym) {
                    const char* filename = current_input_filename ? current_input_filename : "unknown";
                    int line = (node->data.call.func->location.first_line > 0) ? node->data.call.func->location.first_line : 1;
                    int column = (node->data.call.func->location.first_column > 0) ? node->data.call.func->location.first_column : 1;
                    report_undefined_function_with_location_and_column(
                        node->data.call.func->data.identifier.name, 
                        filename, 
                        line,
                        column);
                    errors_found++;
                }
            }
            if (node->data.call.args) {
                errors_found += check_undefined_symbols_in_node_with_visited(node->data.call.args, table, new_visited_list);
            }
            break;
        }
        case AST_INDEX: {
            errors_found += check_undefined_symbols_in_node_with_visited(node->data.index.target, table, new_visited_list);
            errors_found += check_undefined_symbols_in_node_with_visited(node->data.index.index, table, new_visited_list);
            break;
        }
        
        case AST_BINOP:
        case AST_UNARYOP: {
            if (node->type == AST_BINOP) {
                errors_found += check_undefined_symbols_in_node_with_visited(node->data.binop.left, table, new_visited_list);
                errors_found += check_undefined_symbols_in_node_with_visited(node->data.binop.right, table, new_visited_list);
            } else {
                errors_found += check_undefined_symbols_in_node_with_visited(node->data.unaryop.expr, table, new_visited_list);
            }
            break;
        }
        
        
        case AST_IF: {
            errors_found += check_undefined_symbols_in_node_with_visited(node->data.if_stmt.condition, table, new_visited_list);
            errors_found += check_undefined_symbols_in_node_with_visited(node->data.if_stmt.then_body, table, new_visited_list);
            if (node->data.if_stmt.else_body) {
                errors_found += check_undefined_symbols_in_node_with_visited(node->data.if_stmt.else_body, table, new_visited_list);
            }
            break;
        }
        
        case AST_WHILE: {
            errors_found += check_undefined_symbols_in_node_with_visited(node->data.while_stmt.condition, table, new_visited_list);
            errors_found += check_undefined_symbols_in_node_with_visited(node->data.while_stmt.body, table, new_visited_list);
            break;
        }
        
        case AST_FOR: {
            errors_found += check_undefined_symbols_in_node_with_visited(node->data.for_stmt.start, table, new_visited_list);
            errors_found += check_undefined_symbols_in_node_with_visited(node->data.for_stmt.end, table, new_visited_list);
            if (node->data.for_stmt.var && node->data.for_stmt.var->type == AST_IDENTIFIER) {
                add_symbol(table, node->data.for_stmt.var->data.identifier.name, SYMBOL_VARIABLE, TYPE_UNKNOWN);
            }
            errors_found += check_undefined_symbols_in_node_with_visited(node->data.for_stmt.body, table, new_visited_list);
            break;
        }
        
        case AST_PRINT: {
            errors_found += check_undefined_symbols_in_node_with_visited(node->data.print.expr, table, new_visited_list);
            break;
        }
        
        case AST_INPUT: {
            if (node->data.input.prompt) {
                errors_found += check_undefined_symbols_in_node_with_visited(node->data.input.prompt, table, new_visited_list);
            }
            break;
        }
        
        case AST_TOINT:
        case AST_TOFLOAT: {
            if (node->type == AST_TOINT) {
                errors_found += check_undefined_symbols_in_node_with_visited(node->data.toint.expr, table, new_visited_list);
            } else {
                errors_found += check_undefined_symbols_in_node_with_visited(node->data.tofloat.expr, table, new_visited_list);
            }
            break;
        }
        
        case AST_RETURN: {
            if (node->data.return_stmt.expr) {
                errors_found += check_undefined_symbols_in_node_with_visited(node->data.return_stmt.expr, table, new_visited_list);
            }
            break;
        }
        
        case AST_EXPRESSION_LIST: {
            for (int i = 0; i < node->data.expression_list.expression_count; i++) {
                errors_found += check_undefined_symbols_in_node_with_visited(node->data.expression_list.expressions[i], table, new_visited_list);
            }
            break;
        }
        
        case AST_NUM_INT:
        case AST_NUM_FLOAT:
        case AST_STRING:
            break;
            
        default:
            break;
    }
    
    /*释放当前添加的访问标记节点，但不释放整个访问列表
    因为访问列表是由调用者管理的*/
    free(new_visited_list);
    
    return errors_found;
}
int is_variable_used_in_node(ASTNode* node, const char* var_name) {
    if (!node || !var_name) return 0;
    
    switch (node->type) {
        case AST_PROGRAM: {
            for (int i = 0; i < node->data.program.statement_count; i++) {
                if (is_variable_used_in_node(node->data.program.statements[i], var_name)) {
                    return 1;
                }
            }
            break;
        }
        
        case AST_ASSIGN: {
            if (is_variable_used_in_node(node->data.assign.right, var_name)) { //右边
                return 1;
            }
            
            if (node->data.assign.left && node->data.assign.left->type == AST_IDENTIFIER) {
                if (strcmp(node->data.assign.left->data.identifier.name, var_name) == 0) {
                    return 0; // 这是赋值 不是使用 如果左边是相同的变量名 则不是使用而是赋只
                }
            }
            break;
        }
        
        case AST_IDENTIFIER: {
            return strcmp(node->data.identifier.name, var_name) == 0;
        }
        
        case AST_CALL: {
            if (node->data.call.args) {
                if (is_variable_used_in_node(node->data.call.args, var_name)) {
                    return 1;
                }
            }
            break;
        }
        
        case AST_BINOP:
        case AST_UNARYOP: {
            if (node->type == AST_BINOP) {
                return is_variable_used_in_node(node->data.binop.left, var_name) ||is_variable_used_in_node(node->data.binop.right, var_name);
            } else {
                return is_variable_used_in_node(node->data.unaryop.expr, var_name);
            }
        }
        
        case AST_IF: {
            if (is_variable_used_in_node(node->data.if_stmt.condition, var_name)) {
                return 1;
            }
            if (is_variable_used_in_node(node->data.if_stmt.then_body, var_name)) {
                return 1;
            }
            if (node->data.if_stmt.else_body && 
                is_variable_used_in_node(node->data.if_stmt.else_body, var_name)) {
                return 1;
            }
            break;
        }
        
        case AST_WHILE: {
            if (is_variable_used_in_node(node->data.while_stmt.condition, var_name)) {
                return 1;
            }
            if (is_variable_used_in_node(node->data.while_stmt.body, var_name)) {
                return 1;
            }
            break;
        }
        
        case AST_FOR: {
            if (is_variable_used_in_node(node->data.for_stmt.start, var_name)) {
                return 1;
            }
            if (is_variable_used_in_node(node->data.for_stmt.end, var_name)) {
                return 1;
            }
            if (node->data.for_stmt.var && node->data.for_stmt.var->type == AST_IDENTIFIER) {
                if (strcmp(node->data.for_stmt.var->data.identifier.name, var_name) == 0) {
                    return 0; // 循环变量定义 不算使用
                }
            }
            if (is_variable_used_in_node(node->data.for_stmt.body, var_name)) {
                return 1;
            }
            break;
        }
        
        case AST_PRINT: {
            return is_variable_used_in_node(node->data.print.expr, var_name);
        }
        
        case AST_INPUT: {
            if (node->data.input.prompt) {
                return is_variable_used_in_node(node->data.input.prompt, var_name);
            }
            break;
        }
        
        case AST_TOINT:
        case AST_TOFLOAT: {
            if (node->type == AST_TOINT) {
                return is_variable_used_in_node(node->data.toint.expr, var_name);
            } else {
                return is_variable_used_in_node(node->data.tofloat.expr, var_name);
            }
        }
        
        case AST_RETURN: {
            if (node->data.return_stmt.expr) {
                return is_variable_used_in_node(node->data.return_stmt.expr, var_name);
            }
            break;
        }
        
        case AST_EXPRESSION_LIST: {
            for (int i = 0; i < node->data.expression_list.expression_count; i++) {
                if (is_variable_used_in_node(node->data.expression_list.expressions[i], var_name)) {
                    return 1;
                }
            }
            break;
        }
        case AST_INDEX: {
            return is_variable_used_in_node(node->data.index.target, var_name) || is_variable_used_in_node(node->data.index.index, var_name);
        }
        
        case AST_NUM_INT:
        case AST_NUM_FLOAT:
        case AST_STRING:
            break;
            
        default:
            break;
    }
    
    return 0;
}
typedef struct VariableUsage {
    char* name;
    int used;
    int line;
    int column;
    struct VariableUsage* next;
} VariableUsage;
VariableUsage* add_variable_to_usage_with_column(VariableUsage* list, const char* name, int line, int column) {
    VariableUsage* new_var = malloc(sizeof(VariableUsage));
    new_var->name = malloc(strlen(name) + 1);
    strcpy(new_var->name, name);
    new_var->used = 0;
    new_var->line = line;
    new_var->column = column;
    new_var->next = list;
    return new_var;
}

VariableUsage* add_variable_to_usage(VariableUsage* list, const char* name, int line) {
    return add_variable_to_usage_with_column(list, name, line, 1);
}
VariableUsage* find_variable_in_usage(VariableUsage* list, const char* name) {
    VariableUsage* current = list;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}
void free_variable_usage(VariableUsage* list) {
    while (list) {
        VariableUsage* temp = list;
        list = list->next;
        free(temp->name);
        free(temp);
    }
}
int check_unused_variables(ASTNode* node, SymbolTable* table) {
    VariableUsage* usage_list = NULL;
    int warnings_found = 0;
    warnings_found = check_unused_variables_with_usage(node, table, &usage_list);
    VariableUsage* current = usage_list;
    while (current) {
        if (!current->used) {
            const char* filename = current_input_filename ? current_input_filename : "unknown";
            report_unused_variable_warning_with_location(current->name, filename, current->line, current->column);
            warnings_found++;
        }
        current = current->next;
    }
    
    free_variable_usage(usage_list);
    return warnings_found;
}
int check_unused_variables_with_usage(ASTNode* node, SymbolTable* table, VariableUsage** usage_list) {
    if (!node) return 0;
    
    int warnings_found = 0;
    
    switch (node->type) {
        case AST_PROGRAM: {
            for (int i = 0; i < node->data.program.statement_count; i++) {
                warnings_found += check_unused_variables_with_usage(node->data.program.statements[i], table, usage_list);
            }
            break;
        }
        
        case AST_ASSIGN: {
            warnings_found += check_unused_variables_with_usage(node->data.assign.right, table, usage_list);
            if (node->data.assign.left && node->data.assign.left->type == AST_IDENTIFIER) {
                int line = (node->data.assign.left->location.first_line > 0) ? node->data.assign.left->location.first_line : 1;
                int column = (node->data.assign.left->location.first_column > 0) ? node->data.assign.left->location.first_column : 1;
                add_symbol(table, node->data.assign.left->data.identifier.name, SYMBOL_VARIABLE, TYPE_UNKNOWN);
                VariableUsage* var_usage = find_variable_in_usage(*usage_list, node->data.assign.left->data.identifier.name);
                if (!var_usage) {
                    VariableUsage* new_usage = add_variable_to_usage_with_column(*usage_list, node->data.assign.left->data.identifier.name, line, column);
                    *usage_list = new_usage;
                }
            }
            break;
        }
        case AST_CONST: {
            warnings_found += check_unused_variables_with_usage(node->data.assign.right, table, usage_list);
            if (node->data.assign.left && node->data.assign.left->type == AST_IDENTIFIER) {
                int line = (node->data.assign.left->location.first_line > 0) ? node->data.assign.left->location.first_line : 1;
                int column = (node->data.assign.left->location.first_column > 0) ? node->data.assign.left->location.first_column : 1;
                add_symbol(table, node->data.assign.left->data.identifier.name, SYMBOL_CONSTANT, TYPE_UNKNOWN);
                VariableUsage* var_usage = find_variable_in_usage(*usage_list, node->data.assign.left->data.identifier.name);
                if (!var_usage) {
                    VariableUsage* new_usage = add_variable_to_usage_with_column(*usage_list, node->data.assign.left->data.identifier.name, line, column);
                    *usage_list = new_usage;
                }
            }
            break;
        }
        
        case AST_IDENTIFIER: {
            Symbol* sym = lookup_symbol(table, node->data.identifier.name);
            if (sym) {
                VariableUsage* var_usage = find_variable_in_usage(*usage_list, node->data.identifier.name);
                if (var_usage) {
                    var_usage->used = 1;
                }
            }
            break;
        }
        
        case AST_FUNCTION: {
            int line = (node->location.first_line > 0) ? node->location.first_line : 1;
            int column = (node->location.first_column > 0) ? node->location.first_column : 1;
            
            add_symbol(table, node->data.function.name, SYMBOL_FUNCTION, TYPE_UNKNOWN);
            SymbolTable* func_scope = create_symbol_table(table);
            if (node->data.function.params && node->data.function.params->type == AST_EXPRESSION_LIST) {
                for (int i = 0; i < node->data.function.params->data.expression_list.expression_count; i++) {
                    ASTNode* param = node->data.function.params->data.expression_list.expressions[i];
                    if (param->type == AST_IDENTIFIER) {
                        add_symbol(func_scope, param->data.identifier.name, SYMBOL_VARIABLE, TYPE_UNKNOWN);
                        VariableUsage* var_usage = find_variable_in_usage(*usage_list, param->data.identifier.name);
                        if (!var_usage) {
                            int param_line = (param->location.first_line > 0) ? param->location.first_line : line;
                            int param_column = (param->location.first_column > 0) ? param->location.first_column : column;
                            VariableUsage* new_usage = add_variable_to_usage_with_column(*usage_list, param->data.identifier.name, param_line, param_column);
                            *usage_list = new_usage;
                        }
                    }
                }
            }
            if (node->data.function.body) {
                warnings_found += check_unused_variables_with_usage(node->data.function.body, func_scope, usage_list);
            }
            destroy_symbol_table(func_scope);
            break;
        }
        
        case AST_CALL: {
            if (node->data.call.func && node->data.call.func->type == AST_IDENTIFIER) {
                Symbol* sym = lookup_symbol(table, node->data.call.func->data.identifier.name);
                if (sym) {
                    VariableUsage* var_usage = find_variable_in_usage(*usage_list, node->data.call.func->data.identifier.name);
                    if (var_usage) {
                        var_usage->used = 1;
                    }
                }
            }
            if (node->data.call.args) {
                warnings_found += check_unused_variables_with_usage(node->data.call.args, table, usage_list);
            }
            break;
        }
        
        case AST_BINOP:
        case AST_UNARYOP: {
            if (node->type == AST_BINOP) {
                warnings_found += check_unused_variables_with_usage(node->data.binop.left, table, usage_list);
                warnings_found += check_unused_variables_with_usage(node->data.binop.right, table, usage_list);
            } else {
                warnings_found += check_unused_variables_with_usage(node->data.unaryop.expr, table, usage_list);
            }
            break;
        }
        
        case AST_IF: {
            warnings_found += check_unused_variables_with_usage(node->data.if_stmt.condition, table, usage_list);
            warnings_found += check_unused_variables_with_usage(node->data.if_stmt.then_body, table, usage_list);
            if (node->data.if_stmt.else_body) {
                warnings_found += check_unused_variables_with_usage(node->data.if_stmt.else_body, table, usage_list);
            }
            break;
        }
        
        case AST_WHILE: {
            warnings_found += check_unused_variables_with_usage(node->data.while_stmt.condition, table, usage_list);
            warnings_found += check_unused_variables_with_usage(node->data.while_stmt.body, table, usage_list);
            break;
        }
        
        case AST_FOR: {
            warnings_found += check_unused_variables_with_usage(node->data.for_stmt.start, table, usage_list);
            warnings_found += check_unused_variables_with_usage(node->data.for_stmt.end, table, usage_list);
            if (node->data.for_stmt.var && node->data.for_stmt.var->type == AST_IDENTIFIER) {// 获取循环变量定义的行号和列号
                int line = (node->data.for_stmt.var->location.first_line > 0) ? node->data.for_stmt.var->location.first_line : 1;
                int column = (node->data.for_stmt.var->location.first_column > 0) ? node->data.for_stmt.var->location.first_column : 1;
                add_symbol(table, node->data.for_stmt.var->data.identifier.name, SYMBOL_VARIABLE, TYPE_UNKNOWN);
                /*添加变量到使用情况列表，包含列号信息*/
                VariableUsage* var_usage = find_variable_in_usage(*usage_list, node->data.for_stmt.var->data.identifier.name);
                if (!var_usage) {
                    VariableUsage* new_usage = add_variable_to_usage_with_column(*usage_list, node->data.for_stmt.var->data.identifier.name, line, column);
                    *usage_list = new_usage;
                }
            }
            warnings_found += check_unused_variables_with_usage(node->data.for_stmt.body, table, usage_list);
            break;
        }
        
        case AST_PRINT: {
            warnings_found += check_unused_variables_with_usage(node->data.print.expr, table, usage_list);
            break;
        }
        
        case AST_INPUT: {
            if (node->data.input.prompt) {
                warnings_found += check_unused_variables_with_usage(node->data.input.prompt, table, usage_list);
            }
            break;
        }
        
        case AST_TOINT:
        case AST_TOFLOAT: {
            if (node->type == AST_TOINT) {
                warnings_found += check_unused_variables_with_usage(node->data.toint.expr, table, usage_list);
            } else {
                warnings_found += check_unused_variables_with_usage(node->data.tofloat.expr, table, usage_list);
            }
            break;
        }
        
        case AST_RETURN: {
            if (node->data.return_stmt.expr) {
                warnings_found += check_unused_variables_with_usage(node->data.return_stmt.expr, table, usage_list);
            }
            break;
        }
        
        case AST_EXPRESSION_LIST: {
            for (int i = 0; i < node->data.expression_list.expression_count; i++) {
                warnings_found += check_unused_variables_with_usage(node->data.expression_list.expressions[i], table, usage_list);
            }
            break;
        }
        case AST_INDEX: {
            if (node->data.index.target) warnings_found += check_unused_variables_with_usage(node->data.index.target, table, usage_list);
            if (node->data.index.index) warnings_found += check_unused_variables_with_usage(node->data.index.index, table, usage_list);
            break;
        }
        
        case AST_NUM_INT:
        case AST_NUM_FLOAT:
        case AST_STRING:
            break;
            
        default:
            break;
    }
    
    return warnings_found;
}

int check_undefined_symbols_in_node(ASTNode* node, SymbolTable* table) {
    return check_undefined_symbols_in_node_with_visited(node, table, NULL);
}