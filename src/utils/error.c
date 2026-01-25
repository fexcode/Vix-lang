#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "../include/compiler.h"
// typedef enum {
//     ERROR_LEVEL_WARNING,
//     ERROR_LEVEL_ERROR,
//     ERROR_LEVEL_FATAL
// } ErrorLevel;
static const char* current_filename = "unknown";
static int current_line = 1;
static int error_count = 0;
static char* source_content = NULL;
static void vreport_error(ErrorLevel level, ErrorType error_type, const char* format, va_list args);
static int warning_count = 0;
static char* get_line_content(int line_number);
static void show_error_context_with_column(int line_number, int column);
static int current_column = 1;
static void show_error_context_with_column_and_length(int line_number, int column, int length);
void set_location(const char* filename, int line) {
    current_filename = filename ? filename : "unknown";
    current_line = line;
    current_column = 1;
}

void set_location_with_column(const char* filename, int line, int column) {
    current_filename = filename ? filename : "unknown";
    current_line = line;
    current_column = column > 0 ? column : 1;
}
void load_source_file(const char* filename) {
    if (!filename) {
        printf("error:cannot open file\n");
        return;
    }

    FILE* file = fopen(filename, "r");
    if (!file) return;
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    source_content = (char*)malloc(file_size + 1);
    if (source_content) {
        fread(source_content, 1, file_size, file);
        source_content[file_size] = '\0';
    }
    
    fclose(file);
}

static char* get_line_content(int line_number) {
    if (!source_content || line_number <= 0) return NULL;
    
    char* start = source_content;
    int current_line = 1;
    while (current_line < line_number && *start) {
        if (*start == '\n') {
            current_line++;
            start++;
        } else {
            start++;
        }
    }
    
    if (current_line != line_number) 
    {
        return NULL;
    }
    char* end = start;
    while (*end && *end != '\n') {
        end++;
    }
    
    int line_len = end - start;
    char* line_content = (char*)malloc(line_len + 1);
    if (line_content) {
        strncpy(line_content, start, line_len);
        line_content[line_len] = '\0';
        if (line_content[line_len - 1] == '\r') {
            line_content[line_len - 1] = '\0';
        }
    }
    
    return line_content;
}
static void show_error_context_with_column(int line_number, int column) {
    show_error_context_with_column_and_length(line_number, column, 1);
}
static void show_error_context_with_column_and_length(int line_number, int column, int length) {
    if (!source_content) return;
    
    char* line_content = get_line_content(line_number);
    if (!line_content) return;
    
    fprintf(stderr, "%d | %s\n", line_number, line_content);
    fprintf(stderr, "  |");
    for (int i = 0; i < column - 1 && line_content[i] != '\0'; i++) {
        if (line_content[i] == '\t') {
            fputc('\t', stderr);
        } else {
            fputc(' ', stderr);
        }
    }
    for (int i = 0; i < length && (column - 1 + i) < strlen(line_content) && line_content[column - 1 + i] != '\0'; i++) {
        fputc('^', stderr);
    }
    fprintf(stderr, "\n");
    
    free(line_content);
}

