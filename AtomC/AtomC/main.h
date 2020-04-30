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

extern Token* tokens;
extern char* pch;

// LEXICAL FUNCTION DECLARATIONS

void err(const char* fmt, ...);
void tkerr(const Token* tk, const char* fmt, ...);
Token* addTk(int code);
int getNextTk();
void showAtoms(Token* tk);
void terminare(Token* tk);