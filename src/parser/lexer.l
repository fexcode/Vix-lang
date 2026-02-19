%{
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"
#include "../include/ast.h"
#include "../include/compiler.h"
extern const char* current_input_filename;
extern YYSTYPE yylval;

int yycolumn = 1;

char* my_strndup(const char* str, size_t n) {
    size_t len = strlen(str);
    if (n < len) {
        len = n;
    }
    
    char* result = malloc(len + 1);
    if (result) {
        memcpy(result, str, len);
        result[len] = '\0';
    }
    return result;
}
char* process_escape_sequences(const char* input, size_t input_len) {
    char* result = malloc(input_len + 1);
    size_t result_idx = 0;
    for (size_t i = 0; i < input_len; i++) {//对于extern的printf
        if (input[i] == '\\' && i + 1 < input_len) {
            switch (input[i + 1]) {
                case 'n':
                    result[result_idx++] = '\n';
                    i++;
                    break;
                case 't':
                    result[result_idx++] = '\t';
                    i++;
                    break;
                case 'r':
                    result[result_idx++] = '\r';
                    i++;
                    break;
                case '\\':
                    result[result_idx++] = '\\';
                    i++;
                    break;
                case '"':
                    result[result_idx++] = '"';
                    i++;
                    break;
                case '\'':
                    result[result_idx++] = '\'';
                    i++;
                    break;
                case '0':
                    result[result_idx++] = '\0';
                    i++;
                    break;
                default:
                    result[result_idx++] = input[i];
                    break;
            }
        } else {
            result[result_idx++] = input[i];
        }
    }
    
    result[result_idx] = '\0';
    return result;
}
%}

%option noyywrap
%option yylineno
%option nounput
%option noinput
%{
#define UPDATE_COLUMN() \
    do { \
        for (int i = 0; yytext[i] != '\0'; i++) { \
            if (yytext[i] == '\n') { \
                yycolumn = 1; \
            } else { \
                yycolumn++; \
            } \
        } \
    } while(0)

#define GET_FIRST_COLUMN() (yycolumn - yyleng + 1)
%}

%%

[ \t\r\n]           { 
                      for (int i = 0; yytext[i] != '\0'; i++) {
                          if (yytext[i] == '\n') {
                              yycolumn = 1;
                          } else if (yytext[i] == '\t') {
                              yycolumn = ((yycolumn - 1) / 8 + 1) * 8 + 1;
                          } else {
                              yycolumn++;
                          }
                      }
                      } 

"print"             { UPDATE_COLUMN(); yylval.str = strdup(yytext); return PRINT; }
"input"             { UPDATE_COLUMN(); yylval.str = strdup(yytext); return INPUT; }
"toint"             { UPDATE_COLUMN(); yylval.str = strdup(yytext); return TOINT; }
"tofloat"           { UPDATE_COLUMN(); yylval.str = strdup(yytext); return TOFLOAT; }
"string"            { UPDATE_COLUMN(); yylval.str = strdup(yytext); return TYPE_STR; }
"return"            { UPDATE_COLUMN(); yylval.str = strdup(yytext); return RETURN; }
"fn"                { UPDATE_COLUMN(); yylval.str = strdup(yytext); return FN; }
"extern"            { UPDATE_COLUMN(); yylval.str = strdup(yytext); return EXTERN; }
"const"             { UPDATE_COLUMN(); yylval.str = strdup(yytext); return CONST; }
"mut"               { UPDATE_COLUMN(); yylval.str = strdup(yytext); return MUT; }
"->"                { UPDATE_COLUMN(); yylval.str = strdup(yytext); return ARROW; }
"i32"               { UPDATE_COLUMN(); yylval.str = strdup(yytext); return TYPE_I32; }
"i64"               { UPDATE_COLUMN(); yylval.str = strdup(yytext); return TYPE_I64; }
"i8"                { UPDATE_COLUMN(); yylval.str = strdup(yytext); return TYPE_I8; }
"f32"               { UPDATE_COLUMN(); yylval.str = strdup(yytext); return TYPE_F32; }
"f64"               { UPDATE_COLUMN(); yylval.str = strdup(yytext); return TYPE_F64; }
"str"               { UPDATE_COLUMN(); yylval.str = strdup(yytext); return TYPE_STR; }
"void"              { UPDATE_COLUMN(); yylval.str = strdup(yytext); return TYPE_VOID; }
"nil"               { UPDATE_COLUMN(); yylval.str = strdup(yytext); return NIL; }
"if"                { UPDATE_COLUMN(); yylval.str = strdup(yytext); return IF; }
"elif"              { UPDATE_COLUMN(); yylval.str = strdup(yytext); return ELIF; }
"else"              { UPDATE_COLUMN(); yylval.str = strdup(yytext); return ELSE; }
"while"             { UPDATE_COLUMN(); yylval.str = strdup(yytext); return WHILE; }
"break"             { UPDATE_COLUMN(); yylval.str = strdup(yytext); return BREAK; }
"continue"          { UPDATE_COLUMN(); yylval.str = strdup(yytext); return CONTINUE; }
"for"               { UPDATE_COLUMN(); yylval.str = strdup(yytext); return FOR; }
"in"                { UPDATE_COLUMN(); yylval.str = strdup(yytext); return IN; }
"global"           { UPDATE_COLUMN(); yylval.str = strdup(yytext); return GLOBAL; }
"struct"           { UPDATE_COLUMN(); yylval.str = strdup(yytext); return STRUCT; }
"and"              { UPDATE_COLUMN(); yylval.str = strdup(yytext); return AND; }
"or"               { UPDATE_COLUMN(); yylval.str = strdup(yytext); return OR; }
"import"           { UPDATE_COLUMN(); yylval.str = strdup(yytext); return IMPORT; }
"pub"               { UPDATE_COLUMN(); yylval.str = strdup(yytext); return PUB; }

