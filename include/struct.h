#ifndef QBE_STRUCT_H
#define QBE_STRUCT_H

#include "qbe-ir/ir.h"

#ifdef __cplusplus
extern "C" {
#endif
void qbe_register_struct_def(QbeGenState* state, const char* name, char** field_names, char** field_types, int field_count);
void qbe_emit_struct_def(QbeGenState* state, struct ASTNode* node);
int qbe_get_struct_info(QbeGenState* state, const char* name, char*** out_field_names, char*** out_field_types, int* out_count);

#ifdef __cplusplus
}
#endif
#endif // QBE_STRUCT_H
