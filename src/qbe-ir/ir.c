#include "../include/qbe-ir/ir.h"
#include "../include/ast.h"
#include "../include/struct.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
int gen_expr(QbeGenState* state, ASTNode* node);
const char* infer_node_qbe_type(QbeGenState* state, ASTNode* node);

/*===================头文件和初始化部分=======================*/
QbeGenState* init_state(FILE* output) {
    QbeGenState* state = malloc(sizeof(QbeGenState));
    state->output = output;
    state->reg_counter = 0;
    state->label_counter = 0;
    state->var_count = 0;
    state->var_capacity = 16;
    state->var_names = malloc(state->var_capacity * sizeof(char*));
    state->var_regs = malloc(state->var_capacity * sizeof(int));
    state->var_types = malloc(state->var_capacity * sizeof(char*));
    state->string_value = NULL;
    state->used_int_print = 0;
    state->used_string_print = 0;
    state->global_vars = malloc(16 * sizeof(char*));
    state->global_var_types = malloc(16 * sizeof(char*));
    state->global_var_count = 0;
    state->global_var_capacity = 16;
    state->func_has_return = 0;
    state->func_names = malloc(16 * sizeof(char*));
    state->func_ret_types = malloc(16 * sizeof(char*));
    state->func_count = 0;
    state->func_capacity = 16;
    state->current_ret_type = NULL;
    state->pending_strings = malloc(16 * sizeof(char*));
    state->pending_string_count = 0;
    state->pending_string_capacity = 16;
    state->ptr_vars = malloc(16 * sizeof(char*));
    state->ptr_var_count = 0;
    state->ptr_var_capacity = 16;
    state->struct_names = malloc(16 * sizeof(char*));
    state->struct_field_names = malloc(16 * sizeof(char**));
    state->struct_field_types = malloc(16 * sizeof(char**));
    state->struct_field_counts = malloc(16 * sizeof(int));
    state->struct_count = 0;
    state->struct_capacity = 16;
    state->pending_struct_defs = malloc(16 * sizeof(char*));
    state->pending_struct_defs_count = 0;
    state->pending_struct_defs_capacity = 16;
    return state;
}
/*==================变量和函数管理部分======================*/
void record_global_var(QbeGenState* state, const char* var_name, const char* type) {
    for (int i = 0; i < state->global_var_count; i++) {// 检查是否已经存在该全局变量
        if (strcmp(state->global_vars[i], var_name) == 0) {
            const char *old = state->global_var_types[i];
            if (old && strncmp(old, "struct:", 7) == 0) return;
            if (type && strncmp(type, "struct:", 7) == 0) {
                free(state->global_var_types[i]);
                state->global_var_types[i] = strdup(type);
                return;
            }
            int rank_old = (strcmp(old, "d") == 0) ? 2 : (strcmp(old, "l") == 0 ? 1 : 0);
            int rank_new = (strcmp(type, "d") == 0) ? 2 : (strcmp(type, "l") == 0 ? 1 : 0);
            if (rank_new > rank_old) {
                free(state->global_var_types[i]);
                state->global_var_types[i] = strdup(type);
            }
            return;
        }
    }
    
    if (state->global_var_count >= state->global_var_capacity){// 如果不存在，添加新的全局变量
        state->global_var_capacity *= 2;
        state->global_vars = realloc(state->global_vars, state->global_var_capacity * sizeof(char*));
        state->global_var_types = realloc(state->global_var_types, state->global_var_capacity * sizeof(char*));
    }

    state->global_vars[state->global_var_count] = strdup(var_name);
    state->global_var_types[state->global_var_count] = strdup(type);// 存储类型
    state->global_var_count++;
}
void record_ptr_var(QbeGenState* state, const char* var_name) {//记录指针变量
    for (int i = 0; i < state->ptr_var_count; i++) {// 检查是否已经存在该指针变量记录
        if (strcmp(state->ptr_vars[i], var_name) == 0) {
            return; //已经记录过了
        }
    }

    if (state->ptr_var_count >= state->ptr_var_capacity) {//如果不存在 添加新的指针变量记录
        state->ptr_var_capacity *= 2;
        state->ptr_vars = realloc(state->ptr_vars, state->ptr_var_capacity * sizeof(char*));
    }

    state->ptr_vars[state->ptr_var_count] = strdup(var_name);
    state->ptr_var_count++;
}
int is_ptr_var(QbeGenState* state, const char* var_name){//检查是否是指针变量
    for (int i = 0; i < state->ptr_var_count; i++) {
        if (strcmp(state->ptr_vars[i], var_name) == 0) {
            return 1; //是指针变量
        }
    }
    return 0; //反之
}
int next_reg(QbeGenState* state) {//获取下一个寄存器编号
    return state->reg_counter++;
}
int next_label(QbeGenState* state) {
    return state->label_counter++;
}
void map_var_to_reg(QbeGenState* state, const char* var_name, int reg, const char* type) {
    for (int i = 0; i < state->var_count; i++) {
        if (strcmp(state->var_names[i], var_name) == 0) {
            state->var_regs[i] = reg;
            return;
        }
    }
    
    if (state->var_count >= state->var_capacity) {//如果不存在，添加新的映射
        state->var_capacity *= 2;
        state->var_names = realloc(state->var_names, state->var_capacity * sizeof(char*));
        state->var_regs = realloc(state->var_regs, state->var_capacity * sizeof(int));
        state->var_types = realloc(state->var_types, state->var_capacity * sizeof(char*));
    }
    
    state->var_names[state->var_count] = strdup(var_name);
    state->var_regs[state->var_count] = reg;
    state->var_types[state->var_count] = strdup(type);
    state->var_count++;
}
int find_var_reg(QbeGenState* state, const char* var_name) {
    for (int i = 0; i < state->var_count; i++) {
        if (strcmp(state->var_names[i], var_name) == 0) {
            return state->var_regs[i];
        }
    }
    return -1;//没找到就返回错误吗
}

const char* find_var_type(QbeGenState* state, const char* var_name) {
    for (int i = 0; i < state->var_count; i++) {
        if (strcmp(state->var_names[i], var_name) == 0) {
            return state->var_types[i];
        }
    }
    for (int i = 0; i < state->global_var_count; i++) {
        if (strcmp(state->global_vars[i], var_name) == 0) return state->global_var_types[i];
    }
    return NULL;
}

void record_function(QbeGenState* state, const char* name, const char* rettype) {
    for (int i = 0; i < state->func_count; i++) if (strcmp(state->func_names[i], name) == 0) return;
    if (state->func_count >= state->func_capacity) {
        state->func_capacity *= 2;
        state->func_names = realloc(state->func_names, state->func_capacity * sizeof(char*));
        state->func_ret_types = realloc(state->func_ret_types, state->func_capacity * sizeof(char*));
    }
    state->func_names[state->func_count] = strdup(name);
    state->func_ret_types[state->func_count] = strdup(rettype);
    state->func_count++;
}

const char* find_function_ret_type(QbeGenState* state, const char* name) {
    for (int i = 0; i < state->func_count; i++) if (strcmp(state->func_names[i], name) == 0) return state->func_ret_types[i];
    return NULL;
}