"="                 { UPDATE_COLUMN(); return ASSIGN; }
"+="                { UPDATE_COLUMN(); return PLUS_ASSIGN; }
"-="                { UPDATE_COLUMN(); yylval.str = strdup(yytext); return MINUS_ASSIGN; }
"*="                { UPDATE_COLUMN(); return MULTIPLY_ASSIGN; }
"/="                { UPDATE_COLUMN(); return DIVIDE_ASSIGN; }
"%="                { UPDATE_COLUMN(); return MODULO_ASSIGN; }
"+"                 { UPDATE_COLUMN(); return PLUS; }
"-"                 { UPDATE_COLUMN(); return MINUS; }
"*"                 { UPDATE_COLUMN(); return MULTIPLY; }
"/"                 { UPDATE_COLUMN(); return DIVIDE; }
"%"                 { UPDATE_COLUMN(); return MODULO; }
"**"                { UPDATE_COLUMN(); return POWER; }
"=="                { UPDATE_COLUMN(); return EQ; }
"!="                { UPDATE_COLUMN(); return NE; }
"<"                 { UPDATE_COLUMN(); return LT; }
"<="                { UPDATE_COLUMN(); return LE; }
">"                 { UPDATE_COLUMN(); return GT; }
">="                { UPDATE_COLUMN(); return GE; }

"("                 { UPDATE_COLUMN(); return LPAREN; }
")"                 { UPDATE_COLUMN(); return RPAREN; }
"{"                 { UPDATE_COLUMN(); return LBRACE; }
"}"                 { UPDATE_COLUMN(); return RBRACE; }
"["                 { UPDATE_COLUMN(); return LBRACKET; }
"]"                 { UPDATE_COLUMN(); return RBRACKET; }
"@"                 { UPDATE_COLUMN(); return AT; }
"&"                 { UPDATE_COLUMN(); return AMPERSAND; }

";"                 { UPDATE_COLUMN(); return SEMICOLON; }
","                 { UPDATE_COLUMN(); return COMMA; }
"..."               { UPDATE_COLUMN(); return DOTDOTDOT; }
".."                { UPDATE_COLUMN(); return DOTDOT; }
"."                 { UPDATE_COLUMN(); return DOT; }
":"                 { UPDATE_COLUMN(); return COLON; }

[a-zA-Z_][a-zA-Z0-9_]*!?  {
                        int col = GET_FIRST_COLUMN();
                        yylval.str = strdup(yytext);
                        yylloc.first_line = yylineno;
                        yylloc.first_column = col;
                        yylloc.last_line = yylineno;
                        yylloc.last_column = col + yyleng - 1;
                        return IDENTIFIER;
                      }

[0-9]+              {
                        int col = GET_FIRST_COLUMN();
                        yylval.num_int = atoll(yytext);
                        yylloc.first_line = yylineno;
                        yylloc.first_column = col;
                        yylloc.last_line = yylineno;
                        yylloc.last_column = col + yyleng - 1;
                        return NUMBER_INT;
                      }

[0-9]+"."[0-9]*      {
                        int col = GET_FIRST_COLUMN();
                        yylval.num_float = atof(yytext);
                        yylloc.first_line = yylineno;
                        yylloc.first_column = col;
                        yylloc.last_line = yylineno;
                        yylloc.last_column = col + yyleng - 1;
                        return NUMBER_FLOAT;
                      }

\"([^\"\\]|\\[\"\\nrt0\\'])+\"        {
                        int col = GET_FIRST_COLUMN();
                        char* raw_str = my_strndup(yytext + 1, yyleng - 2);
                        yylval.str = process_escape_sequences(raw_str, strlen(raw_str));
                        free(raw_str);
                        yylloc.first_line = yylineno;
                        yylloc.first_column = col;
                        yylloc.last_line = yylineno;
                        yylloc.last_column = col + yyleng - 1;
                        return STRING;
                      }

\'[^\']{1}\'        {
                        int col = GET_FIRST_COLUMN();
                        yylval.str = my_strndup(yytext + 1, yyleng - 2);
                        yylloc.first_line = yylineno;
                        yylloc.first_column = col;
                        yylloc.last_line = yylineno;
                        yylloc.last_column = col + yyleng - 1;
                        return CHAR_LITERAL;
                      }

"//".*              { UPDATE_COLUMN(); } 

"/"[*]([^*]|[*]+[^*/])*[*]+"/"  { UPDATE_COLUMN(); } 

.                   {
                        char error_msg[256];
                        snprintf(error_msg, sizeof(error_msg), "Unknown: %c", yytext[0]);
                        report_lexical_error_with_location(error_msg, current_input_filename ? current_input_filename : "unknown", yylineno);
                        return ERROR;
                    }

%%

/*
 *wocao tmd 这个lexer也是真tm nb啊，我tm都不知道我是怎么写出来的
 */