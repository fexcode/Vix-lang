#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/ast.h"
#include "../include/parser.h"
#include "../include/bytecode.h"
#include "../include/compiler.h"
#include "../include/qbe-ir/ir.h"
#include "../include/vic-ir/mir.h"
#include "../include/llvm_emit.h"
#include "../include/semantic.h"
#include "../include/opt.h"
extern FILE* yyin;
extern ASTNode* root;
void create_lib_files();
void analyze_ast(TypeInferenceContext* ctx, ASTNode* node);
const char* current_input_filename = NULL;

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <input.vix> [-o output_file]\n", argv[0]);
        fprintf(stderr, "       %s <input.vix>  -o <output_file> -kt\n", argv[0]);
        fprintf(stderr, "       %s <input.vix>  -kt (generate C++ code without compiling)\n", argv[0]);
        fprintf(stderr, "       %s <input.vix>  -q <qbe_ir_file>\n", argv[0]);
        fprintf(stderr, "       %s <input.vix>  -ir <vic_ir_file>\n", argv[0]);
        fprintf(stderr, "       %s <input.vic> -ll <cpp_file>\n", argv[0]);
        return 1;
    }
    if (strcmp(argv[1], "init") == 0) {
        create_lib_files();
        return 0;
    }
    
    char* output_filename = NULL;
    char* qbe_ir_filename = NULL;
    char* vic_ir_filename = NULL;
    char* llvm_ir_filename = NULL;
    int is_vbtc_file = 0;
    int is_vic_file = 0;
    int save_cpp_file = 0;
    int keep_cpp_file = 0;
    int generate_qbe_ir = 0;
    int generate_vic_ir = 0;
    int generate_llvm_ir = 0;
    int optimize_vic_ir = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_filename = argv[i + 1];
                save_cpp_file = 1;
                i++; 
            } else {
                fprintf(stderr, "Error: -o option requires a filename\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-q") == 0) {
            if (i + 1 < argc) {
                qbe_ir_filename = argv[i + 1];
                generate_qbe_ir = 1;
                i++;
            } else {
                fprintf(stderr, "Error: -q option requires a filename\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0 || strcmp(argv[i] , "-ver") == 0){
            printf("Vix-lang Compiler 0.1.0_rc_indev (Beta_26.01.01) by:Mincx1203 Copyright(c) 2025-2026\n");
            return 0;
        }
        else if (strcmp(argv[i], "-ir") == 0) {
            if (i + 1 < argc) {
                vic_ir_filename = argv[i + 1];
                generate_vic_ir = 1;
                i++;
            } else {
                fprintf(stderr, "Error: -ir option requires a filename\n");
                return 1;
            }
        
        } else if (strcmp(argv[i], "-llvm") == 0) {
            if (i + 1 < argc) {
                llvm_ir_filename = argv[i + 1];
                generate_llvm_ir = 1;
                i++;
            } else {
                fprintf(stderr, "Error: -llvm option requires a filename\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-ll") == 0) {
            if (i + 1 < argc) {
                llvm_ir_filename = argv[i + 1];
                generate_llvm_ir = 1;
                i++;
            } else {
                fprintf(stderr, "Error: -ll option requires a filename\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-opt") == 0) {
            optimize_vic_ir = 1;
        } else if (strcmp(argv[i], "-kt") == 0) {
            keep_cpp_file = 1;
        }else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            fprintf(stderr, "usage: %s <input.vix> [-o output_file]\n", argv[0]);
            fprintf(stderr, "       %s <input.vix> [-o output_file] [-kt]\n", argv[0]);
            fprintf(stderr, "       %s <input.vix> -q <qbe_ir_file>\n", argv[0]);
            fprintf(stderr, "       %s <input.vix> -ir <vic_ir_file> [-opt]\n", argv[0]);
            fprintf(stderr, "       %s <input.vix> -llvm <llvm_ir_file>\n", argv[0]);
            fprintf(stderr, "       %s <input.vic> -ll <llvm_ir_file>\n", argv[0]);
            return 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            fprintf(stderr, "Usage: %s <input.vix> [-o output_file] [-kt] [-q qbe_file] [-ir vic_file -opt] [-llvm llvm_file] [-ll llvm_file]\n", argv[0]);
            return 1;
        } else {
            is_vbtc_file = strlen(argv[i]) > 5 && strcmp(argv[i] + strlen(argv[i]) - 5, ".vbtc") == 0;
            is_vic_file = strlen(argv[i]) > 4 && strcmp(argv[i] + strlen(argv[i]) - 4, ".vic") == 0;
        }
    }
    FILE* input_file = fopen(argv[1], "r");
    if (!input_file) {
        perror("Failed to open file");
        return 1;
    }
    if (is_vic_file && generate_llvm_ir) {
        char llvm_filename[256];
        if (strstr(llvm_ir_filename, ".ll") == NULL) {
            snprintf(llvm_filename, sizeof(llvm_filename), "%s.ll", llvm_ir_filename);
        } else {
            strcpy(llvm_filename, llvm_ir_filename);
        }
        
        FILE* llvm_file = fopen(llvm_filename, "w");
        if (!llvm_file) {
            fprintf(stderr, "Error: Cannot open LLVM IR file %s for writing\n", llvm_filename);
            fclose(input_file);
            return 1;
        }
        
        llvm_emit_from_vic_fp(input_file, llvm_file);
        fclose(llvm_file);
        fclose(input_file);
        return 0;
    }
    current_input_filename = argv[1];
    load_source_file(argv[1]);
    set_location_with_column(argv[1], 1, 1);
    yyin = input_file;
    
    int result = yyparse();
    
    if (result == 0) {
        int semantic_errors = check_undefined_symbols(root);
        if (semantic_errors > 0) {
            fprintf(stderr, "Error: Found %d semantic error(s)\n", semantic_errors);
            if (root) {
                free_ast(root);
            }
            cleanup_error_handler();
            fclose(input_file);
            return 1;
        }
        SymbolTable* global_table = create_symbol_table(NULL);
        int unused_vars = check_unused_variables(root, global_table);
        destroy_symbol_table(global_table);
        if (unused_vars > 0) {
            fprintf(stderr, "\033[33mFound %d unused variable(s)\033[0m\n", unused_vars);
        }
        if (get_error_count() > 0) {
            fprintf(stderr, "Compilation failed with %d error(s)\n", get_error_count());
            if (root) {
                free_ast(root);
            }
            cleanup_error_handler();
            fclose(input_file);
            return 1;
        }

        ByteCodeGen* gen = create_bytecode_gen();
        generate_bytecode(gen, root);
        if (generate_vic_ir) {
            char vic_filename[256];
            if (strstr(vic_ir_filename, ".vic") == NULL) {
                snprintf(vic_filename, sizeof(vic_filename), "%s.vic", vic_ir_filename);
            } else {
                strcpy(vic_filename, vic_ir_filename);
            }
            FILE* vic_file = fopen(vic_filename, "w");
            if (!vic_file) {
                free_bytecode_gen(gen);
                fclose(input_file);
                return 1;
            }
            if (optimize_vic_ir) {
                char temp_filename[] = "temp.vic";
                FILE* temp_file = fopen(temp_filename, "w");
                if (!temp_file) {
                    fprintf(stderr, "Error: Cannot open output file for writing\n");
                    fclose(vic_file);
                    free_bytecode_gen(gen);
                    fclose(input_file);
                    return 1;
                }
                vic_gen(root, temp_file);
                fclose(temp_file);
                FILE* temp_read = fopen(temp_filename, "r");
                if (!temp_read) {
                    fprintf(stderr, "Error: Cannot open temp file for reading\n");
                    remove(temp_filename);
                    fclose(vic_file);
                    free_bytecode_gen(gen);
                    fclose(input_file);
                    return 1;
                }
                fseek(temp_read, 0, SEEK_END);
                long len = ftell(temp_read);
                fseek(temp_read, 0, SEEK_SET);
                char* vic_code = malloc(len + 1);
                if (vic_code) {
                    fread(vic_code, 1, len, temp_read);
                    vic_code[len] = '\0';
                }
                fclose(temp_read);
                char* optimized_code = optimize_code(vic_code);
                if (optimized_code) {
                    fputs(optimized_code, vic_file);
                    free(optimized_code);
                } else {
                    fprintf(stderr, "Warning: Optimization failed, using original code\n");
                    temp_read = fopen(temp_filename, "r");
                    if (temp_read) {
                        char buffer[1024];
                        while (fgets(buffer, sizeof(buffer), temp_read)) {
                            fputs(buffer, vic_file);
                        }
                        fclose(temp_read);
                    }
                }
                
                free(vic_code);
                remove(temp_filename);
            } else {
                vic_gen(root, vic_file);
            }
            fclose(vic_file);
            free_bytecode_gen(gen);
            if (root) free_ast(root);
            fclose(input_file);
            return 0;
        }
        
        if (generate_llvm_ir) {
            char llvm_filename[256];
            if (strstr(llvm_ir_filename, ".ll") == NULL) {
                snprintf(llvm_filename, sizeof(llvm_filename), "%s.ll", llvm_ir_filename);
            } else {
                strcpy(llvm_filename, llvm_ir_filename);
            }
            FILE* llvm_file = fopen(llvm_filename, "w");
            if (!llvm_file) {
                fprintf(stderr, "Error: Cannot open LLVM IR file %s for writing\n", llvm_filename);
                free_bytecode_gen(gen);
                fclose(input_file);
                return 1;
            }
            FILE* temp_vic = fopen("temp.vic", "w");
            if (temp_vic) {
                vic_gen(root, temp_vic);
                fclose(temp_vic);
                
                FILE* temp_vic_read = fopen("temp.vic", "r");
                if (temp_vic_read) {
                    llvm_emit_from_vic_fp(temp_vic_read, llvm_file);
                    fclose(temp_vic_read);
                }
            }
            fclose(llvm_file);
            remove("temp.vic");
            free_bytecode_gen(gen);
            if (root) {
                free_ast(root);
            }
            fclose(input_file);
            return 0;
        }
        
        if (generate_qbe_ir) {
            char qbe_filename[256];
            if (strstr(qbe_ir_filename, ".ssa") == NULL) {
                snprintf(qbe_filename, sizeof(qbe_filename), "%s.ssa", qbe_ir_filename);
            } else {
                strcpy(qbe_filename, qbe_ir_filename);
            }
            
            FILE* qbe_file = fopen(qbe_filename, "w");
            if (!qbe_file) {
                fprintf(stderr, "Error: Cannot open QBE IR file %s for writing\n", qbe_filename);
                free_bytecode_gen(gen);
                fclose(input_file);
                return 1;
            }
            FILE* temp_vic = fopen("temp.vic", "w");
            if (temp_vic) {
                vic_gen(root, temp_vic);
                fclose(temp_vic);
                FILE* temp_vic_read = fopen("temp.vic", "r");
                if (temp_vic_read){
                    ir_gen(temp_vic_read, qbe_file);
                    fclose(temp_vic_read);
                } else {
                    fprintf(stderr, "Error: Cannot read temporary VIC IR file\n");
                }
            } else {
                fprintf(stderr, "Error: Cannot create temporary VIC IR file\n");
            }
            fclose(qbe_file);
            remove("temp.vic");
            free_bytecode_gen(gen);
            if (root) {
                free_ast(root);
            }
            fclose(input_file);
            return 0;
        }
        if (save_cpp_file && output_filename != NULL) {
            char cpp_filename[256];
            if (strstr(output_filename, ".cpp") == NULL) {
                snprintf(cpp_filename, sizeof(cpp_filename), "%s.cpp", output_filename);
            } else {
                strcpy(cpp_filename, output_filename);
            }
            
            FILE* output_file = fopen(cpp_filename, "w");
            if (!output_file) {
                perror("Failed to create output file");
                free_bytecode_gen(gen);
                fclose(input_file);
                return 1;
            }
            TypeInferenceContext* type_ctx = create_type_inference_context();
            analyze_ast(type_ctx, root);
            compile_ast_to_cpp_with_types(gen, type_ctx, root, output_file);
            free_type_inference_context(type_ctx);
            fclose(output_file);
            if (!keep_cpp_file) {
                char compile_command[512];
                snprintf(compile_command, sizeof(compile_command), "g++ -std=c++2a -O3 -flto %s -o %s -lm", cpp_filename, output_filename);
                int compile_result = system(compile_command);
                if (compile_result == 0) {
                    remove(cpp_filename);
                } else {
                    printf("Failed to compile to executable\n");
                    return 1;
                }
            } else {
                remove(cpp_filename);
            }
        } else if (keep_cpp_file) {
            char temp_cpp_filename[] = "temp.cpp";
            FILE* output_file = fopen(temp_cpp_filename, "w");
            if (!output_file) {
                perror("Failed to create temporary output file");
                free_bytecode_gen(gen);
                fclose(input_file);
                return 1;
            }
            TypeInferenceContext* type_ctx = create_type_inference_context();
            analyze_ast(type_ctx, root);
            compile_ast_to_cpp_with_types(gen, type_ctx, root, output_file);
            free_type_inference_context(type_ctx);
            fclose(output_file);
        }else {
            printf("===========================AST=======================\n");
            print_ast(root, 0);
            printf("\n=========================VIC IR====================\n");
            vic_gen(root, stdout);
            printf("\n=========================LLVM IR===================\n");
            FILE* temp_vic = fopen("temp.vic", "w");
            if (temp_vic) {
                vic_gen(root, temp_vic);
                fclose(temp_vic);
                
                FILE* temp_vic_read = fopen("temp.vic", "r");
                if (temp_vic_read) {
                    llvm_emit_from_vic_fp(temp_vic_read, stdout);
                    fclose(temp_vic_read);
                }
            }
            remove("temp.vic");
        }
        
        free_bytecode_gen(gen);
    } else {
        if (get_error_count() == 0) {
            const char* filename = current_input_filename ? current_input_filename : "unknown";
            report_syntax_error_with_location("parsing failed due to syntax errors", filename, 1);  // 默认行号为1
        }
    }
    
    if (root) {
        free_ast(root);
    }
    cleanup_error_handler();
    fclose(input_file);
    return result;
}


void analyze_ast(TypeInferenceContext* ctx, ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->data.program.statement_count; i++) {
                analyze_ast(ctx, node->data.program.statements[i]);
            }
            break;
            
        case AST_ASSIGN:
            infer_type(ctx, node->data.assign.right);
            if (node->data.assign.left->type == AST_IDENTIFIER) {
                set_variable_type(ctx, node->data.assign.left->data.identifier.name, 
                    infer_type(ctx, node->data.assign.right));
            }
            analyze_ast(ctx, node->data.assign.left);
            analyze_ast(ctx, node->data.assign.right);
            break;
        case AST_CONST:
            infer_type(ctx, node->data.assign.right);
            if (node->data.assign.left->type == AST_IDENTIFIER) {
                set_variable_type(ctx, node->data.assign.left->data.identifier.name,
                    infer_type(ctx, node->data.assign.right));
            }
            analyze_ast(ctx, node->data.assign.left);
            analyze_ast(ctx, node->data.assign.right);
            break;
            
        case AST_PRINT:
            analyze_ast(ctx, node->data.print.expr);
            break;
            
        case AST_BINOP:
            analyze_ast(ctx, node->data.binop.left);
            analyze_ast(ctx, node->data.binop.right);
            break;
            
        case AST_UNARYOP:
            analyze_ast(ctx, node->data.unaryop.expr);
            break;
            
        case AST_IDENTIFIER: {
            InferredType type = get_variable_type(ctx, node->data.identifier.name);
            if (type == TYPE_UNKNOWN) {
                report_undefined_variable_with_location(
                    node->data.identifier.name,
                    current_input_filename ? current_input_filename : "unknown",
                    node->location.first_line
                );
            }
            break;
        }
            
        default:
            break;
    }
}