static void show_error_context(int line_number) {
    show_error_context_with_column_and_length(line_number, 1, 0);
}
int get_error_count() {
    return error_count;
}
int get_warning_count() {
    return warning_count;
}
static void vreport_error(ErrorLevel level, ErrorType error_type, const char* format, va_list args) {
    const char* level_str = "";
    const char* error_type_str = "";
    const char* color_code = "";
    const char* reset_code = "\033[0m";
    switch (level) {
        case ERROR_LEVEL_WARNING:
            level_str = "warning";
            color_code = "\033[33m"; // 黄色
            warning_count++;
            break;
        case ERROR_LEVEL_ERROR:
        case ERROR_LEVEL_FATAL:
            level_str = "error";
            color_code = "\033[31m"; // 红色
            error_count++;
            break;
    }
    switch (error_type) {
        case ERROR_SYNTAX:
            error_type_str = "SyntaxError";
            break;
        case ERROR_LEXICAL:
            error_type_str = "LexicalError";
            break;
        case ERROR_TYPE:
            error_type_str = "TypeError";
            break;
        case ERROR_UNDEFINED:
            error_type_str = "NameError";
            break;
        case ERROR_UNDEFINED_FUNC:
            error_type_str = "NameError";
            break;
        case ERROR_REDEFINITION:
            error_type_str = "RedefinitionError";
            break;
        case ERROR_SEMANTIC:
            error_type_str = "SemanticError";
            break;
        case ERROR_RUNTIME:
            error_type_str = "RuntimeError";
            break;
        case ERROR_WARNING:
            error_type_str = "Warning";
            break;
        default:
            error_type_str = "Error";
            break;
    }

    fprintf(stderr, "%s%s:%d:%d: %s: %s: ", color_code, current_filename, current_line, current_column, level_str, error_type_str);
    vfprintf(stderr, format, args);
    if (error_type == ERROR_SYNTAX) {
        fprintf(stderr, ". Check if all parentheses are properly closed");
    }
    fprintf(stderr, "%s\n", reset_code);

    /* Show source context (line and caret) when available */
    if (source_content && current_line > 0) {
        if (current_column > 0) {
            show_error_context_with_column(current_line, current_column);
        } else {
            show_error_context(current_line);
        }
    }

    /* Provide brief fix suggestions based on error type */
    const char* suggestion = NULL;
    switch (error_type) {
        case ERROR_UNDEFINED:
            suggestion = "Declare the identifier before use or check for typos";
            break;
        case ERROR_UNDEFINED_FUNC:
            suggestion = "Ensure the function is declared and linked, or check its name";
            break;
        case ERROR_REDEFINITION:
            suggestion = "Remove or rename the previous declaration to avoid conflicts";
            break;
        case ERROR_TYPE:
            suggestion = "Check types or add an explicit cast/convert the expression";
            break;
        case ERROR_SYNTAX:
            suggestion = "Check syntax near the indicated location (missing token/parenthesis)";
            break;
        case ERROR_LEXICAL:
            suggestion = "Remove unsupported characters or fix the token spelling";
            break;
        case ERROR_RUNTIME:
            suggestion = "Verify runtime conditions (null/overflow) or add guards";
            break;
        default:
            suggestion = NULL;
            break;
    }

    if (suggestion) {
        fprintf(stderr, "%sFix: %s%s\n", color_code, suggestion, reset_code);
    }
}

void report_simple_error_with_length(ErrorLevel level, ErrorType error_type, const char* msg, int length) {
    const char* level_str = "";
    const char* error_type_str = "";
    const char* color_code = "";
    const char* reset_code = "\033[0m";
    switch (level) {
        case ERROR_LEVEL_WARNING:
            level_str = "warning";
            color_code = "\033[33m"; // 黄色
            warning_count++;
            break;
        case ERROR_LEVEL_ERROR:
        case ERROR_LEVEL_FATAL:
            level_str = "error";
            color_code = "\033[31m"; // 红色
            error_count++;
            break;
    }
    switch (error_type) {
        case ERROR_SYNTAX:
            error_type_str = "SyntaxError";
            break;
        case ERROR_LEXICAL:
            error_type_str = "LexicalError";
            break;
        case ERROR_TYPE:
            error_type_str = "TypeError";
            break;
        case ERROR_UNDEFINED:
            error_type_str = "NameError";
            break;
        case ERROR_UNDEFINED_FUNC:
            error_type_str = "NameError";
            break;
        case ERROR_REDEFINITION:
            error_type_str = "RedefinitionError";
            break;
        case ERROR_SEMANTIC:
            error_type_str = "SemanticError";
            break;
        case ERROR_RUNTIME:
            error_type_str = "RuntimeError";
            break;
        case ERROR_WARNING:
            error_type_str = "Warning";
            break;
        default:
            error_type_str = "Error";
            break;
    }

    fprintf(stderr, "%s%s:%d:%d: %s: %s: %s%s\n", color_code, current_filename, current_line, current_column, level_str, error_type_str, msg, reset_code);
    if (source_content && current_line > 0) {
        if (current_column > 0) {
            show_error_context_with_column_and_length(current_line, current_column, length);
        } else {
            show_error_context(current_line);
        }
    }

    const char* suggestion = NULL;
    switch (error_type) {
        case ERROR_UNDEFINED:
            suggestion = "Declare the identifier before use or check for typos.";
            break;
        case ERROR_REDEFINITION:
            suggestion = "Remove or rename the previous declaration to avoid conflicts.";
            break;
        case ERROR_TYPE:
            suggestion = "Check types or add an explicit cast/convert the expression.";
            break;
        case ERROR_SYNTAX:
            suggestion = "Check syntax near the indicated location (missing token/parenthesis).";
            break;
        default:
            suggestion = NULL;
            break;
    }

    if (suggestion) {
        fprintf(stderr, "%sFix: %s%s\n", color_code, suggestion, reset_code);
    }
}
void report_simple_error(ErrorLevel level, ErrorType error_type, const char* msg) {
    report_simple_error_with_length(level, error_type, msg, 1);
}

