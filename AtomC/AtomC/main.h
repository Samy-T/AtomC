#pragma once

/* ---------------------------------------- LIBRARIES ---------------------------------------- */

// Libraries included
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

/* ----------------------------------------- MACROS ----------------------------------------- */

// Macro for memory allocation
#define SAFEALLOC(var, Type) if((var = (Type *)malloc(sizeof(Type))) == NULL) err("Not enough memory!");

// Macro for stack size
#define STACK_SIZE (32*1024)

// Macro for global variables
#define GLOBAL_SIZE (32*1024)

/* ------------------------------------------ ENUMS ------------------------------------------ */

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

/* ---------------------------------------- OPCODES ---------------------------------------- */

enum {
    O_ADD_C, O_ADD_D, O_ADD_I,
    O_AND_A, O_AND_C, O_AND_D, O_AND_I,
    O_CALL, O_CALLEXT,
    O_CAST_C_D, O_CAST_C_I,
    O_CAST_D_C, O_CAST_D_I,
    O_CAST_I_C, O_CAST_I_D,
    O_DIV_C, O_DIV_D, O_DIV_I,
    O_DROP,
    O_ENTER,
    O_EQ_A, O_EQ_C, O_EQ_D, O_EQ_I,
    O_GREATER_C, O_GREATER_D, O_GREATER_I,
    O_GREATEREQ_C, O_GREATEREQ_D, O_GREATEREQ_I,
    O_HALT,
    O_INSERT,
    O_JF_A, O_JF_C, O_JF_D, O_JF_I,
    O_JMP,
    O_JT_A, O_JT_C, O_JT_D, O_JT_I,
    O_LESS_C, O_LESS_D, O_LESS_I,
    O_LESSEQ_C, O_LESSEQ_D, O_LESSEQ_I,
    O_LOAD,
    O_MUL_C, O_MUL_D, O_MUL_I,
    O_NEG_C, O_NEG_D, O_NEG_I,
    O_NOP,
    O_NOT_A, O_NOT_C, O_NOT_D, O_NOT_I,
    O_NOTEQ_A, O_NOTEQ_C, O_NOTEQ_D, O_NOTEQ_I,
    O_OFFSET,
    O_OR_A, O_OR_C, O_OR_D, O_OR_I,
    O_PUSHFPADDR,
    O_PUSHCT_A, O_PUSHCT_C, O_PUSHCT_D, O_PUSHCT_I,
    O_RET,
    O_STORE,
    O_SUB_C, O_SUB_D, O_SUB_I
}; // all opcodes; each one starts with O_

/* ----------------------------------- Token STRUCT ----------------------------------- */

// Structura care defineste un AL
typedef struct _Token
{
    int code; // codul (numele)
    union {
        char* text; // folosit pentru ID, CT_STRING (alocat dinamic)
        int i; // folosit pentru CT_INT, CT_CHAR
        double r;   // folosit pentru CT_REAL
    };
    int line;            // linia din fisierul de intrare
    struct _Token* next; // inlantuire la urmatorul AL
} Token;

struct _Symbol;
typedef struct _Symbol Symbol;

/* ----------------------------------- Type STRUCT ----------------------------------- */

typedef struct {
    int typeBase;           // TB_*
    Symbol* s;              // struct definition for TB_STRUCT
    int nElements;          // >0 array of given size, 0=array without size, <0 non array
}Type;

/* ----------------------------------- Symbols STRUCT ----------------------------------- */

typedef struct {
    Symbol** begin;         // the beginning of the symbols, or NULL
    Symbol** end;           // the position after the last symbol
    Symbol** after;         // the position after the allocated space
}Symbols;

/* ----------------------------------- Symbol STRUCT ----------------------------------- */

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
    union {
        void* addr; // vm: the memory address for global symbols
        int offset; // vm: the stack offset for local symbols
    };
}Symbol;

/* ----------------------------------- Instr STRUCT ----------------------------------- */

typedef struct _Instr {
    int opcode; // O_*
    union {
        int i; // int, char
        double d;
        void* addr;
    }args[2];
    struct _Instr* last, * next; // links to last, next instructions
}Instr;

/* --------------------------------- GLOBAL VARIABLES --------------------------------- */

extern Token* tokens;
extern char* pch;
extern Token* crtTk;
extern Symbols symbols;

extern Symbol* crtStruct;
extern Symbol* crtFunc;

extern Instr* instructions;
extern Instr* lastInstruction;

extern int sizeArgs, offset;
extern Instr* crtLoopEnd;

/* --------------------------- LEXICAL FUNCTION DECLARATIONS --------------------------- */

void err(const char* fmt, ...);
void tkerr(const Token* tk, const char* fmt, ...);
Token* addTk(int code);
int getNextTk();
void showAtoms(Token* tk);
void terminare(Token* tk);


/* --------------------------- SINTAX FUNCTION DECLARATIONS --------------------------- */

int unit();             // unit: ( declStruct | declFunc | declVar )* END ;    
void initSymbols(Symbols* symbols);
void addExtFuncs(); // Add default functions: void put_s(char s[]), void get_s(char s[]), void put_i(int i), int get_i(), void put_d(double d), double get_d(), char get_c(), double seconds()
Symbol* addSymbol(Symbols* symbols, const char* name, int cls);
Symbol* findSymbol(Symbols* symbols, const char* name);  // Find a symbol with given name in specified Symbols Table
Symbol* requireSymbol(Symbols* symbols, const char* name);
void deleteSymbolsAfter(Symbols* symbols, Symbol* symbolPointer);    // Delete all symbols after symbol given in symbolPointer
Type createType(int typeBase, int nElements);
Symbol* addExtFunc(const char* name, Type type, void* addr);
Symbol* addFuncArg(Symbol* func, const char* name, Type type);

/* ----------------------------- MV FUNCTION DECLARATIONS ----------------------------- */

void* allocGlobal(int size);
void pushd(double d);
double popd();
void pushi(int i);
int popi();
void pushc(char c);
char popc();
void pusha(void* a);
void* popa();
Instr* createInstr(int opcode);
void insertInstrAfter(Instr* after, Instr* i);
Instr* addInstr(int opcode);
Instr* addInstrAfter(Instr* after, int opcode);
Instr* addInstrA(int opcode, void* addr);
Instr* addInstrI(int opcode, int val);
Instr* addInstrII(int opcode, int val1, int val2);
void deleteInstructionsAfter(Instr* start);
void run(Instr* IP);
void mvTest();