void create_lib_files() {
#ifdef _WIN32
    system("mkdir lib 2>nul");
#else
    system("mkdir -p lib 2>/dev/null");
#endif
    FILE* vcore_file = fopen("lib/vcore.hpp", "w");
    if (vcore_file) {
        fprintf(vcore_file, "#ifndef VCORE_HPP\n");
        fprintf(vcore_file, "#define VCORE_HPP\n\n");
        fprintf(vcore_file, "#include <iostream>\n");
        fprintf(vcore_file, "#include <string>\n");
        fprintf(vcore_file, "#include <cstring>\n");
        fprintf(vcore_file, "#include <cctype>\n");
        fprintf(vcore_file, "#include <cstdlib>\n");
        fprintf(vcore_file, "#include <cerrno>\n");
        fprintf(vcore_file, "#include <limits>\n");
        fprintf(vcore_file, "#include <stdexcept>\n");
        fprintf(vcore_file, "#include \"vtypes.hpp\"\n\n");
        fprintf(vcore_file, "namespace vcore {\n");
        fprintf(vcore_file, "    template<typename T>\n");
        fprintf(vcore_file, "    inline void print(const T& value) {\n");
        fprintf(vcore_file, "        std::cout << value << '\\n';\n");
        fprintf(vcore_file, "    }\n\n");
        fprintf(vcore_file, "    template<>\n");
        fprintf(vcore_file, "    inline void print<bool>(const bool& value) {\n");
        fprintf(vcore_file, "        std::cout << (value ? \"true\" : \"false\") << '\\n';\n");
        fprintf(vcore_file, "    }\n\n");
        fprintf(vcore_file, "    template<>\n");
        fprintf(vcore_file, "    inline void print<char*>(char* const& value) {\n");
        fprintf(vcore_file, "        if (value == nullptr) {\n");
        fprintf(vcore_file, "            std::cout << \"nullptr\\n\";\n");
        fprintf(vcore_file, "        } else {\n");
        fprintf(vcore_file, "            std::cout << value << '\\n';\n");
        fprintf(vcore_file, "        }\n");
        fprintf(vcore_file, "    }\n\n");
        fprintf(vcore_file, "    template<>\n");
        fprintf(vcore_file, "    inline void print<const char*>(const char* const& value) {\n");
        fprintf(vcore_file, "        if (value == nullptr) {\n");
        fprintf(vcore_file, "            std::cout << \"nullptr\\n\";\n");
        fprintf(vcore_file, "        } else {\n");
        fprintf(vcore_file, "            std::cout << value << '\\n';\n");
        fprintf(vcore_file, "        }\n");
        fprintf(vcore_file, "    }\n\n");
        fprintf(vcore_file, "    template<>\n");
        fprintf(vcore_file, "    inline void print<std::string>(const std::string& value) {\n");
        fprintf(vcore_file, "        std::cout << value << '\\n';\n");
        fprintf(vcore_file, "    }\n");
        fprintf(vcore_file, "    template<typename T, typename... Args>\n");
        fprintf(vcore_file, "    inline void print(const T& first, const Args&... args) {\n");
        fprintf(vcore_file, "        std::cout << first;\n");
        fprintf(vcore_file, "        ((std::cout << ' ' << args), ...);\n");
        fprintf(vcore_file, "        std::cout << '\\n';\n");
        fprintf(vcore_file, "    }\n\n");
        fprintf(vcore_file, "    template<typename T>\n");
        fprintf(vcore_file, "    inline T add(const T& a, const T& b) {\n");
        fprintf(vcore_file, "        return a + b;\n");
        fprintf(vcore_file, "    }\n\n");
        fprintf(vcore_file, "    template<typename T>\n");
        fprintf(vcore_file, "    inline T sub(const T& a, const T& b) {\n");
        fprintf(vcore_file, "        return a - b;\n");
        fprintf(vcore_file, "    }\n\n");
        fprintf(vcore_file, "    template<typename T>\n");
        fprintf(vcore_file, "    inline T mul(const T& a, const T& b) {\n");
        fprintf(vcore_file, "        return a * b;\n");
        fprintf(vcore_file, "    }\n\n");
        fprintf(vcore_file, "    template<typename T>\n");
        fprintf(vcore_file, "    inline T div(const T& a, const T& b) {\n");
        fprintf(vcore_file, "        return a / b;\n");
        fprintf(vcore_file, "    }\n\n");
        fprintf(vcore_file, "    inline long long velox_to_int(const vtypes::VString &s) {\n");
        fprintf(vcore_file, "        const char* p = s.c_str();\n");
        fprintf(vcore_file, "        while (*p && std::isspace((unsigned char)*p)) ++p;\n");
        fprintf(vcore_file, "        const char* q = s.c_str() + s.size();\n");
        fprintf(vcore_file, "        while (q > p && std::isspace((unsigned char)*(q - 1))) --q;\n");
        fprintf(vcore_file, "        std::string trimmed(p, q);\n\n");
        fprintf(vcore_file, "        if (trimmed.empty()) return 0;\n\n");
        fprintf(vcore_file, "        char* endptr = nullptr;\n");
        fprintf(vcore_file, "        errno = 0;\n");
        fprintf(vcore_file, "        long long value = std::strtoll(trimmed.c_str(), &endptr, 10);\n");
        fprintf(vcore_file, "        if (endptr == trimmed.c_str()) {\n");
        fprintf(vcore_file, "            return 0;\n");
        fprintf(vcore_file, "        }\n");
        fprintf(vcore_file, "        if (errno == ERANGE) {\n");
        fprintf(vcore_file, "            if (trimmed.size() > 0 && trimmed[0] == '-') {\n");
        fprintf(vcore_file, "                return std::numeric_limits<long long>::min();\n");
        fprintf(vcore_file, "            } else {\n");
        fprintf(vcore_file, "                return std::numeric_limits<long long>::max();\n");
        fprintf(vcore_file, "            }\n");
        fprintf(vcore_file, "        }\n\n");
        fprintf(vcore_file, "        return value;\n");
        fprintf(vcore_file, "    }\n");
        fprintf(vcore_file, "}\n\n");
        fprintf(vcore_file, "#endif\n");
        fclose(vcore_file);
    }
    
    FILE* vtypes_file = fopen("lib/vtypes.hpp", "w");
    if (vtypes_file) {
        fprintf(vtypes_file, "#ifndef VTYPES_HPP\n");
        fprintf(vtypes_file, "#define VTYPES_HPP\n");
        fprintf(vtypes_file, "#include <string>\n");
        fprintf(vtypes_file, "#include <vector>\n");
        fprintf(vtypes_file, "#include <iostream>\n");
        fprintf(vtypes_file, "#include <stdexcept>\n\n");
        fprintf(vtypes_file, "namespace vtypes {\n\n");
        fprintf(vtypes_file, "class VString : public std::string {\n");
        fprintf(vtypes_file, "public:\n");
        fprintf(vtypes_file, "    using std::string::string;\n");
        fprintf(vtypes_file, "    VString() : std::string() {}\n");
        fprintf(vtypes_file, "    VString(const std::string &s) : std::string(s) {}\n");
        fprintf(vtypes_file, "    VString(const char* s) : std::string(s) {}\n\n");
        fprintf(vtypes_file, "    VString operator+(const VString &other) const {\n");
        fprintf(vtypes_file, "        return VString(static_cast<const std::string&>(*this) + static_cast<const std::string&>(other));\n");
        fprintf(vtypes_file, "    }\n\n");
        fprintf(vtypes_file, "    VString operator+(const std::string &other) const {\n");
        fprintf(vtypes_file, "        return VString(static_cast<const std::string&>(*this) + other);\n");
        fprintf(vtypes_file, "    }\n\n");
        fprintf(vtypes_file, "    friend VString operator+(const std::string &lhs, const VString &rhs) {\n");
        fprintf(vtypes_file, "        return VString(lhs + static_cast<const std::string&>(rhs));\n");
        fprintf(vtypes_file, "    }\n\n");
        fprintf(vtypes_file, "    VString operator*(long long times) const {\n");
        fprintf(vtypes_file, "        if (times < 0) throw std::invalid_argument(\"Multiplier must be non-negative\");\n");
        fprintf(vtypes_file, "        VString result;\n");
        fprintf(vtypes_file, "        result.reserve(this->size() * (size_t)times);\n");
        fprintf(vtypes_file, "        for (long long i = 0; i < times; ++i) result += *this;\n");
        fprintf(vtypes_file, "        return result;\n");
        fprintf(vtypes_file, "    }\n\n");
        fprintf(vtypes_file, "    friend VString operator*(long long times, const VString &s) {\n");
        fprintf(vtypes_file, "        return s * times;\n");
        fprintf(vtypes_file, "    }\n\n");
        fprintf(vtypes_file, "    VString& operator+=(const VString &other) {\n");
        fprintf(vtypes_file, "        static_cast<std::string&>(*this) += static_cast<const std::string&>(other);\n");
        fprintf(vtypes_file, "        return *this;\n");
        fprintf(vtypes_file, "    }\n\n");
        fprintf(vtypes_file, "    VString& operator+=(const std::string &other) {\n");
        fprintf(vtypes_file, "        static_cast<std::string&>(*this) += other;\n");
        fprintf(vtypes_file, "        return *this;\n");
        fprintf(vtypes_file, "    }\n\n");
        fprintf(vtypes_file, "    VString& operator*=(long long times) {\n");
        fprintf(vtypes_file, "        if (times < 0) throw std::invalid_argument(\"Multiplier must be non-negative\");\n");
        fprintf(vtypes_file, "        VString tmp = *this;\n");
        fprintf(vtypes_file, "        this->clear();\n");
        fprintf(vtypes_file, "        for (long long i = 0; i < times; ++i) *this += tmp;\n");
        fprintf(vtypes_file, "        return *this;\n");
        fprintf(vtypes_file, "    }\n\n");
        fprintf(vtypes_file, "    friend std::ostream& operator<<(std::ostream &os, const VString &s) {\n");
        fprintf(vtypes_file, "        os << static_cast<const std::string&>(s);\n");
        fprintf(vtypes_file, "        return os;\n");
        fprintf(vtypes_file, "    }\n");
        fprintf(vtypes_file, "};\n\n");
        fprintf(vtypes_file, "class VList {\n");
        fprintf(vtypes_file, "public:\n");
        fprintf(vtypes_file, "    std::vector<VString> items;\n");
        fprintf(vtypes_file, "    VList() : items() {}\n");
        fprintf(vtypes_file, "    VList(std::initializer_list<VString> init) : items(init) {}\n");
        fprintf(vtypes_file, "    size_t size() const { return items.size(); }\n");
        fprintf(vtypes_file, "    VString operator[](size_t i) const { return items.at(i); }\n");
        fprintf(vtypes_file, "    VString& operator[](size_t i) { return items.at(i); }\n");
        fprintf(vtypes_file, "    friend std::ostream& operator<<(std::ostream &os, const VList &l) {\n");
        fprintf(vtypes_file, "        os << \"[\";\n");
        fprintf(vtypes_file, "        for (size_t i = 0; i < l.items.size(); ++i) { if (i) os << \", \"; os << l.items[i]; }\n");
        fprintf(vtypes_file, "        os << \"]\";\n");
        fprintf(vtypes_file, "        return os;\n");
        fprintf(vtypes_file, "    }\n");
        fprintf(vtypes_file, "};\n\n");
        fprintf(vtypes_file, "} // namespace vtypes\n\n");
        fprintf(vtypes_file, "#endif\n");
        fclose(vtypes_file);
    }
    
    FILE* vconvert_file = fopen("lib/vconvert.hpp", "w");
    if (vconvert_file) {
        fprintf(vconvert_file, "#ifndef VCONVERT_HPP\n");
        fprintf(vconvert_file, "#define VCONVERT_HPP\n");
        fprintf(vconvert_file, "#include <string>\n");
        fprintf(vconvert_file, "#include <cstdlib>\n");
        fprintf(vconvert_file, "#include <typeinfo>\n");
        fprintf(vconvert_file, "#include <stdexcept>\n");
        fprintf(vconvert_file, "#include \"vtypes.hpp\"\n");
        fprintf(vconvert_file, "namespace detail {\n");
        fprintf(vconvert_file, "    inline void reverse_string(char* str, int len) {\n");
        fprintf(vconvert_file, "        int start = 0;\n");
        fprintf(vconvert_file, "        int end = len - 1;\n");
        fprintf(vconvert_file, "        while (start < end) {\n");
        fprintf(vconvert_file, "            char temp = str[start];\n");
        fprintf(vconvert_file, "            str[start] = str[end];\n");
        fprintf(vconvert_file, "            str[end] = temp;\n");
        fprintf(vconvert_file, "            start++;\n");
        fprintf(vconvert_file, "            end--;\n");
        fprintf(vconvert_file, "        }\n");
        fprintf(vconvert_file, "    }\n\n");
        fprintf(vconvert_file, "    inline int int_to_string(long long num, char* str) {\n");
        fprintf(vconvert_file, "        int i = 0;\n");
        fprintf(vconvert_file, "        bool isNegative = false;\n");
        fprintf(vconvert_file, "        \n");
        fprintf(vconvert_file, "        if (num == 0) {\n");
        fprintf(vconvert_file, "            str[i++] = '0';\n");
        fprintf(vconvert_file, "            str[i] = '\\0';\n");
        fprintf(vconvert_file, "            return i-1;\n");
        fprintf(vconvert_file, "        }\n");
        fprintf(vconvert_file, "        \n");
        fprintf(vconvert_file, "        if (num < 0) {\n");
        fprintf(vconvert_file, "            isNegative = true;\n");
        fprintf(vconvert_file, "            num = -num;\n");
        fprintf(vconvert_file, "        }\n");
        fprintf(vconvert_file, "        \n");
        fprintf(vconvert_file, "        while (num != 0) {\n");
        fprintf(vconvert_file, "            int rem = num %% 10;\n");
        fprintf(vconvert_file, "            str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';\n");
        fprintf(vconvert_file, "            num = num/10;\n");
        fprintf(vconvert_file, "        }\n");
        fprintf(vconvert_file, "        \n");
        fprintf(vconvert_file, "        if (isNegative) {\n");
        fprintf(vconvert_file, "            str[i++] = '-';\n");
        fprintf(vconvert_file, "        }\n");
        fprintf(vconvert_file, "        \n");
        fprintf(vconvert_file, "        str[i] = '\\0';\n");
        fprintf(vconvert_file, "        reverse_string(str, i);\n");
        fprintf(vconvert_file, "        return i;\n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    inline int double_to_string(double num, char* str, int precision = 6) {\n");
        fprintf(vconvert_file, "        long long intPart = (long long)num;\n");
        fprintf(vconvert_file, "        double fracPart = num - intPart;\n");
        fprintf(vconvert_file, "        if (fracPart < 0) fracPart = -fracPart;\n");
        fprintf(vconvert_file, "        \n");
        fprintf(vconvert_file, "        int i = 0;\n");
        fprintf(vconvert_file, "        if (intPart == 0) {\n");
        fprintf(vconvert_file, "            str[i++] = '0';\n");
        fprintf(vconvert_file, "        } else {\n");
        fprintf(vconvert_file, "            char intStr[32];\n");
        fprintf(vconvert_file, "            int len = int_to_string(intPart, intStr);\n");
        fprintf(vconvert_file, "            for (int j = 0; j < len; j++) {\n");
        fprintf(vconvert_file, "                str[i++] = intStr[j];\n");
        fprintf(vconvert_file, "            }\n");
        fprintf(vconvert_file, "        }\n");
        fprintf(vconvert_file, "        if (precision > 0) {\n");
        fprintf(vconvert_file, "            str[i++] = '.';\n");
        fprintf(vconvert_file, "            for (int j = 0; j < precision; j++) {\n");
        fprintf(vconvert_file, "                fracPart *= 10;\n");
        fprintf(vconvert_file, "                int digit = (int)fracPart;\n");
        fprintf(vconvert_file, "                str[i++] = '0' + digit;\n");
        fprintf(vconvert_file, "                fracPart -= digit;\n");
        fprintf(vconvert_file, "            }\n");
        fprintf(vconvert_file, "        }\n");
        fprintf(vconvert_file, "        \n");
        fprintf(vconvert_file, "        str[i] = '\\0';\n");
        fprintf(vconvert_file, "        return i;\n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "}\n\n");
        fprintf(vconvert_file, "namespace vconvert {\n");
        fprintf(vconvert_file, "    inline long long to_int(const vtypes::VString& s) { \n");
        fprintf(vconvert_file, "        return std::stoll(static_cast<const std::string&>(s)); \n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    inline long long to_int(const std::string& s) { \n");
        fprintf(vconvert_file, "        return std::stoll(s); \n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    inline long long to_int(long long v) { \n");
        fprintf(vconvert_file, "        return v; \n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    inline long long to_int(int v) { \n");
        fprintf(vconvert_file, "        return v; \n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    inline long long to_int(double v) { \n");
        fprintf(vconvert_file, "        return static_cast<long long>(v); \n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    inline long long to_int_rtti(const vtypes::VString& var) {\n");
        fprintf(vconvert_file, "        return std::stoll(static_cast<const std::string&>(var));\n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    inline long long to_int_rtti(double var) {\n");
        fprintf(vconvert_file, "        return static_cast<long long>(var);\n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    inline long long to_int_rtti(int var) {\n");
        fprintf(vconvert_file, "        return static_cast<long long>(var);\n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    template<typename T>\n");
        fprintf(vconvert_file, "    inline long long to_int_rtti(const T& var) {\n");
        fprintf(vconvert_file, "        const std::type_info& t = typeid(var);\n");
        fprintf(vconvert_file, "        return static_cast<long long>(var);\n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    inline double to_double(const vtypes::VString& s) { \n");
        fprintf(vconvert_file, "        return std::stod(static_cast<const std::string&>(s)); \n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    inline double to_double(const std::string& s) { \n");
        fprintf(vconvert_file, "        return std::stod(s); \n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    inline double to_double(double v) { \n");
        fprintf(vconvert_file, "        return v; \n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    inline double to_double(long long v) { \n");
        fprintf(vconvert_file, "        return static_cast<double>(v); \n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    inline double to_double(int v) { \n");
        fprintf(vconvert_file, "        return static_cast<double>(v); \n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    class Convertible {\n");
        fprintf(vconvert_file, "    public:\n");
        fprintf(vconvert_file, "        virtual ~Convertible() = default;\n");
        fprintf(vconvert_file, "        virtual long long to_int() const = 0;\n");
        fprintf(vconvert_file, "        virtual double to_double() const = 0;\n");
        fprintf(vconvert_file, "        virtual vtypes::VString to_vstring() const = 0;\n");
        fprintf(vconvert_file, "    };\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    class IntConvertible : public Convertible {\n");
        fprintf(vconvert_file, "    private:\n");
        fprintf(vconvert_file, "        long long value;\n");
        fprintf(vconvert_file, "    public:\n");
        fprintf(vconvert_file, "        explicit IntConvertible(long long v) : value(v) {}\n");
        fprintf(vconvert_file, "        \n");
        fprintf(vconvert_file, "        long long to_int() const override {\n");
        fprintf(vconvert_file, "            return value;\n");
        fprintf(vconvert_file, "        }\n");
        fprintf(vconvert_file, "        \n");
        fprintf(vconvert_file, "        double to_double() const override {\n");
        fprintf(vconvert_file, "            return static_cast<double>(value);\n");
        fprintf(vconvert_file, "        }\n");
        fprintf(vconvert_file, "        \n");
        fprintf(vconvert_file, "        vtypes::VString to_vstring() const override {\n");
        fprintf(vconvert_file, "            char buffer[32];\n");
        fprintf(vconvert_file, "            detail::int_to_string(value, buffer);\n");
        fprintf(vconvert_file, "            return vtypes::VString(buffer);\n");
        fprintf(vconvert_file, "        }\n");
        fprintf(vconvert_file, "    };\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    class DoubleConvertible : public Convertible {\n");
        fprintf(vconvert_file, "    private:\n");
        fprintf(vconvert_file, "        double value;\n");
        fprintf(vconvert_file, "    public:\n");
        fprintf(vconvert_file, "        explicit DoubleConvertible(double v) : value(v) {}\n");
        fprintf(vconvert_file, "        \n");
        fprintf(vconvert_file, "        long long to_int() const override {\n");
        fprintf(vconvert_file, "            return static_cast<long long>(value);\n");
        fprintf(vconvert_file, "        }\n");
        fprintf(vconvert_file, "        \n");
        fprintf(vconvert_file, "        double to_double() const override {\n");
        fprintf(vconvert_file, "            return value;\n");
        fprintf(vconvert_file, "        }\n");
        fprintf(vconvert_file, "        \n");
        fprintf(vconvert_file, "        vtypes::VString to_vstring() const override {\n");
        fprintf(vconvert_file, "            char buffer[32];\n");
        fprintf(vconvert_file, "            detail::double_to_string(value, buffer);\n");
        fprintf(vconvert_file, "            return vtypes::VString(buffer);\n");
        fprintf(vconvert_file, "        }\n");
        fprintf(vconvert_file, "    };\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    class StringConvertible : public Convertible {\n");
        fprintf(vconvert_file, "    private:\n");
        fprintf(vconvert_file, "        vtypes::VString value;\n");
        fprintf(vconvert_file, "    public:\n");
        fprintf(vconvert_file, "        explicit StringConvertible(const vtypes::VString& v) : value(v) {}\n");
        fprintf(vconvert_file, "        \n");
        fprintf(vconvert_file, "        long long to_int() const override {\n");
        fprintf(vconvert_file, "            try {\n");
        fprintf(vconvert_file, "                return std::stoll(static_cast<const std::string&>(value));\n");
        fprintf(vconvert_file, "            } catch (...) {\n");
        fprintf(vconvert_file, "                return 0;\n");
        fprintf(vconvert_file, "            }\n");
        fprintf(vconvert_file, "        }\n");
        fprintf(vconvert_file, "        \n");
        fprintf(vconvert_file, "        double to_double() const override {\n");
        fprintf(vconvert_file, "            try {\n");
        fprintf(vconvert_file, "                return std::stod(static_cast<const std::string&>(value));\n");
        fprintf(vconvert_file, "            } catch (...) {\n");
        fprintf(vconvert_file, "                return 0.0;\n");
        fprintf(vconvert_file, "            }\n");
        fprintf(vconvert_file, "        }\n");
        fprintf(vconvert_file, "        \n");
        fprintf(vconvert_file, "        vtypes::VString to_vstring() const override {\n");
        fprintf(vconvert_file, "            return value;\n");
        fprintf(vconvert_file, "        }\n");
        fprintf(vconvert_file, "    };\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    inline vtypes::VString to_vstring(const vtypes::VString& s) { \n");
        fprintf(vconvert_file, "        return s; \n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    inline vtypes::VString to_vstring(const std::string& s) { \n");
        fprintf(vconvert_file, "        return vtypes::VString(s); \n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    inline vtypes::VString to_vstring(long long v) { \n");
        fprintf(vconvert_file, "        char buffer[32];\n");
        fprintf(vconvert_file, "        detail::int_to_string(v, buffer);\n");
        fprintf(vconvert_file, "        return vtypes::VString(buffer);\n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    inline vtypes::VString to_vstring(int v) { \n");
        fprintf(vconvert_file, "        char buffer[32];\n");
        fprintf(vconvert_file, "        detail::int_to_string(v, buffer);\n");
        fprintf(vconvert_file, "        return vtypes::VString(buffer);\n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    inline vtypes::VString to_vstring(double v) { \n");
        fprintf(vconvert_file, "        char buffer[32];\n");
        fprintf(vconvert_file, "        detail::double_to_string(v, buffer);\n");
        fprintf(vconvert_file, "        return vtypes::VString(buffer);\n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    inline vtypes::VString to_vstring(float v) { \n");
        fprintf(vconvert_file, "        char buffer[32];\n");
        fprintf(vconvert_file, "        detail::double_to_string(v, buffer);\n");
        fprintf(vconvert_file, "        return vtypes::VString(buffer);\n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "    \n");
        fprintf(vconvert_file, "    template<typename T> \n");
        fprintf(vconvert_file, "    inline vtypes::VString to_vstring(const T& v) { \n");
        fprintf(vconvert_file, "        char buffer[32];\n");
        fprintf(vconvert_file, "        detail::int_to_string(v, buffer);\n");
        fprintf(vconvert_file, "        return vtypes::VString(buffer);\n");
        fprintf(vconvert_file, "    }\n");
        fprintf(vconvert_file, "}\n\n");
        fprintf(vconvert_file, "#endif\n");
        fclose(vconvert_file);
    }//ai就是好用
}