void report_warning(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vreport_error(ERROR_LEVEL_WARNING, ERROR_WARNING, format, args);
    va_end(args);
}
void report_unused_variable_warning(const char* variable_name, const char* filename, int line) {
    if (variable_name && filename && line > 0) {
        set_location(filename, line);
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "unused variable '%s'", variable_name);
        report_simple_error(ERROR_LEVEL_WARNING, ERROR_WARNING, buffer);
    }
}
void report_unused_variable_warning_with_location(const char* variable_name, const char* filename, int line, int column) {
    if (variable_name && filename && line > 0) {
        const char* old_filename = current_filename;
        int old_line = current_line;
        int old_column = current_column;
        
        current_filename = filename;
        current_line = line;
        current_column = column;
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "unused variable '%s'", variable_name);
        report_simple_error_with_length(ERROR_LEVEL_WARNING, ERROR_WARNING, buffer, variable_name ? strlen(variable_name) : 1);
        
        current_filename = old_filename;
        current_line = old_line;
        current_column = old_column;
    }
}


void report_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vreport_error(ERROR_LEVEL_ERROR, ERROR_SEMANTIC, format, args);
    va_end(args);
}
void report_fatal_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vreport_error(ERROR_LEVEL_FATAL, ERROR_SEMANTIC, format, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

void internal_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "\033[31mInternal compiler error at %s:%d:%d:\033[0m ", current_filename, current_line, current_column);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    if (source_content && current_line > 0) {
        if (current_column > 0) {
            show_error_context_with_column(current_line, current_column);
        } else {
            show_error_context(current_line);
        }
    }
    va_end(args);
    exit(EXIT_FAILURE);
}

void report_lexical_error_with_location(const char* message, const char* filename, int line) {
    const char* old_filename = current_filename;
    int old_line = current_line;
    
    current_filename = filename ? filename : "unknown";
    current_line = line;
    report_simple_error(ERROR_LEVEL_ERROR, ERROR_LEXICAL, message);
    
    current_filename = old_filename;
    current_line = old_line;
}

void report_syntax_error_with_location(const char* token, const char* filename, int line) {
    const char* old_filename = current_filename;
    int old_line = current_line;
    
    current_filename = filename ? filename : "unknown";
    current_line = line;
    char buffer[256];
    if (token) {
        snprintf(buffer, sizeof(buffer), "unexpected token '%s'", token);
    } else {
        snprintf(buffer, sizeof(buffer), "unexpected token");
    }
    report_simple_error(ERROR_LEVEL_ERROR, ERROR_SYNTAX, buffer);
    
    current_filename = old_filename;
    current_line = old_line;
}
void report_undefined_identifier_with_location(const char* identifier, const char* filename, int line) {
    const char* old_filename = current_filename;
    int old_line = current_line;
    
    current_filename = filename ? filename : "unknown";
    current_line = line;
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "undefined identifier: '%s'", identifier);
    report_simple_error_with_length(ERROR_LEVEL_ERROR, ERROR_UNDEFINED, buffer, identifier ? strlen(identifier) : 1);
    
    current_filename = old_filename;
    current_line = old_line;
}
void report_undefined_identifier_with_location_and_column(const char* identifier, const char* filename, int line, int column) {
    const char* old_filename = current_filename;
    int old_line = current_line;
    int old_column = current_column;
    
    current_filename = filename ? filename : "unknown";
    current_line = line;
    current_column = column;
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "undefined identifier: '%s'", identifier);
    report_simple_error(ERROR_LEVEL_ERROR, ERROR_UNDEFINED, buffer);
    
    current_filename = old_filename;
    current_line = old_line;
    current_column = old_column;
}

void report_undefined_function_with_location_and_column(const char* function_name, const char* filename, int line, int column) {
    const char* old_filename = current_filename;
    int old_line = current_line;
    int old_column = current_column;
    
    current_filename = filename ? filename : "unknown";
    current_line = line;
    current_column = column;
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "call undefined function: '%s'", function_name);
    report_simple_error(ERROR_LEVEL_ERROR, ERROR_UNDEFINED_FUNC, buffer);
    
    current_filename = old_filename;
    current_line = old_line;
    current_column = old_column;
}

void report_undefined_variable_with_location(const char* variable_name, const char* filename, int line) {
    const char* old_filename = current_filename;
    int old_line = current_line;
    
    current_filename = filename ? filename : "unknown";
    current_line = line;
    if (variable_name) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "undefined variable: '%s'", variable_name);
        report_simple_error(ERROR_LEVEL_ERROR, ERROR_UNDEFINED, buffer);
    } else {
        report_simple_error(ERROR_LEVEL_ERROR, ERROR_UNDEFINED, "undefined variable");
    }
    
    current_filename = old_filename;
    current_line = old_line;
}

