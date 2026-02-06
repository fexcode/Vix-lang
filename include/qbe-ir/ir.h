#ifndef QBE_IR_H
#define QBE_IR_H

#include <stdio.h>
#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct QbeGenState {
	FILE* output;
	int reg_counter;
	int label_counter;
	char **var_names;
	int *var_regs;
	int var_count;
	int var_capacity;
	char **var_types;
	char* string_value;
	int used_int_print;
	int used_string_print;
	char **global_vars;
	char **global_var_types;
	int global_var_count;
	int global_var_capacity;
	int func_has_return;
	char **func_names;
	char **func_ret_types;
	int func_count;
	int func_capacity;
	char *current_ret_type;
	char **pending_strings;
	int pending_string_count;
	int pending_string_capacity;
	char **ptr_vars;
	int ptr_var_count;
	int ptr_var_capacity;
	/* Struct registry */
	char **struct_names;
	char ***struct_field_names;
	char ***struct_field_types;
	int *struct_field_counts;
	int struct_count;
	int struct_capacity;
	/* Pending struct definitions for output at correct location */
	char **pending_struct_defs;
	int pending_struct_defs_count;
	int pending_struct_defs_capacity;
} QbeGenState;

void ir_gen(ASTNode* ast, FILE* fp);

#ifdef __cplusplus
}
#endif

#endif // QBE_IR_H