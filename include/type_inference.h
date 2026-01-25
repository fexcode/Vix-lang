#ifndef TYPE_INFERENCE_H
#define TYPE_INFERENCE_H
#include "bytecode.h"
#include <stdio.h>
typedef enum {
    TYPE_UNKNOWN,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_LIST
} InferredType;
typedef struct {
    char* name;
    InferredType type;
    InferredType element_type;
} VariableInfo;
typedef struct {
    VariableInfo* variables;
    int count;
    int capacity;
} TypeInferenceContext;
TypeInferenceContext* create_type_inference_context();
void free_type_inference_context(TypeInferenceContext* ctx);
InferredType infer_type(TypeInferenceContext* ctx, ASTNode* node);
InferredType get_variable_type(TypeInferenceContext* ctx, const char* var_name);
void set_variable_type(TypeInferenceContext* ctx, const char* var_name, InferredType type);
void set_variable_list_type(TypeInferenceContext* ctx, const char* var_name, InferredType list_type, InferredType element_type);
const char* type_to_cpp_string(InferredType type);
#endif//TYPE_INFERENCE_H