void report_redefinition_error_with_location(const char* identifier, const char* filename, int line) {
    const char* old_filename = current_filename;
    int old_line = current_line;
    
    current_filename = filename ? filename : "unknown";
    current_line = line;
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "redefinition of '%s'", identifier);
    report_simple_error(ERROR_LEVEL_ERROR, ERROR_REDEFINITION, buffer);
    
    current_filename = old_filename;
    current_line = old_line;
}

void report_type_error_with_location(const char* expected, const char* actual, const char* filename, int line) {
    const char* old_filename = current_filename;
    int old_line = current_line;
    
    current_filename = filename ? filename : "unknown";
    current_line = line;
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "expected %s but got %s", expected, actual);
    report_simple_error(ERROR_LEVEL_ERROR, ERROR_TYPE, buffer);
    
    current_filename = old_filename;
    current_line = old_line;
}

void report_semantic_error_with_location(const char* message, const char* filename, int line) {
    const char* old_filename = current_filename;
    int old_line = current_line;
    
    current_filename = filename ? filename : "unknown";
    current_line = line;
    report_simple_error(ERROR_LEVEL_ERROR, ERROR_SEMANTIC, message);
    
    current_filename = old_filename;
    current_line = old_line;
}
void report_runtime_error_with_location(const char* message, const char* filename, int line) {
    const char* old_filename = current_filename;
    int old_line = current_line;
    
    current_filename = filename ? filename : "unknown";
    current_line = line;
    report_simple_error(ERROR_LEVEL_ERROR, ERROR_RUNTIME, message);
    
    current_filename = old_filename;
    current_line = old_line;
}
void report_syntax_error(const char* token) {
    char buffer[256];
    if (token) {
        snprintf(buffer, sizeof(buffer), "unexpected token '%s'", token);
    } else {
        snprintf(buffer, sizeof(buffer), "unexpected token");
    }
    report_simple_error(ERROR_LEVEL_ERROR, ERROR_SYNTAX, buffer);
}
void report_lexical_error(const char* message) {
    report_simple_error(ERROR_LEVEL_ERROR, ERROR_LEXICAL, message);
}

void report_type_error(const char* expected, const char* actual) {
    if (expected && actual) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "expected %s but got %s", expected, actual);
        report_simple_error(ERROR_LEVEL_ERROR, ERROR_TYPE, buffer);
    } else {
        report_simple_error(ERROR_LEVEL_ERROR, ERROR_TYPE, "type error");
    }
}
void report_undefined_identifier(const char* identifier) {
    if (identifier) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "undefined identifier: '%s'", identifier);
        report_simple_error_with_length(ERROR_LEVEL_ERROR, ERROR_UNDEFINED, buffer, strlen(identifier));
    } else {
        report_simple_error(ERROR_LEVEL_ERROR, ERROR_UNDEFINED, "undefined identifier");
    }
}
void report_undefined_function(const char* function_name) {
    if (function_name) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "call to undefined function: '%s'", function_name);
        report_simple_error_with_length(ERROR_LEVEL_ERROR, ERROR_UNDEFINED_FUNC, buffer, strlen(function_name));
    } else {
        report_simple_error(ERROR_LEVEL_ERROR, ERROR_UNDEFINED_FUNC, "call to undefined function");
    }
}
void report_undefined_variable(const char* variable_name) {
    if (variable_name) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "undefined variable: '%s'", variable_name);
        report_simple_error_with_length(ERROR_LEVEL_ERROR, ERROR_UNDEFINED, buffer, strlen(variable_name));
    } else {
        report_simple_error(ERROR_LEVEL_ERROR, ERROR_UNDEFINED, "undefined variable");
    }
}
void report_redefinition_error(const char* identifier) {
    if (identifier) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "redefinition of '%s'", identifier);
        report_simple_error_with_length(ERROR_LEVEL_ERROR, ERROR_REDEFINITION, buffer, strlen(identifier));
    } else {
        report_simple_error(ERROR_LEVEL_ERROR, ERROR_REDEFINITION, "redefinition of identifier");
    }
}

void report_mismatched_parentheses() {
    report_simple_error(ERROR_LEVEL_ERROR, ERROR_SYNTAX, "mismatched parentheses");
}

void print_error_summary() {
    if (error_count > 0 || warning_count > 0) {
        fprintf(stderr, "\nCompilation finished with ");
        if (error_count > 0) {
            fprintf(stderr, "%d error(s) ", error_count);
            if (warning_count > 0) {
                fprintf(stderr, "and ");
            }
        }
        if (warning_count > 0) {
            fprintf(stderr, "%d warning(s)", warning_count);
        }
        fprintf(stderr, ".\n");
    }
}

void cleanup_error_handler() {
    if (source_content) {
        free(source_content);
        source_content = NULL;
    }
}