/*=======================指令生成部分======================*/
void gen_store_global_var(QbeGenState* state, int src_reg, const char* var_name, const char* type) {
    record_global_var(state, var_name, type);
    if (strcmp(type, "w") == 0) {
        fprintf(state->output, "    storew %%r%d, $%s\n", src_reg, var_name);
    } else if (strcmp(type, "l") == 0) {
        fprintf(state->output, "    storel %%r%d, $%s\n", src_reg, var_name);
    } else if (strcmp(type, "d") == 0) {
        fprintf(state->output, "    stored %%r%d, $%s\n", src_reg, var_name);
    }
}
int gen_load_global_var(QbeGenState* state, const char* var_name, const char* type) {
    record_global_var(state, var_name, type);
    int reg = next_reg(state);
    if (strcmp(type, "w") == 0) {
        fprintf(state->output, "    %%r%d =w loadw $%s\n", reg, var_name);
    } else if (strcmp(type, "l") == 0) {
        fprintf(state->output, "    %%r%d =l loadl $%s\n", reg, var_name);
    } else if (strcmp(type, "d") == 0) {
        fprintf(state->output, "    %%r%d =d loadd $%s\n", reg, var_name);
    }
    return reg;
}
int gen_bin_op(QbeGenState* state, BinOpType op, int left_reg, int right_reg, const char* type) {// 生成二元操作
    int result_reg = next_reg(state);
    const char* op_str;
    
    switch (op) {
        case OP_ADD:
            op_str = "add";
            break;
        case OP_SUB:
            op_str = "sub";
            break;
        case OP_MUL:
            op_str = "mul";
            break;
        case OP_DIV:
            op_str = "div";
            break;
        case OP_MOD: // 666 qbe 没有直接的mod 所以只能使用a % b = a - ( a / b ) * b来取代mod: 7 % 3 = 7 - ( 7 / 3 ) * 3 = 1
            {
                int div_result_reg = next_reg(state);
                int mul_result_reg = next_reg(state);
                fprintf(state->output, "    %%r%d =%s div %%r%d, %%r%d\n", div_result_reg, type, left_reg, right_reg);
                fprintf(state->output, "    %%r%d =%s mul %%r%d, %%r%d\n", mul_result_reg, type, div_result_reg, right_reg);
                fprintf(state->output, "    %%r%d =%s sub %%r%d, %%r%d\n", result_reg, type, left_reg, mul_result_reg);
                return result_reg;
            }
        case OP_EQ:
            op_str = "ceqw";
            break;
        case OP_NE:// qbe没有直接的不等比较，需要用其他方式实现，比如用eq然后取反，所以要一个临时寄存器
            {
                int eq_result_reg = next_reg(state);
                fprintf(state->output, "    %%r%d =%s ceqw %%r%d, %%r%d\n", eq_result_reg, type, left_reg, right_reg);
                int result_reg_neg = next_reg(state);
                fprintf(state->output, "    %%r%d =%s sub 1, %%r%d\n", result_reg_neg, type, eq_result_reg);
                return result_reg_neg;
            }
        case OP_LT:
            op_str = "csltw";
            break;
        case OP_LE:
            op_str = "cslew";
            break;
        case OP_GT://qbe没有直接的大于比较，可以用小于的反向操作实现
            {// (a > b) = (b < a)
                int result_reg_gt = next_reg(state);
                fprintf(state->output, "    %%r%d =%s csltw %%r%d, %%r%d\n", result_reg_gt, type, right_reg, left_reg);
                return result_reg_gt;
            }
        case OP_GE:// QBE没有直接的大于等于比较，可以用小于的反向操作实现
            {// (a >= b) = (b <= a)
                int result_reg_ge = next_reg(state);
                fprintf(state->output, "    %%r%d =%s cslew %%r%d, %%r%d\n", result_reg_ge, type, right_reg, left_reg);
                return result_reg_ge;
            }
        default:
            op_str = "add";//默认op
            break;
    }
    
    fprintf(state->output, "    %%r%d =%s %s %%r%d, %%r%d\n", result_reg, type, op_str, left_reg, right_reg);
    return result_reg;
}
/*==================语句生成部分=====================*/
/*
gen_stmt是比较主要的生成函数，ast to qbe的核心函数
1. 结构体字面量处理：
当遇到结构体字面量赋值时（如myStruct = { field1: value1, field2: value2 }）
生成结构体初始化标记并加入待处理列表
为每个字段生成适当类型的值（字符串、整数、浮点数、指针等）
2. 变量赋值处理：
普通变量赋值（ident = expr）
根据右侧表达式的类型确定存储类型（w=32位整数，l=64位整数/指针，d=浮点数）
对于全局变量，生成store指令
对于局部变量，更新寄存器值
3. 解引用赋值处理：
处理指针解引用赋值（如@ptr = value）
生成storeind指令就会将值存储到指针指向的内存位置
4. 结构体字段赋值处理：
处理结构体字段赋值(如obj.field = value)
计算字段偏移量，生成字段地址and存储值
5.打印语句处理：
支持单个表达式或表达式列表的打印
根据表达式类型生成相应的格式化字符串（%d、%f、%s等）
生成对printf函数的调用
6. 条件语句处理：
生成条件跳转代码（jnz指令）
创建适当的标签和跳转逻辑
7. 循环语句处理：
while循环：生成条件检查和循环体的跳转逻辑
for循环：生成循环变量初始化、比较、递增和跳转逻辑
8. 函数定义处理：
生成函数声明和参数映射
设置函数返回类型
9. 返回语句处理：
生成适当的返回指令
处理返回值类型转换（如将32位变成64位，或将64位变成为32位）
//====================================================/
好了，下面是代码实现：
*/
void gen_stmt(QbeGenState* state, ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_ASSIGN: {
            if (node->data.assign.left->type == AST_IDENTIFIER &&
                node->data.assign.right->type == AST_STRUCT_LITERAL) {
                const char* varname = node->data.assign.left->data.identifier.name;
                ASTNode* lit = node->data.assign.right;
                const char* struct_name = NULL;
                if (lit->data.struct_literal.type_name && lit->data.struct_literal.type_name->type == AST_IDENTIFIER) {
                    struct_name = lit->data.struct_literal.type_name->data.identifier.name;
                }
                char** field_names = NULL;
                char** field_types = NULL;
                int field_count = 0;
                int has_struct_info = 0;
                if (struct_name) {
                    has_struct_info = qbe_get_struct_info(state, struct_name, &field_names, &field_types, &field_count);
                }
                ASTNode* init_fields = lit->data.struct_literal.fields;
                int nfields = 0;
                if (has_struct_info) nfields = field_count;
                else if (init_fields && init_fields->type == AST_EXPRESSION_LIST) nfields = init_fields->data.expression_list.expression_count;
                else nfields = 0;

                char** tokens = malloc(sizeof(char*) * (nfields > 0 ? nfields : 1));
                for (int i = 0; i < nfields; i++) tokens[i] = NULL;

                for (int fi = 0; fi < nfields; fi++) {
                    const char* ftype = NULL;
                    const char* fname = NULL;
                    ASTNode* init = NULL;
                    if (has_struct_info) {
                        ftype = field_types[fi];
                        fname = field_names[fi];
                        if (init_fields && init_fields->type == AST_EXPRESSION_LIST) {
                            for (int k = 0; k < init_fields->data.expression_list.expression_count; k++) {
                                ASTNode* a = init_fields->data.expression_list.expressions[k];
                                if (a->type == AST_ASSIGN && a->data.assign.left->type == AST_IDENTIFIER) {
                                    if (strcmp(a->data.assign.left->data.identifier.name, fname) == 0) {
                                        init = a->data.assign.right;
                                        break;
                                    }
                                }
                            }
                        }
                    } else {
                        ASTNode* a = init_fields->data.expression_list.expressions[fi];
                        if (a->type == AST_ASSIGN) init = a->data.assign.right;
                        else init = a;
                    }

                    char buf[512]; buf[0] = '\0';
                    if (ftype && strcmp(ftype, "l") == 0) {
                        if (init && init->type == AST_STRING) {
                            int sl = next_label(state);
                            snprintf(buf, sizeof(buf), "%s_%s_str%d", varname, fname ? fname : "f", sl);
                            size_t marker_len = strlen(buf) + strlen(init->data.string.value) + 20;
                            char *marker = malloc(marker_len);
                            snprintf(marker, marker_len, "STR_LITERAL:%s:%s", buf, init->data.string.value);
                            if (state->pending_string_count >= state->pending_string_capacity) {
                                state->pending_string_capacity *= 2;
                                state->pending_strings = realloc(state->pending_strings, state->pending_string_capacity * sizeof(char*));
                            }
                            state->pending_strings[state->pending_string_count++] = marker;
                            size_t toklen = strlen(buf) + 4;
                            char *tok = malloc(toklen);
                            snprintf(tok, toklen, "l $%s", buf);
                            tokens[fi] = tok;
                        } else if (init && init->type == AST_IDENTIFIER) {
                            size_t toklen = strlen(init->data.identifier.name) + 4;
                            char *tok = malloc(toklen);
                            snprintf(tok, toklen, "l $%s", init->data.identifier.name);
                            tokens[fi] = tok;
                        } else {
                            tokens[fi] = strdup("l 0");
                        }
                    } else if (ftype && strcmp(ftype, "l") == 0) {
                        if (init && init->type == AST_NUM_INT) {
                            int need = snprintf(NULL, 0, "l %lld", init->data.num_int.value);
                            char *tok = malloc(need + 1);
                            snprintf(tok, need + 1, "l %lld", init->data.num_int.value);
                            tokens[fi] = tok;
                        } else {
                            tokens[fi] = strdup("l 0");
                        }
                    } else if (ftype && strcmp(ftype, "d") == 0) {
                        if (init && init->type == AST_NUM_FLOAT) {
                            int need = snprintf(NULL, 0, "d d_%g", init->data.num_float.value);
                            char *tok = malloc(need + 1);
                            snprintf(tok, need + 1, "d d_%g", init->data.num_float.value);
                            tokens[fi] = tok;
                        } else {
                            tokens[fi] = strdup("d d_0.0");
                        }
                    } else {//w
                        if (init && init->type == AST_NUM_INT) {
                            int need = snprintf(NULL, 0, "w %lld", init->data.num_int.value);
                            char *tok = malloc(need + 1);
                            snprintf(tok, need + 1, "w %lld", init->data.num_int.value);
                            tokens[fi] = tok;
                        } else if (init && init->type == AST_IDENTIFIER) {
                            size_t toklen2 = strlen(init->data.identifier.name) + 4;
                            char *tok = malloc(toklen2);
                            snprintf(tok, toklen2, "w $%s", init->data.identifier.name);
                            tokens[fi] = tok;
                        } else {
                            tokens[fi] = strdup("w 0");
                        }
                    }
                }
                size_t total_len = strlen("STRUCT_INIT:") + strlen(varname) + 2;
                for (int i = 0; i < nfields; i++) total_len += strlen(tokens[i]) + 1;
                char *struct_marker = malloc(total_len + 1);
                struct_marker[0] = '\0';
                strcat(struct_marker, "STRUCT_INIT:");
                strcat(struct_marker, varname);
                strcat(struct_marker, ":");
                for (int i = 0; i < nfields; i++) {
                    if (i > 0) strcat(struct_marker, "|");
                    strcat(struct_marker, tokens[i]);
                }

                if (state->pending_string_count >= state->pending_string_capacity) {
                    state->pending_string_capacity *= 2;
                    state->pending_strings = realloc(state->pending_strings, state->pending_string_capacity * sizeof(char*));
                }
                state->pending_strings[state->pending_string_count++] = struct_marker;

                for (int i = 0; i < nfields; i++) if (tokens[i]) free(tokens[i]);
                free(tokens);
                // 记录这个全局变量是一个结构体实例（故它不会再次作为标量被生成
                char *smark = malloc(strlen(struct_name ? struct_name : "") + 10);
                if (struct_name) snprintf(smark, strlen(struct_name) + 10, "struct:%s", struct_name);
                else strcpy(smark, "struct:unknown");
                record_global_var(state, varname, smark);
                free(smark);
                break;
            }

            int value_reg = gen_expr(state, node->data.assign.right);
            
            if (node->data.assign.left->type == AST_IDENTIFIER) {
                const char* type = "w";//默认
                if (node->data.assign.right->type == AST_UNARYOP && 
                         node->data.assign.right->data.unaryop.op == OP_ADDRESS) {
                    type = "l";//指针
                } else if (node->data.assign.right->type == AST_NUM_FLOAT) {
                    type = "d";//浮点数
                } else {
                    type = infer_node_qbe_type(state, node->data.assign.right);
                }

                const char* varname = node->data.assign.left->data.identifier.name;
                int existing = find_var_reg(state, varname);
                if (existing >= 0) {
                    fprintf(state->output, "    %%r%d =%s copy %%r%d\n", existing, type, value_reg);
                } else {
                    gen_store_global_var(state, value_reg, varname, type);
                }
                if (node->data.assign.right->type == AST_UNARYOP &&
                    node->data.assign.right->data.unaryop.op == OP_ADDRESS) {// 如果右值是取地址操作（例如 ptr = &x），记录指针变量信息
                    record_ptr_var(state, varname);
                }
            }
            else if (node->data.assign.left->type == AST_UNARYOP && 
                node->data.assign.left->data.unaryop.op == OP_DEREF) {//解引用
                ASTNode* deref_expr = node->data.assign.left->data.unaryop.expr;
                int ptr_reg = gen_expr(state, deref_expr);
                const char* type = infer_node_qbe_type(state, node->data.assign.right);
                fprintf(state->output, "    store%s %%r%d, %%r%d\n", type, value_reg, ptr_reg);
            }
            else if (node->data.assign.left->type == AST_INDEX) {//处理结构体字段赋值 obj.field = value
                ASTNode* target = node->data.assign.left->data.index.target;
                ASTNode* field = node->data.assign.left->data.index.index;
                
                if (target->type == AST_IDENTIFIER && field->type == AST_IDENTIFIER) {
                    const char* obj_name = target->data.identifier.name;
                    const char* field_name = field->data.identifier.name;
                    const char* obj_type = find_var_type(state, obj_name);// 获取对象类型，判断是否为结构体
                    if (obj_type && strncmp(obj_type, "struct:", 7) == 0) {
                        const char* struct_name = obj_type + 7;
                        char** field_names = NULL;
                        char** field_types = NULL;
                        int field_count = 0;
                        if (qbe_get_struct_info(state, struct_name, &field_names, &field_types, &field_count)) {// 查找字段的偏移量
                            int offset = 0;
                            int found_field = -1;
                            for (int i = 0; i < field_count; i++) {
                                if (strcmp(field_names[i], field_name) == 0) {
                                    found_field = i;
                                    break;
                                }
                                if (strcmp(field_types[i], "l") == 0 || strcmp(field_types[i], "d") == 0) {// 计算累积偏移量
                                    offset += 8;//64位值8b
                                } else {
                                    offset += 4;//32位值4b
                                }
                            }
                            
                            if (found_field >= 0) {
                                int field_addr_reg = next_reg(state);
                                fprintf(state->output, "    %%r%d =l add $%s, %d\n", field_addr_reg, obj_name, offset);

                                fprintf(state->output, "    store%s %%r%d, %%r%d\n", field_types[found_field], value_reg, field_addr_reg);
                            }
                        }
                    }
                }
            }
            break;
        }
        
        case AST_PRINT: {
            if (node->data.print.expr->type == AST_STRING) {
                fprintf(state->output, "    call $printf(l $fmt_str0)\n");
                if (state->pending_string_count >= state->pending_string_capacity) {
                    state->pending_string_capacity *= 2;
                    state->pending_strings = realloc(state->pending_strings, 
                        state->pending_string_capacity * sizeof(char*));
                }
                state->pending_strings[state->pending_string_count++] = 
                    strdup(node->data.print.expr->data.string.value);
            }
            else if (node->data.print.expr->type == AST_EXPRESSION_LIST) {
                ASTNode* expr_list = node->data.print.expr;
                int count = expr_list->data.expression_list.expression_count;
                int *arg_regs = malloc(count * sizeof(int));//计算所需的寄存器和类型
                char **arg_types = malloc(count * sizeof(char*));
                for (int i = 0; i < count; i++) {
                    ASTNode* expr = expr_list->data.expression_list.expressions[i];
                    if (expr->type == AST_STRING) {//对字符串字面量，生成标签和寄存器
                        arg_regs[i] = next_reg(state);
                        int str_label = next_label(state);
                        fprintf(state->output, "    %%r%d =l loadl $str_ptr%d\n", arg_regs[i], str_label);
                        if (state->pending_string_count >= state->pending_string_capacity) {
                            state->pending_string_capacity *= 2;
                            state->pending_strings = realloc(state->pending_strings, 
                                state->pending_string_capacity * sizeof(char*));
                        }
                        char *str_marker = malloc(256);
                        snprintf(str_marker, 256, "STR_PTR:%d:%s", str_label, expr->data.string.value);
                        state->pending_strings[state->pending_string_count++] = str_marker;
                        arg_types[i] = strdup("l");//指针类型
                    } else {
                        if (expr->type == AST_IDENTIFIER) {
                            const char* name = expr->data.identifier.name;
                            const char* t = find_var_type(state, name);
                            if (!t) t = "w";
                            arg_regs[i] = gen_load_global_var(state, name, t);
                            arg_types[i] = strdup(t);
                        } else {
                            arg_regs[i] = gen_expr(state, expr);
                            arg_types[i] = strdup(infer_node_qbe_type(state, expr));
                        }
                    }
                }
                char format_str[1024] = {0};
                strcpy(format_str, "");
                for (int i = 0; i < count; i++) {
                    ASTNode* expr = expr_list->data.expression_list.expressions[i];
                    
                    if (expr->type == AST_STRING) {
                        strcat(format_str, "%s");
                    } else {
                        const char* expr_type = infer_node_qbe_type(state, expr);
                        if (strcmp(expr_type, "d") == 0) {
                            strcat(format_str, "%f");//浮点数
                        } else if (strcmp(expr_type, "l") == 0) {
                            if (expr->type == AST_STRING) {
                                strcat(format_str, "%s");
                            } else if (expr->type == AST_IDENTIFIER) {
                                strcat(format_str, "%s");
                            } else {
                                strcat(format_str, "%s");
                            }
                        } else {
                            strcat(format_str, "%d");
                        }
                    }
                    if (i < count - 1) {//除了最后一个元素，每个格式化字符串后面加一个空格
                        strcat(format_str, " ");
                    }
                }
                strcat(format_str, "\\n");
                int fmt_label = next_label(state);
                if (state->pending_string_count >= state->pending_string_capacity) {
                    state->pending_string_capacity *= 2;
                    state->pending_strings = realloc(state->pending_strings, 
                        state->pending_string_capacity * sizeof(char*));
                }
                char *fmt_str_marker = malloc(strlen(format_str) + 20);
                snprintf(fmt_str_marker, strlen(format_str) + 20, "FMT:%d:%s", fmt_label, format_str);
                state->pending_strings[state->pending_string_count++] = fmt_str_marker;
                fprintf(state->output, "    call $printf(l $fmt_args%d", fmt_label);
                
                for (int i = 0; i < count; i++) {
                    fprintf(state->output, ", %s %%r%d", arg_types[i], arg_regs[i]);
                }
                fprintf(state->output, ")\n");
                free(arg_regs);
                for (int i = 0; i < count; i++) {
                    if (arg_types[i]) free(arg_types[i]);
                }
                free(arg_types);
            } else {
                const char* actual_type = infer_node_qbe_type(state, node->data.print.expr);
                int arg_reg;
                arg_reg = gen_expr(state, node->data.print.expr);
                if (strcmp(actual_type, "d") == 0) {
                    fprintf(state->output, "    call $printf(l $fmt_f, d %%r%d)\n", arg_reg);
                    state->used_string_print = 1;
                    } else if (strcmp(actual_type, "l") == 0) {
                    if (node->data.print.expr->type == AST_STRING) {
                        fprintf(state->output, "    call $printf(l $fmt_s, l %%r%d)\n", arg_reg);
                        state->used_string_print = 1;
                    } else if (node->data.print.expr->type == AST_IDENTIFIER) {
                        const char* var_name = node->data.print.expr->data.identifier.name;
                        if (is_ptr_var(state, var_name)) {
                            fprintf(state->output, "    call $printf(l $fmt_l, l %%r%d)\n", arg_reg);
                            state->used_int_print = 1;
                        } else {
                            fprintf(state->output, "    call $printf(l $fmt_s, l %%r%d)\n", arg_reg);
                            state->used_string_print = 1;
                        }
                    } else if (node->data.print.expr->type == AST_INDEX) {
                        fprintf(state->output, "    call $printf(l $fmt_s, l %%r%d)\n", arg_reg);
                        state->used_string_print = 1;
                    } else {
                        fprintf(state->output, "    call $printf(l $fmt_l, l %%r%d)\n", arg_reg);
                        state->used_int_print = 1;
                    }
                } else if (node->data.print.expr->type == AST_STRING) {
                    fprintf(state->output, "    call $printf(l $fmt_s, l %%r%d)\n", arg_reg);
                    state->used_string_print = 1;
                } else {
                    fprintf(state->output, "    call $printf(l $fmt, w %%r%d)\n", arg_reg);
                    state->used_int_print = 1;
                }//666写完了
            }
            break;
        }
        
        case AST_IF: {
            int then_label = next_label(state);
            int else_label = next_label(state);
            int end_label = next_label(state);

            int cond_reg = gen_expr(state, node->data.if_stmt.condition);
            fprintf(state->output, "    jnz %%r%d, @L%d, @L%d\n", cond_reg, then_label, else_label);// jnz cond, @then, @else
            fprintf(state->output, "@L%d\n", then_label);// then 部分
            int old_func_ret = state->func_has_return;
            state->func_has_return = 0;
            if (node->data.if_stmt.then_body) {
                gen_stmt(state, node->data.if_stmt.then_body);
            }
            int then_has_ret = state->func_has_return;
            state->func_has_return = old_func_ret || then_has_ret;
            int emitted_end_jump = 0;
            if (!then_has_ret) {
                fprintf(state->output, "    jmp @L%d\n", end_label);
                emitted_end_jump = 1;
            }
            fprintf(state->output, "@L%d\n", else_label);//else 部分
            if (node->data.if_stmt.else_body) {
                gen_stmt(state, node->data.if_stmt.else_body);
            }
            if (emitted_end_jump) {
                fprintf(state->output, "@L%d\n", end_label);
            }
            break;
        }
        
        case AST_WHILE: {
            int start_label = next_label(state);
            int body_label = next_label(state);
            int end_label = next_label(state);
            
            fprintf(state->output, "    jmp @L%d\n", start_label);
            fprintf(state->output, "@L%d\n", start_label);
            
            int cond_reg = gen_expr(state, node->data.while_stmt.condition);
            fprintf(state->output, "    jnz %%r%d, @L%d, @L%d\n", cond_reg, body_label, end_label);
            
            fprintf(state->output, "@L%d\n", body_label);
            gen_stmt(state, node->data.while_stmt.body);
            fprintf(state->output, "    jmp @L%d\n", start_label);
            
            fprintf(state->output, "@L%d\n", end_label);
            break;
        }
        
        case AST_FOR: {
            /*
            for循环结构：
            for var in start..end { body }
            生成的ir belike：
            初始化循环变量
            @start:
                if var <= end goto body else goto end
            @body:
                执行循环体
                var = var + 1
                oto start
            @end:
                结束循环
            */
            
            int start_label = next_label(state);
            int body_label = next_label(state);
            int end_label = next_label(state);
            int start_reg = gen_expr(state, node->data.for_stmt.start);// 生成起始值并分配给循环变量
            const char* loop_type = infer_node_qbe_type(state, node->data.for_stmt.start);
            const char* var_name = node->data.for_stmt.var->data.identifier.name;// 确保循环变量被映射到一个寄存器
            record_global_var(state, var_name, loop_type);
            gen_store_global_var(state, start_reg, var_name, loop_type);
            int end_reg = gen_expr(state, node->data.for_stmt.end);// 生成结束值并存储到临时寄存器
            fprintf(state->output, "@L%d\n", start_label);
            int curr_var_reg = gen_load_global_var(state, var_name, loop_type);
            int cmp_reg = next_reg(state);
            fprintf(state->output, "    %%r%d =%s csltw %%r%d, %%r%d\n", cmp_reg, loop_type, curr_var_reg, end_reg);
            fprintf(state->output, "    jnz %%r%d, @L%d, @L%d\n", cmp_reg, body_label, end_label);
            fprintf(state->output, "@L%d\n", body_label);
            gen_stmt(state, node->data.for_stmt.body);
            int one_reg = next_reg(state);
            fprintf(state->output, "    %%r%d =%s copy 1\n", one_reg, loop_type);
            int next_val_reg = next_reg(state);
            fprintf(state->output, "    %%r%d =%s add %%r%d, %%r%d\n", next_val_reg, loop_type, curr_var_reg, one_reg);
            gen_store_global_var(state, next_val_reg, var_name, loop_type);
            fprintf(state->output, "    jmp @L%d\n", start_label);
            fprintf(state->output, "@L%d\n", end_label);
            break;
        }

        case AST_EXPRESSION_LIST: {
            for (int i = 0; i < node->data.expression_list.expression_count; i++) {
                gen_stmt(state, node->data.expression_list.expressions[i]);
            }
            break;
        }

        case AST_STRUCT_DEF: {
            qbe_emit_struct_def(state, node);
            break;
        }

        case AST_CALL: {
            gen_expr(state, node);
            break;
        }

        case AST_RETURN: {
            state->func_has_return = 1;
            if (node->data.return_stmt.expr) {
                int ret_reg = gen_expr(state, node->data.return_stmt.expr);
                const char* expr_t = infer_node_qbe_type(state, node->data.return_stmt.expr);
                const char* expect_t = state->current_ret_type ? state->current_ret_type : "w";

                if (strcmp(expect_t, "l") == 0 && strcmp(expr_t, "w") == 0) {
                    int tmp = next_reg(state);
                    fprintf(state->output, "    %%r%d =l extsw %%r%d\n", tmp, ret_reg);
                    fprintf(state->output, "    ret %%r%d\n", tmp);
                } else if (strcmp(expect_t, "w") == 0 && strcmp(expr_t, "l") == 0) {
                    int tmp = next_reg(state);
                    fprintf(state->output, "    %%r%d =w copy %%r%d\n", tmp, ret_reg);
                    fprintf(state->output, "    ret %%r%d\n", tmp);
                } else {
                    fprintf(state->output, "    ret %%r%d\n", ret_reg);
                }
            } else {
                fprintf(state->output, "    ret 0\n");
            }
            break;
        }
        case AST_FUNCTION: {
            const char* fname = node->data.function.name;

            
            const char* rett = "w";//默认
            if (node->data.function.return_type) {
                switch (node->data.function.return_type->type) {
                    case AST_TYPE_INT32: rett = "w"; break;
                    case AST_TYPE_INT64: rett = "l"; break;
                    case AST_TYPE_FLOAT32: case AST_TYPE_FLOAT64: rett = "d"; break;
                    case AST_TYPE_STRING: rett = "l"; break;
                    case AST_TYPE_VOID: rett = "w"; break;
                    default: rett = "w"; break;
                }
            }
            fprintf(state->output, "export function %s $%s(", rett, fname);
            ASTNode* params = node->data.function.params;
            // 记录函数返回类型
            record_function(state, fname, rett);
            if (params && params->type == AST_EXPRESSION_LIST) {
                for (int i = 0; i < params->data.expression_list.expression_count; i++) {
                    ASTNode* p = params->data.expression_list.expressions[i];
                    const char* pname = NULL;
                    const char* ptype = "w";

                    if (p->type == AST_IDENTIFIER) {
                        pname = p->data.identifier.name;
                    } else if (p->type == AST_ASSIGN) {
                        // annotated param: identifier : type
                        if (p->data.assign.left && p->data.assign.left->type == AST_IDENTIFIER) {
                            pname = p->data.assign.left->data.identifier.name;
                        }
                        if (p->data.assign.right) {
                            switch (p->data.assign.right->type) {
                                case AST_TYPE_INT32: ptype = "w"; break;
                                case AST_TYPE_INT64: ptype = "l"; break;
                                case AST_TYPE_FLOAT32: case AST_TYPE_FLOAT64: ptype = "d"; break;
                                case AST_TYPE_STRING: ptype = "l"; break;
                                case AST_TYPE_POINTER: ptype = "l"; break; //指针参数映射为 l
                                default: ptype = "w"; break;
                            }
                        }
                    }

                    if (pname) {
                        if (i > 0) fprintf(state->output, ",");
                        fprintf(state->output, "%s%%p%s", ptype, pname);
                    }
                }
            }

            fprintf(state->output, ") {\n");
            fprintf(state->output, "@start\n");
            int old_var_count = state->var_count;//保存当前局部变量计数，以便函数返回后恢复
            if (params && params->type == AST_EXPRESSION_LIST) {
                for (int i = 0; i < params->data.expression_list.expression_count; i++) {
                    ASTNode* p = params->data.expression_list.expressions[i];
                    const char* pname = NULL;
                    const char* ptype = "w";

                    if (p->type == AST_IDENTIFIER) {
                        pname = p->data.identifier.name;
                    } else if (p->type == AST_ASSIGN) {
                        if (p->data.assign.left && p->data.assign.left->type == AST_IDENTIFIER) {
                            pname = p->data.assign.left->data.identifier.name;
                        }
                        if (p->data.assign.right) {
                            switch (p->data.assign.right->type) {
                                case AST_TYPE_INT32: ptype = "w"; break;
                                case AST_TYPE_INT64: ptype = "l"; break;
                                case AST_TYPE_FLOAT32: case AST_TYPE_FLOAT64: ptype = "d"; break;
                                case AST_TYPE_STRING: ptype = "l"; break;
                                case AST_TYPE_POINTER: ptype = "l"; break;
                                default: ptype = "w"; break;
                            }
                        }
                    }

                    if (pname) {
                        int reg = next_reg(state);
                        /*
                        从签名中带 p 前缀的参数名复制，例如从 %pa 复制
                        但在内部映射仍使用原始参数名 pname，保持 AST 中对参数名的引用有效
                        */
                        fprintf(state->output, "    %%r%d =%s copy %%p%s\n", reg, ptype, pname);
                        map_var_to_reg(state, pname, reg, ptype);
                    }
                }
            }
            state->func_has_return = 0;
            state->current_ret_type = strdup(rett);
            if (node->data.function.body){
                gen_stmt(state, node->data.function.body);
            }
            if (!state->func_has_return) {
                fprintf(state->output, "    ret 0\n");
            }
            fprintf(state->output, "}\n");
            state->var_count = old_var_count;
            if (state->current_ret_type) {
                free(state->current_ret_type);
                state->current_ret_type = NULL;
            }
            break;
        }
        
        case AST_PROGRAM: {
            for (int i = 0; i < node->data.program.statement_count; i++) {
                gen_stmt(state, node->data.program.statements[i]);
            }
            break;
        }
        
        default:
            break;
    }
}
/*============================*/
//=======主生成api部分=========//
/*============================*/
void ir_gen_from_ast(ASTNode* ast, FILE* output) {
    if (!ast || !output) return;
    
    QbeGenState* state = init_state(output);
    
    fprintf(output, "#ast to qbe ir\n\n");
    if (ast->type == AST_PROGRAM) {
        /* First collect top-level struct/type declarations */
        for (int i = 0; i < ast->data.program.statement_count; i++) {
            ASTNode* s = ast->data.program.statements[i];
            if (s->type == AST_STRUCT_DEF) {
                gen_stmt(state, s);
            }
        }
    }
    for (int i = 0; i < state->pending_struct_defs_count; i++) {
        fprintf(output, "%s\n", state->pending_struct_defs[i]);
    }
    int main_exists = 0;
    if (ast->type == AST_PROGRAM) {
        for (int i = 0; i < ast->data.program.statement_count; i++) {
            if (ast->data.program.statements[i]->type == AST_FUNCTION) {
                const char* fn = ast->data.program.statements[i]->data.function.name;
                if (fn && strcmp(fn, "main") == 0) main_exists = 1;
                gen_stmt(state, ast->data.program.statements[i]);
            }
        }
    }
    if (!main_exists) {
        fprintf(output, "export function w $main() {\n");
        fprintf(output, "@start\n");

        if (ast->type == AST_PROGRAM) {
            for (int i = 0; i < ast->data.program.statement_count; i++) {
                if (ast->data.program.statements[i]->type != AST_FUNCTION) {
                    gen_stmt(state, ast->data.program.statements[i]);
                }
            }
        } else {
            gen_stmt(state, ast);
        }

        fprintf(output, "    ret 0\n");
        fprintf(output, "}\n");
    }
    for (int i = 0; i < state->pending_string_count; i++) {
        char* pending_str = state->pending_strings[i];
        if (strncmp(pending_str, "STR:", 4) == 0) {
            char* colon_pos1 = strchr(pending_str + 4, ':');
            if (colon_pos1 != NULL) {
                *colon_pos1 = '\0';
                int label = atoi(pending_str + 4);
                *colon_pos1 = ':';//恢复原字符串
                char* content = colon_pos1 + 1;
                
                fprintf(output, "data $str_arg%d = { b \"%s\", b 0 }\n", label, content);
            }
        }
        else if (strncmp(pending_str, "FMT:", 4) == 0) {
            char* colon_pos1 = strchr(pending_str + 4, ':');
            if (colon_pos1 != NULL) {
                *colon_pos1 = '\0';
                int label = atoi(pending_str + 4);
                *colon_pos1 = ':';
                char* content = colon_pos1 + 1;
                
                fprintf(output, "data $fmt_args%d = { b \"%s\", b 0 }\n", label, content);
            }
        } 
        else if (strncmp(pending_str, "STR_PTR:", 8) == 0) {
            char* colon_pos1 = strchr(pending_str + 8, ':');
            if (colon_pos1 != NULL) {
                *colon_pos1 = '\0';
                int label = atoi(pending_str + 8);
                *colon_pos1 = ':';
                char* content = colon_pos1 + 1;
                fprintf(output, "data $str_data%d = { b \"%s\", b 0 }\n", label, content);
                fprintf(output, "data $str_ptr%d = { l $str_data%d }\n", label, label);
            }
        }
        else if (strncmp(pending_str, "STR_LITERAL:", 12) == 0) {
            char* colon_pos1 = strchr(pending_str + 12, ':');
            if (colon_pos1 != NULL) {
                int labellen = colon_pos1 - (pending_str + 12);
                char* label = malloc(labellen + 1);
                strncpy(label, pending_str + 12, labellen);
                label[labellen] = '\0';
                *colon_pos1 = ':';
                char* content = colon_pos1 + 1;

                fprintf(output, "data $%s = { b \"%s\", b 0 }\n", label, content);
                free(label);
            }
        }
        else if (strncmp(pending_str, "STRUCT_INIT:", 12) == 0) {
            char* p = strchr(pending_str + 12, ':');
            if (p) {
                int varlen = p - (pending_str + 12);
                char* varname = malloc(varlen + 1);
                strncpy(varname, pending_str + 12, varlen);
                varname[varlen] = '\0';

                char* tokens = p + 1;
                fprintf(output, "data $%s = { ", varname);
                char* tok = strtok(tokens, "|");
                int first = 1;
                while (tok) {
                    if (!first) fprintf(output, ", ");
                    fprintf(output, "%s", tok);
                    first = 0;
                    tok = strtok(NULL, "|");
                }
                fprintf(output, " }\n");
                free(varname);
            }
        }
        else {
            fprintf(output, "data $fmt_str%d = { b \"%s\\n\", b 0 }\n", i, pending_str);
        }
    }
    for (int i = 0; i < state->pending_string_count; i++) {
        free(state->pending_strings[i]);
    }
    free(state->pending_strings);
    for (int i = 0; i < state->pending_struct_defs_count; i++) {
        free(state->pending_struct_defs[i]);
    }
    free(state->pending_struct_defs);
    for (int i = 0; i < state->global_var_count; i++) {
        if (strncmp(state->global_var_types[i], "struct:", 7) == 0) continue;
        if (strcmp(state->global_var_types[i], "d") == 0) {
            fprintf(output, "data $%s = { d d_0.0 }\n", state->global_vars[i]);
        } else if (strcmp(state->global_var_types[i], "l") == 0) {
            fprintf(output, "data $%s = { l 0 }\n", state->global_vars[i]);
        } else {
            fprintf(output, "data $%s = { w 0 }\n", state->global_vars[i]);
        }
    }
    if (state->used_string_print && state->string_value) {
        fprintf(output, "data $fmt_str = { b \"%s\\n\", b 0 }\n", state->string_value);
        free(state->string_value);
    }
    if (state->used_int_print) {
        fprintf(output, "data $fmt = { b \"%%d\\n\", b 0 }\n");
    }
    if (state->used_string_print) {
        fprintf(output, "data $fmt_f = { b \"%%f\\n\", b 0 }\n");
    }
    fprintf(output, "data $fmt_l = { b \"%%ld\\n\", b 0 }\n");
    fprintf(output, "data $fmt_s = { b \"%%s\\n\", b 0 }\n");
    for (int i = 0; i < state->var_count; i++) {
        free(state->var_names[i]);
        free(state->var_types[i]);
    }
    free(state->var_names);
    free(state->var_regs);
    free(state->var_types);
    
    for (int i = 0; i < state->global_var_count; i++) {
        free(state->global_vars[i]);
        free(state->global_var_types[i]);
    }
    free(state->global_vars);
    free(state->global_var_types);
    for (int i = 0; i < state->ptr_var_count; i++) {
        free(state->ptr_vars[i]);
    }
    free(state->ptr_vars);
    
    for (int i = 0; i < state->func_count; i++) {
        free(state->func_names[i]);
        free(state->func_ret_types[i]);
    }
    free(state->func_names);
    free(state->func_ret_types);
    if (state->current_ret_type) free(state->current_ret_type);
    for (int i = 0; i < state->struct_count; i++) {
        free(state->struct_names[i]);
        if (state->struct_field_counts[i] > 0) {
            for (int j = 0; j < state->struct_field_counts[i]; j++) {
                free(state->struct_field_names[i][j]);
                free(state->struct_field_types[i][j]);
            }
            free(state->struct_field_names[i]);
            free(state->struct_field_types[i]);
        }
    }
    free(state->struct_names);
    free(state->struct_field_names);
    free(state->struct_field_types);
    free(state->struct_field_counts);
    
    free(state);
}
void ir_gen(ASTNode* ast, FILE* fp) {
    ir_gen_from_ast(ast, fp);//为了保持兼容性
}
const char* infer_node_qbe_type(QbeGenState* state, ASTNode* node) {
    switch (node->type) {
        case AST_NIL:
            return "l";
            
        case AST_NUM_INT:
            return "w";
        case AST_UNARYOP:
            if (node->data.unaryop.op == OP_ADDRESS) {
                return "l";//地址用l类型表示
            } else if (node->data.unaryop.op == OP_DEREF) {
                const char* expr_type = infer_node_qbe_type(state, node->data.unaryop.expr);
                if (strcmp(expr_type, "l") == 0) {
                    // 假设l类型通常是某种指针，解引用后通常得到w类型（整型）
                    // 这里可以更精确地处理，但目前简单地返回w
                    return "w";
                }
                return expr_type;
            }
            return "w";
        case AST_BINOP:
            {
                const char* left_type = infer_node_qbe_type(state, node->data.binop.left);
                const char* right_type = infer_node_qbe_type(state, node->data.binop.right);
                if (strcmp(left_type, "d") == 0 || strcmp(right_type, "d") == 0) {
                    return "d";
                }
                else if (strcmp(left_type, "l") == 0 || strcmp(right_type, "l") == 0) {
                    return "l";
                }
                else {
                    return "w";
                }
            }
        case AST_CALL: {
            const char* func_name = node->data.call.func->data.identifier.name;
            const char* ret_type = find_function_ret_type(state, func_name);
            if (ret_type) {
                return ret_type;
            }
            return "w";
        }
        case AST_INDEX: {
            ASTNode* target = node->data.index.target;
            ASTNode* idx = node->data.index.index;
            if (!target || !idx) return "w";
            if (target->type == AST_IDENTIFIER && idx->type == AST_IDENTIFIER) {
                const char* var = target->data.identifier.name;
                const char* field = idx->data.identifier.name;
                const char* vartype = find_var_type(state, var);
                if (vartype && strncmp(vartype, "struct:", 7) == 0) {
                    const char* sname = vartype + 7;
                    char** fnames = NULL;
                    char** ftypes = NULL;
                    int fcount = 0;
                    if (qbe_get_struct_info(state, sname, &fnames, &ftypes, &fcount)) {
                        for (int i = 0; i < fcount; i++) {
                            if (strcmp(fnames[i], field) == 0) return ftypes[i];
                        }
                    }
                }
            }
            return "w";
        }
        default:
            return "w";
    }
}
int is_float_expr(ASTNode* node) {
    if (!node) return 0;
    
    switch (node->type) {
        case AST_NUM_FLOAT:
            return 1;
        case AST_IDENTIFIER: {
            /*
            如果标识符指向的是浮点变量，返回1
            */
            // 注意：目前我们没有一个全局的类型上下文，因此无法准确获取变量类型
            // 此功能依赖于 get_variable_type 函数，但该函数未在此代码中定义
            // InferredType type = get_variable_type(ctx, node->identifier.name);
            // return strcmp(type, "d") == 0;
            return 0; // 目前返回0，因为无法获取标识符的类型

        }
        case AST_UNARYOP:
            return is_float_expr(node->data.unaryop.expr);
        case AST_BINOP:
            return is_float_expr(node->data.binop.left) || 
                   is_float_expr(node->data.binop.right);
        case AST_CALL: {
            return 0;
        }
        default:
            return 0;
    }
}
int gen_expr(QbeGenState* state, ASTNode* node) {
    switch (node->type) {
        case AST_NIL: {
            int reg = next_reg(state);
            fprintf(state->output, "    %%r%d =l copy 0\n", reg);
            return reg;
        }
        
        case AST_NUM_INT: {
            int reg = next_reg(state);
            fprintf(state->output, "    %%r%d =w copy %lld\n", reg, node->data.num_int.value);
            return reg;
        }
        case AST_NUM_FLOAT: {
            int reg = next_reg(state);
            fprintf(state->output, "    %%r%d =d copy d_%g\n", reg, node->data.num_float.value);
            return reg;
        }
        case AST_STRING: {
            int label = next_label(state);
            int reg = next_reg(state);
            fprintf(state->output, "    %%r%d =l loadl $str_ptr%d\n", reg, label);
            if (state->pending_string_count >= state->pending_string_capacity) {
                state->pending_string_capacity *= 2;
                state->pending_strings = realloc(state->pending_strings, 
                    state->pending_string_capacity * sizeof(char*));
            }
            char *str_ptr_marker = malloc(strlen(node->data.string.value) + 50);
            snprintf(str_ptr_marker, strlen(node->data.string.value) + 50, 
                     "STR_PTR:%d:%s", label, node->data.string.value);
            state->pending_strings[state->pending_string_count++] = str_ptr_marker;
            
            return reg;
        }
        case AST_IDENTIFIER: {
            int existing = find_var_reg(state, node->data.identifier.name);
            if (existing >= 0) {
                return existing;
            }
            const char* type = find_var_type(state, node->data.identifier.name);
            if (!type) type = "w";
            if (strcmp(type, "l") == 0) {
                int reg = gen_load_global_var(state, node->data.identifier.name, type);
                return reg;
            } else {
                int reg = gen_load_global_var(state, node->data.identifier.name, type);
                return reg;
            }
        }
        case AST_UNARYOP: {
            int sub_reg = gen_expr(state, node->data.unaryop.expr);
            
            switch (node->data.unaryop.op) {
                case OP_MINUS: {
                    int reg = next_reg(state);
                    if (is_float_expr(node->data.unaryop.expr)) {
                        fprintf(state->output, "    %%r%d =d sub d_0.0, %%r%d\n", reg, sub_reg);
                    } else {
                        fprintf(state->output, "    %%r%d =w sub 0, %%r%d\n", reg, sub_reg);
                    }
                    return reg;
                }
                case OP_PLUS:
                    return sub_reg;
                case OP_ADDRESS: {
                    int reg = next_reg(state);
                    fprintf(state->output, "    %%r%d =l copy $%s\n", reg, 
                           node->data.unaryop.expr->data.identifier.name);
                    return reg;
                }
                case OP_DEREF: {
                    const char* expr_type = infer_node_qbe_type(state, node->data.unaryop.expr);
                    int reg = next_reg(state);
                    if (strcmp(expr_type, "l") == 0) {
                        fprintf(state->output, "    %%r%d =w loadw %%r%d\n", reg, sub_reg);
                    } else {
                        fprintf(state->output, "    %%r%d =%s copy %%r%d\n", reg, expr_type, sub_reg);
                    }
                    return reg;
                }
                default:
                    return sub_reg;
            }
        }
        case AST_INDEX: {
            ASTNode* target = node->data.index.target;
            ASTNode* idx = node->data.index.index;
            if (!target || !idx) return 0;

            if (target->type == AST_IDENTIFIER && idx->type == AST_IDENTIFIER) {
                const char* var = target->data.identifier.name;
                const char* field = idx->data.identifier.name;
                const char* vartype = find_var_type(state, var);
                if (vartype && strncmp(vartype, "struct:", 7) == 0) {
                    const char* sname = vartype + 7;
                    char** fnames = NULL;
                    char** ftypes = NULL;
                    int fcount = 0;
                    if (qbe_get_struct_info(state, sname, &fnames, &ftypes, &fcount)) {
                        int offset = 0;
                        int found = -1;
                        for (int i = 0; i < fcount; i++) {
                            if (strcmp(fnames[i], field) == 0) { found = i; break; }
                            if (strcmp(ftypes[i], "l") == 0 || strcmp(ftypes[i], "d") == 0) offset += 8;
                            else offset += 4; //w
                        }
                        if (found >= 0) {
                            const char* ftype = ftypes[found];
                            if (offset == 0 && strcmp(ftype, "l") == 0) {
                                return gen_load_global_var(state, var, "l");
                            } else {
                                int addr_reg = next_reg(state);
                                fprintf(state->output, "    %%r%d =l add $%s, %d\n", addr_reg, var, offset);
                                int val_reg = next_reg(state);
                                if (strcmp(ftype, "l") == 0) {
                                    fprintf(state->output, "    %%r%d =l loadl %%r%d\n", val_reg, addr_reg);
                                } else if (strcmp(ftype, "d") == 0) {
                                    fprintf(state->output, "    %%r%d =d loadd %%r%d\n", val_reg, addr_reg);
                                } else {
                                    fprintf(state->output, "    %%r%d =w loadw %%r%d\n", val_reg, addr_reg);
                                }
                                return val_reg;
                            }
                        }
                    }
                }
            }
            return 0;// fallback
        }
        /*
        对于不同的运算
        都生成不同的运算指令
        */
        case AST_BINOP: {
            int left_reg = gen_expr(state, node->data.binop.left);
            int right_reg = gen_expr(state, node->data.binop.right);
            int result_reg = next_reg(state);
            const char* left_type = infer_node_qbe_type(state, node->data.binop.left);
            const char* right_type = infer_node_qbe_type(state, node->data.binop.right);
            if (strcmp(left_type, "d") == 0 || strcmp(right_type, "d") == 0) {
                switch (node->data.binop.op) {
                    case OP_ADD:
                        fprintf(state->output, "    %%r%d =d add %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_SUB:
                        fprintf(state->output, "    %%r%d =d sub %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_MUL:
                        fprintf(state->output, "    %%r%d =d mul %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_DIV:
                        fprintf(state->output, "    %%r%d =d div %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_MOD:
                        {
                            int div_result_reg = next_reg(state);
                            int mul_result_reg = next_reg(state);
                            fprintf(state->output, "    %%r%d =d div %%r%d, %%r%d\n", div_result_reg, left_reg, right_reg);
                            fprintf(state->output, "    %%r%d =d mul %%r%d, %%r%d\n", mul_result_reg, div_result_reg, right_reg);
                            fprintf(state->output, "    %%r%d =d sub %%r%d, %%r%d\n", result_reg, left_reg, mul_result_reg);
                            return result_reg;
                        }
                    case OP_LT:
                        fprintf(state->output, "    %%r%d =d csltd %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_LE:
                        fprintf(state->output, "    %%r%d =d csled %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_GT:
                        fprintf(state->output, "    %%r%d =d csgtd %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_GE:
                        fprintf(state->output, "    %%r%d =d csged %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_EQ:
                        fprintf(state->output, "    %%r%d =d ceqd %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_NE:
                        fprintf(state->output, "    %%r%d =d cned %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    default:
                        fprintf(state->output, "    %%r%d =d add %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                }
            }
            else if (strcmp(left_type, "l") == 0 || strcmp(right_type, "l") == 0) {
                switch (node->data.binop.op) {
                    case OP_ADD:
                        fprintf(state->output, "    %%r%d =l add %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_SUB:
                        fprintf(state->output, "    %%r%d =l sub %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_MUL:
                        fprintf(state->output, "    %%r%d =l mul %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_DIV:
                        fprintf(state->output, "    %%r%d =l div %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_MOD:
                        {
                            int div_result_reg = next_reg(state);
                            int mul_result_reg = next_reg(state);
                            fprintf(state->output, "    %%r%d =l div %%r%d, %%r%d\n", div_result_reg, left_reg, right_reg);
                            fprintf(state->output, "    %%r%d =l mul %%r%d, %%r%d\n", mul_result_reg, div_result_reg, right_reg);
                            fprintf(state->output, "    %%r%d =l sub %%r%d, %%r%d\n", result_reg, left_reg, mul_result_reg);
                            return result_reg;
                        }
                    case OP_LT:
                        fprintf(state->output, "    %%r%d =l csltl %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_LE:
                        fprintf(state->output, "    %%r%d =l cslel %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_GT:
                        fprintf(state->output, "    %%r%d =l csgtl %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_GE:
                        fprintf(state->output, "    %%r%d =l csgel %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_EQ:
                        fprintf(state->output, "    %%r%d =l ceql %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_NE:
                        fprintf(state->output, "    %%r%d =l cnel %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    default:
                        // 默认使用加法
                        fprintf(state->output, "    %%r%d =l add %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                }
            }
            // 否则默认为word类型
            else {
                // 生成整型运算指令
                switch (node->data.binop.op) {
                    case OP_ADD:
                        fprintf(state->output, "    %%r%d =w add %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_SUB:
                        fprintf(state->output, "    %%r%d =w sub %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_MUL:
                        fprintf(state->output, "    %%r%d =w mul %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_DIV:
                        fprintf(state->output, "    %%r%d =w div %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_MOD://这里上面讲过了
                        {
                            int div_result_reg = next_reg(state);
                            int mul_result_reg = next_reg(state);
                            fprintf(state->output, "    %%r%d =w div %%r%d, %%r%d\n", div_result_reg, left_reg, right_reg);
                            fprintf(state->output, "    %%r%d =w mul %%r%d, %%r%d\n", mul_result_reg, div_result_reg, right_reg);
                            fprintf(state->output, "    %%r%d =w sub %%r%d, %%r%d\n", result_reg, left_reg, mul_result_reg);
                            return result_reg;
                        }
                    case OP_LT:
                        fprintf(state->output, "    %%r%d =w csltw %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_LE:
                        fprintf(state->output, "    %%r%d =w cslew %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_GT:
                        fprintf(state->output, "    %%r%d =w csgtw %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_GE:
                        fprintf(state->output, "    %%r%d =w csgew %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_EQ:
                        fprintf(state->output, "    %%r%d =w ceqw %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    case OP_NE:
                        fprintf(state->output, "    %%r%d =w cnew %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                    default:
                        fprintf(state->output, "    %%r%d =w add %%r%d, %%r%d\n", result_reg, left_reg, right_reg);
                        break;
                }
            }
            
            return result_reg;
        }
        case AST_CALL: {
            int result_reg = next_reg(state);
            const char* fname = node->data.call.func->data.identifier.name;

            int argc = 0;
            int *arg_regs = NULL;
            char **arg_types = NULL;

            if (node->data.call.args && node->data.call.args->type == AST_EXPRESSION_LIST) {
                argc = node->data.call.args->data.expression_list.expression_count;
                arg_regs = malloc(sizeof(int) * argc);
                arg_types = malloc(sizeof(char*) * argc);
                for (int i = 0; i < argc; i++) {
                    ASTNode* a = node->data.call.args->data.expression_list.expressions[i];
                    int reg = gen_expr(state, a);
                    arg_regs[i] = reg;
                    const char* at = infer_node_qbe_type(state, a);
                    arg_types[i] = strdup(at);
                }
            }
            const char* call_ret = find_function_ret_type(state, fname);
            if (!call_ret) call_ret = "w";
            fprintf(state->output, "    %%r%d =%s call $%s(", result_reg, call_ret, fname);
            for (int i = 0; i < argc; i++) {
                if (i > 0) fprintf(state->output, ",");
                fprintf(state->output, " %s %%r%d", arg_types[i], arg_regs[i]);
                free(arg_types[i]);
            }
            fprintf(state->output, ")\n");
            free(arg_regs);
            free(arg_types);
            return result_reg;
        }
        default:
            return 0;
    }
}
void gen_expr_list(QbeGenState* state, ASTNode* node) {
    if (!node || node->type != AST_EXPRESSION_LIST) return;
    for (int i = 0; i < node->data.expression_list.expression_count; i++) {
        gen_expr(state, node->data.expression_list.expressions[i]);
    }
}//expr