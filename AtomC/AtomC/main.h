#pragma once

// Libraries included
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

// Macro for memory allocation
#define SAFEALLOC(var, Type) if((var = (Type *)malloc(sizeof(Type))) == NULL) err("Not enough memory!");

//Enums
enum
{
    ID,
    END,
    CT_INT,
    CT_REAL,
    CT_CHAR,
    CT_STRING,
    COMMA,
    SEMICOLON,
    LPAR,
    RPAR,
    LBRACKET,
    RBRACKET,
    LACC,
    RACC,
    ADD,
    SUB,
    MUL,
    DIV,
    DOT,
    AND,
    OR,
    NOT,
    NOTEQ,
    EQUAL,
    ASSIGN,
    LESS,
    LESSEQ,
    GREATER,
    GREATEREQ,
    BREAK,
    CHAR,
    DOUBLE,
    ELSE,
    FOR,
    IF,
    INT,
    RETURN,
    STRUCT,
    VOID,
    WHILE
}; // codurile AL   // is used in analizorLexical.c

enum { TB_INT, TB_DOUBLE, TB_CHAR, TB_STRUCT, TB_VOID };    // is used in analizorSintactic.c
enum { CLS_VAR, CLS_FUNC, CLS_EXTFUNC, CLS_STRUCT };        // is used in analizorSintactic.c
enum { MEM_GLOBAL, MEM_ARG, MEM_LOCAL };                    // is used in analizorSintactic.c

// Structura care defineste un AL
typedef struct _Token
{
    int code; // codul (numele)
    union {
        char* text; // folosit pentru ID, CT_STRING (alocat dinamic)
        long int i; // folosit pentru CT_INT, CT_CHAR
        double r;   // folosit pentru CT_REAL
    };
    int line;            // linia din fisierul de intrare
    struct _Token* next; // inlantuire la urmatorul AL
} Token;

struct _Symbol;
typedef struct _Symbol Symbol;

typedef struct {
    int typeBase;           // TB_*
    Symbol* s;              // struct definition for TB_STRUCT
    int nElements;          // >0 array of given size, 0=array without size, <0 non array
}Type;

typedef struct {
    Symbol** begin;         // the beginning of the symbols, or NULL
    Symbol** end;           // the position after the last symbol
    Symbol** after;         // the position after the allocated space
}Symbols;

typedef struct _Symbol {
    const char* name;       // a reference to the name stored in a token
    int cls;                // CLS_*
    int mem;                // MEM_*
    Type type;
    int depth;              // 0-global, 1-in function, 2... - nested blocks in function
    union {
        Symbols args;       // used only of functions
        Symbols members;    // used only for structs
    };
}Symbol;

extern Token* tokens;
extern char* pch;
extern Token* crtTk;
extern Symbols symbols;

// LEXICAL FUNCTION DECLARATIONS

void err(const char* fmt, ...);
void tkerr(const Token* tk, const char* fmt, ...);
Token* addTk(int code);
int getNextTk();
void showAtoms(Token* tk);
void terminare(Token* tk);


// SINTAX FUNCTION DECLARATIONS

int unit();             // unit: ( declStruct | declFunc | declVar )* END ;    
void initSymbols(Symbols* symbols);
void addExtFunctions(); // Add default functions: void put_s(char s[]), void get_s(char s[]), void put_i(int i), int get_i(), void put_d(double d), double get_d(), char get_c(), double seconds()