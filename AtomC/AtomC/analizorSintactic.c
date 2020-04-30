#include "main.h"

// This file contains ANALIZORUL SINTACTIC | ANALIZA DE DOMENIU | ANALIZA DE TIPURI

// ANALIZOR SINTACTIC DESCENDENT RECURSIV (ASDR)

Token* consumedTk;
Token* crtTk;

int consume(int code)
{
    if (crtTk->code == code) {
        consumedTk = crtTk;
        crtTk = crtTk->next;
        return 1;
    }
    return 0;
}

// ANALIZA DE DOMENIU

int crtDepth = 0;   // ("adancimea" contextului curent, initial 0)
Symbols symbols;
Symbol* crtStruct;
Symbol* crtFunc;

void initSymbols(Symbols* symbols)
{
    symbols->begin = NULL;
    symbols->end = NULL;
    symbols->after = NULL;
}

Symbol* addSymbol(Symbols* symbols, const char* name, int cls)
{
    Symbol* s;
    if (symbols->end == symbols->after)    // create more room
    {
        int count = symbols->after - symbols->begin;
        int n = count * 2; // double the room
        if (n == 0)
            n = 1; // needed for the initial case
        symbols->begin = (Symbol**)realloc(symbols->begin, n * sizeof(Symbol*));
        if (symbols->begin == NULL)
            err("Not enough memory!");
        symbols->end = symbols->begin + count;
        symbols->after = symbols->begin + n;
    }
    SAFEALLOC(s, Symbol)
        * symbols->end++ = s;
    s->name = name;
    s->cls = cls;
    s->depth = crtDepth;
    return s;
}

Symbol* findSymbol(Symbols* symbols, const char* name)  // Find a symbol with given name in specified Symbols Table
{
    int index = (symbols->end) - (symbols->begin) - 1;
    for (int i = index; i >= 0; i--)
    {
        if (strcmp(symbols->begin[i]->name, name) == 0)
            return symbols->begin[i];
    }
    return NULL;
}

void deleteSymbolsAfter(Symbols* symbols, Symbol* symbolPointer)    // Delete all symbols after symbol given in symbolPointer
{
    int index = 0;
    for (Symbol** i = symbols->begin; i != symbols->end; i++, index++)
    {
        if (i == &symbolPointer)  // Find symbolPointer in Symbols Table
        {
            i++;
            for (Symbol** j = i; j != symbols->end; j++)
            {
                free(j);    // Delete symbols after symbolPointer
            }
            symbols->end = symbols->begin + index + 1;
        }
    }
}

void addVar(Token* tkName, Type* t)
{
    Symbol* s;
    if (crtStruct) {
        if (findSymbol(&crtStruct->members, tkName->text))
            tkerr(crtTk, "symbol redefinition: %s", tkName->text);
        s = addSymbol(&crtStruct->members, tkName->text, CLS_VAR);
    }
    else if (crtFunc) {
        s = findSymbol(&symbols, tkName->text);
        if (s && s->depth == crtDepth)
            tkerr(crtTk, "symbol redefinition: %s", tkName->text);
        s = addSymbol(&symbols, tkName->text, CLS_VAR);
        s->mem = MEM_LOCAL;
    }
    else {
        if (findSymbol(&symbols, tkName->text))
            tkerr(crtTk, "symbol redefinition: %s", tkName->text);
        s = addSymbol(&symbols, tkName->text, CLS_VAR);
        s->mem = MEM_GLOBAL;
    }
    s->type = *t;
}

// ANALIZA TIPURILOR

typedef union {
    long int i; // int, char
    double d; // double
    const char* str; // char[]
}CtVal;

typedef struct {
    Type type; // type of the result
    int isLVal; // if it is a LVal
    int isCtVal; // if it is a constant value (int, real, char, char[])
    CtVal ctVal; // the constat value
}RetVal;

Type createType(int typeBase, int nElements)    // Function for creating a new type
{
    Type t;
    t.typeBase = typeBase;
    t.nElements = nElements;
    return t;
}

void cast(Type* dst, Type* src)
{
    if (src->nElements > -1) {
        if (dst->nElements > -1) {
            if (src->typeBase != dst->typeBase)
                tkerr(crtTk, "an array cannot be converted to an array of another type");
        }
        else {
            tkerr(crtTk, "an array cannot be converted to a non-array");
        }
    }
    else {
        if (dst->nElements > -1) {
            tkerr(crtTk, "a non-array cannot be converted to an array");
        }
    }
    switch (src->typeBase) {
    case TB_CHAR:
    case TB_INT:
    case TB_DOUBLE:
        switch (dst->typeBase) {
        case TB_CHAR:
        case TB_INT:
        case TB_DOUBLE:
            return;
        }
    case TB_STRUCT:
        if (dst->typeBase == TB_STRUCT) {
            if (src->s != dst->s)
                tkerr(crtTk, "a structure cannot be converted to another one");
            return;
        }
    }
    tkerr(crtTk, "incompatible types");
}

Type getArithType(Type* t1, Type* t2)
{
    Type typet1 = createType(t1->typeBase, t1->nElements);
    Type typet2 = createType(t2->typeBase, t2->nElements);
    switch (t1->typeBase) {
    case TB_CHAR:
        switch (t2->typeBase) {
        case TB_CHAR:
        case TB_INT:
        case TB_DOUBLE:
            return typet2;
        }
    case TB_INT:
        switch (t2->typeBase) {
        case TB_CHAR:
            return typet1;
            break;
        case TB_INT:
        case TB_DOUBLE:
            return typet2;
        }
    case TB_DOUBLE:
        switch (t2->typeBase) {
        case TB_CHAR:
        case TB_INT:
        case TB_DOUBLE:
            return typet1;
        }
    }
}

Symbol* addExtFunc(const char* name, Type type)
{
    Symbol* s = addSymbol(&symbols, name, CLS_EXTFUNC);
    s->type = type;
    initSymbols(&s->args);
    return s;
}

Symbol* addFuncArg(Symbol* func, const char* name, Type type)
{
    Symbol* a = addSymbol(&func->args, name, CLS_VAR);
    a->type = type;
    return a;
}

void addExtFuncs()
{
    Symbol* s;
    // void put_s(char s[]) - Afiseaza sirul de caractere dat
    s = addExtFunc("put_s", createType(TB_VOID, -1));
    addFuncArg(s, "s", createType(TB_CHAR, 0));

    // void get_s(char s[]) - Cere de la tastatura un sir de caractere si il depune in s
    s = addExtFunc("get_s", createType(TB_VOID, -1));
    addFuncArg(s, "s", createType(TB_CHAR, 0));

    // void put_i(int i) - Afiseaza intregul �i�
    s = addExtFunc("put_i", createType(TB_VOID, -1));
    addFuncArg(s, "s", createType(TB_INT, -1));

    // int get_i() - Cere de la tastatura un intreg
    s = addExtFunc("get_i", createType(TB_INT, -1));

    // void put_d(double d) - Afiseaza numarul real �d�
    s = addExtFunc("put_d", createType(TB_VOID, -1));
    addFuncArg(s, "d", createType(TB_DOUBLE, -1));

    // double get_d() - Cere de la tastatura un numar real
    s = addExtFunc("get_d", createType(TB_DOUBLE, -1));

    // void put_c(char c) - Afiseaza caracterul �c�
    s = addExtFunc("put_c", createType(TB_VOID, -1));
    addFuncArg(s, "c", createType(TB_CHAR, -1));

    // char get_c() - Cere de la tastatura un caracter
    s = addExtFunc("get_c", createType(TB_CHAR, -1));

    // double seconds() - Returneaza un numar (posibil zecimal pentru precizie mai buna) de secunde.
                       // Nu se specifica de cand se calculeaza acest numar (poate fi de la inceputul rularii programului, de la 1 / 1 / 1970, ...)
    s = addExtFunc("seconds", createType(TB_DOUBLE, -1));
}

// IMPLEMENTAREA PREDICATELOR

// unit: ( declStruct | declFunc | declVar )* END ;
int unit()
{
    Token* startTk = crtTk;

    for (;;)
    {
        if (declStruct()) {
        }
        else if (declFunc()) {
        }
        else if (declVar()) {
        }
        else break;
    }
    if (consume(END)) {
        return 1;
    }
    crtTk = startTk;
    return 0;
}

// declStruct: STRUCT ID LACC declVar* RACC SEMICOLON
int declStruct()
{
    Token* startTk = crtTk;
    if (consume(STRUCT))
    {
        if (consume(ID))
        {
            Token* tkName = consumedTk;
            if (consume(LACC))
            {
                if (findSymbol(&symbols, tkName->text))
                    tkerr(crtTk, "symbol redefinition: %s", tkName->text);
                crtStruct = addSymbol(&symbols, tkName->text, CLS_STRUCT);
                initSymbols(&crtStruct->members);
                for (;;)
                {
                    if (declVar()) {
                    }
                    else break;
                }
                if (consume(RACC))
                {
                    if (consume(SEMICOLON))
                    {
                        crtStruct = NULL;
                        return 1;
                    }
                    else tkerr(crtTk, "missing ';' after '}' for STRUCT");
                }
                else tkerr(crtTk, "missing '}' for STRUCT");
            }
        }
    }
    crtTk = startTk;
    return 0;
}

//declVar:  typeBase ID arrayDecl? ( COMMA ID arrayDecl? )* SEMICOLON
int declVar()
{
    Token* startTk = crtTk;
    int isDV;
    Type t;

    if (typeBase(&t))
    {
        if (consume(ID))
        {
            Token* tkName = consumedTk;
            isDV = arrayDecl(&t);
            if (!isDV)
            {
                t.nElements = -1;
            }
            addVar(tkName, &t);
            for (;;)
            {
                if (consume(COMMA))
                {
                    isDV = 1;
                    if (consume(ID))
                    {
                        tkName = consumedTk;
                        if (!arrayDecl(&t))
                        {
                            t.nElements = -1;
                        }
                    }
                    else tkerr(crtTk, "expected variable name after ','");
                }
                else break;
                addVar(tkName, &t);
            }
            if (consume(SEMICOLON))
            {
                return 1;
            }
            else
            {
                if (isDV)        // folosim variabila isDV pentru a returna o eroare doar atunci cand avem o declarare de variabila
                    tkerr(crtTk, "missing ';' after declaring variable");
                else
                    tkerr(crtTk, "expected '=', ',', ';' or array declaration");    // in caz ca se doreste declararea unei functii
            }
        }
        else tkerr(crtTk, "missing or invalid ID");
    }
    crtTk = startTk;
    return 0;
}

// typeBase: INT | DOUBLE | CHAR | STRUCT ID ;
int typeBase(Type* ret)
{
    Token* startTk = crtTk;
    if (consume(INT)) {
        ret->typeBase = TB_INT;
        return 1;
    }
    else if (consume(DOUBLE))
    {
        ret->typeBase = TB_DOUBLE;
        return 1;
    }
    else if (consume(CHAR))
    {
        ret->typeBase = TB_CHAR;
        return 1;
    }
    else if (consume(STRUCT)) {
        if (consume(ID)) {
            Token* tkName = consumedTk;
            Symbol* s = findSymbol(&symbols, tkName->text);
            if (s == NULL)tkerr(crtTk, "undefined symbol: %s", tkName->text);
            if (s->cls != CLS_STRUCT)tkerr(crtTk, "%s is not a struct", tkName->text);
            ret->typeBase = TB_STRUCT;
            ret->s = s;
            return 1;
        }
        else tkerr(crtTk, "missing struct name after STRUCT");
        crtTk = startTk;
    }
    return 0;
}

// arrayDecl: LBRACKET expr? RBRACKET ;
int arrayDecl(Type* ret)
{
    Token* startTk = crtTk;
    RetVal rv;
    if (consume(LBRACKET)) {
        if (expr(&rv))  // if an expression, get its value
        {
            if (!rv.isCtVal)
                tkerr(crtTk, "the array size is not a constant");
            if (rv.type.typeBase != TB_INT)
                tkerr(crtTk, "the array size is not an integer");
            ret->nElements = rv.ctVal.i;
        }
        else
        {
            ret->nElements = 0; /*array without given size*/
        }
        if (consume(RBRACKET))
        {
            return 1;
        }
        else tkerr(crtTk, "invalid expression or missing ']'");
    }
    crtTk = startTk;
    return 0;
}

// typeName: typeBase arrayDecl? ;
int typeName(Type* ret)
{
    if (typeBase(ret))
    {
        if (!arrayDecl(ret))
        {
            ret->nElements = -1;
        }
        return 1;
    }
    return 0;
}

/* declFuncPrim: ID
                 LPAR ( funcArg ( COMMA funcArg )* )? RPAR
                 stmCompound ; */
int declFuncPrim(Type* t)
{

    if (consume(ID))
    {
        Token* tkName = consumedTk;
        if (consume(LPAR))
        {
            if (findSymbol(&symbols, tkName->text))
                tkerr(crtTk, "symbol redefinition: %s", tkName->text);
            crtFunc = addSymbol(&symbols, tkName->text, CLS_FUNC);
            initSymbols(&crtFunc->args);
            crtFunc->type = *t;
            crtDepth++;
            if (funcArg())
            {
                for (;;)
                {
                    if (consume(COMMA))
                    {
                        if (funcArg()) {
                        }
                        else tkerr(crtTk, "expected function argument after ','");
                    }
                    else break;
                }
            }
            if (consume(RPAR))
            {
                crtDepth--;
                if (stmCompound())
                {
                    deleteSymbolsAfter(&symbols, crtFunc);  // se elimina simbolurile adaugate din functie
                    crtFunc = NULL;
                    return 1;
                }
            }
            else tkerr(crtTk, "invalid function arguments or missing ')'");
        }
        else return 0;        // pentru a putea trece si in cazul declVar()
    }
    else return 0;            // pentru a putea trece si in cazul declVar()
    return 1;
}

/* declFunc: ( typeBase MUL? | VOID ) ID
                    LPAR ( funcArg ( COMMA funcArg )* )? RPAR
                    stmCompound ; */
int declFunc()
{
    Token* startTk = crtTk;
    Type t;
    if (typeBase(&t))
    {
        if (consume(MUL))
        {
            t.nElements = 0;
        }
        else
        {
            t.nElements = -1;
        }
        if (declFuncPrim(&t))
        {
            return 1;
        }
    }
    else if (consume(VOID))
    {
        t.typeBase = TB_VOID;
        if (declFuncPrim(&t))
        {
            return 1;
        }
    }
    crtTk = startTk;
    return 0;
}

// funcArg: typeBase ID arrayDecl? ;
int funcArg()
{
    Token* startTk = crtTk;
    Type t;
    if (typeBase(&t))
    {
        if (consume(ID))
        {
            Token* tkName = consumedTk;
            if (!arrayDecl(&t))
            {
                t.nElements = -1;
            }
            Symbol* s = addSymbol(&symbols, tkName->text, CLS_VAR);
            s->mem = MEM_ARG;
            s->type = t;
            s = addSymbol(&crtFunc->args, tkName->text, CLS_VAR);
            s->mem = MEM_ARG;
            s->type = t;
            return 1;
        }
        else tkerr(crtTk, "expected variable name for function argument");
    }
    crtTk = startTk;
    return 0;
}

// IF: LPAR expr RPAR stm ( ELSE stm )? ;
int stmIF()
{
    RetVal rv;
    if (consume(LPAR))
    {
        if (expr(&rv))
        {
            if (rv.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be logically tested");
            if (consume(RPAR))
            {
                if (stm())
                {
                    if (consume(ELSE))
                    {
                        if (stm())
                        {
                            return 1;
                        }
                        else tkerr(crtTk, "missing statement for ELSE");
                    }
                    return 1;
                }
                else tkerr(crtTk, "error in IF body or missing body for IF");
            }
            else tkerr(crtTk, "invalid condition if IF or missing ')' after IF condition");
        }
        else tkerr(crtTk, "invalid condition for IF");
    }
    else tkerr(crtTk, "missing '(' after IF");
    return 1;
}

// WHILE: LPAR expr RPAR stm ;
int stmWHILE()
{
    RetVal rv;
    if (consume(LPAR))
    {
        if (expr(&rv))
        {
            if (rv.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be logically tested");
            if (consume(RPAR))
            {
                if (stm())
                {
                    return 1;
                }
                else tkerr(crtTk, "error in WHILE body or missing body for WHILE");
            }
            else tkerr(crtTk, "invalid condition in WHILE or missing ')' after WHILE condition");
        }
        else tkerr(crtTk, "invalid condition for WHILE");
    }
    else tkerr(crtTk, "missing '(' after WHILE");
    return 1;
}

// FOR: LPAR expr? SEMICOLON expr? SEMICOLON expr? RPAR stm ;
int stmFOR()
{
    RetVal rv1;
    RetVal rv2;
    RetVal rv3;
    if (consume(LPAR))
    {
        expr(&rv1);
        if (consume(SEMICOLON))
        {
            if (expr(&rv2))
            {
                if (rv2.type.typeBase == TB_STRUCT)
                    tkerr(crtTk, "a structure cannot be logically tested");
            }
            if (consume(SEMICOLON))
            {
                expr(&rv3);
                if (consume(RPAR))
                {
                    if (stm())
                    {
                        return 1;
                    }
                    else tkerr(crtTk, "error in FOR body or missing body for FOR");
                }
                else tkerr(crtTk, "invalid expressions in FOR or missing ')' for FOR");
            }
            else tkerr(crtTk, "invalid FOR condition or missing ';'");
        }
        else tkerr(crtTk, "invalid FOR initialization or missing ';'");
    }
    else tkerr(crtTk, "missing '(' after FOR");
    return 1;
}

/* stm: stmCompound
        | IF LPAR expr RPAR stm ( ELSE stm )?
        | WHILE LPAR expr RPAR stm
        | FOR LPAR expr? SEMICOLON expr? SEMICOLON expr? RPAR stm
        | BREAK SEMICOLON
        | RETURN expr? SEMICOLON
        | expr? SEMICOLON ; */
int stm()
{
    Token* startTk = crtTk;
    RetVal rv;
    if (stmCompound())
    {
        return 1;
    }
    else if (consume(IF))
    {
        if (stmIF())
        {
            return 1;
        }
    }
    else if (consume(WHILE))
    {
        if (stmWHILE())
        {
            return 1;
        }
    }
    else if (consume(FOR))
    {
        if (stmFOR())
        {
            return 1;
        }
    }
    else if (consume(BREAK))
    {
        if (consume(SEMICOLON))
        {
            return 1;
        }
        else tkerr(crtTk, "missing ; after BREAK");
    }
    else if (consume(RETURN))
    {
        if (expr(&rv))
        {
            if (crtFunc->type.typeBase == TB_VOID)
                tkerr(crtTk, "a void function cannot return a value");
            cast(&crtFunc->type, &rv.type);
        }
        if (consume(SEMICOLON))
        {
            return 1;
        }
        else tkerr(crtTk, "missing ';' after RETURN");
    }
    else if (expr(&rv))
    {
        if (consume(SEMICOLON)) {
            return 1;
        }
        else tkerr(crtTk, "missing ';' before line %d", crtTk->line);
    }
    if (consume(SEMICOLON))
    {
        return 1;
    }
    crtTk = startTk;
    return 0;
}

// stmCompound: LACC ( declVar | stm )* RACC ;
int stmCompound()
{
    Token* startTk = crtTk;
    //Symbol* start = symbols.end[-1];

    if (consume(LACC))
    {
        //crtDepth++;
        for (;;)
        {
            if (declVar()) {
            }
            else if (stm()) {
            }
            else break;
        }
        if (consume(RACC)) {
            //crtDepth--;
            //deleteSymbolsAfter(&symbols, start);
            return 1;
        }
        else tkerr(crtTk, "missing '}'");
    }
    crtTk = startTk;
    return 0;
}

// expr: exprAssign ;
int expr(RetVal* rv)
{
    Token* startTk = crtTk;
    if (exprAssign(rv))
    {
        return 1;
    }
    crtTk = startTk;
    return 0;
}

// exprAssign: exprUnary ASSIGN exprAssign | exprOr ;
int exprAssign(RetVal* rv)
{
    Token* startTk = crtTk;
    RetVal rve;
    if (exprUnary(rv))             // exprUnary() este un subset al lui exprOr()
    {
        if (consume(ASSIGN))
        {
            if (exprAssign(&rve))
            {
                if (!rv->isLVal)tkerr(crtTk, "cannot assign to a non-lval");
                if (rv->type.nElements > -1 || rve.type.nElements > -1)
                    tkerr(crtTk, "the arrays cannot be assigned");
                cast(&rv->type, &rve.type);
                rv->isCtVal = rv->isLVal = 0;
                return 1;
            }
            else tkerr(crtTk, "invalid expression after '='");
        }
        crtTk = startTk;
    }
    if (exprOr(rv))
    {
        return 1;
    }
    crtTk = startTk;
    return 0;
}

// exprOr: exprOr OR exprAnd | exprAnd ;
int exprOrPrim(RetVal* rv)
{
    RetVal rve;
    if (consume(OR))
    {
        if (exprAnd(&rve))
        {
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be logically tested");
            rv->type = createType(TB_INT, -1);
            rv->isCtVal = rv->isLVal = 0;
            if (exprOrPrim(rv)) {
                return 1;
            }
        }
    }
    return 1;
}

int exprOr(RetVal* rv)
{
    Token* startTk = crtTk;
    if (exprAnd(rv))
    {
        if (exprOrPrim(rv))
        {
            return 1;
        }
    }
    crtTk = startTk;
    return 0;
}

// exprAnd: exprAnd AND exprEq | exprEq ;
int exprAndPrim(RetVal* rv)
{
    RetVal rve;
    if (consume(AND))
    {
        if (exprEq(&rve))
        {
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be logically tested");
            rv->type = createType(TB_INT, -1);
            rv->isCtVal = rv->isLVal = 0;
            if (exprAndPrim(rv))
            {
                return 1;
            }
        }
    }
    return 1;
}

int exprAnd(RetVal* rv)
{
    Token* startTk = crtTk;
    if (exprEq(rv))
    {
        if (exprAndPrim(rv))
        {
            return 1;
        }
    }
    crtTk = startTk;
    return 0;
}

// exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel ;
int exprEqPrim(RetVal* rv)
{
    RetVal rve;
    if (consume(EQUAL) || consume(NOTEQ))
    {
        Token* tkop = consumedTk;
        if (exprRel(&rve))
        {
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be compared");
            rv->type = createType(TB_INT, -1);
            rv->isCtVal = rv->isLVal = 0;
            if (exprEqPrim(rv))
            {
                return 1;
            }
        }
    }
    return 1;
}

int exprEq(RetVal* rv)
{
    Token* startTk = crtTk;
    if (exprRel(rv))
    {
        if (exprEqPrim(rv))
        {
            return 1;
        }
    }
    crtTk = startTk;
    return 0;
}

// exprRel: exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd ;
int exprRelPrim(RetVal* rv)
{
    RetVal rve;
    if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ))
    {
        Token* tkop = consumedTk;
        if (exprAdd(&rve))
        {
            if (rv->type.nElements > -1 || rve.type.nElements > -1)
                tkerr(crtTk, "an array cannot be compared");
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be compared");
            rv->type = createType(TB_INT, -1);
            rv->isCtVal = rv->isLVal = 0;
            if (exprRelPrim(rv))
            {
                return 1;
            }
        }
    }
    return 1;
}

int exprRel(RetVal* rv)
{
    Token* startTk = crtTk;
    if (exprAdd(rv))
    {
        if (exprRelPrim(rv))
        {
            return 1;
        }
    }
    crtTk = startTk;
    return 0;
}

// exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul ;
int exprAddPrim(RetVal* rv)
{
    RetVal rve;
    if (consume(ADD) || consume(SUB))
    {
        Token* tkop = consumedTk;
        if (exprMul(&rve))
        {
            if (rv->type.nElements > -1 || rve.type.nElements > -1)
                tkerr(crtTk, "an array cannot be added or subtracted");
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be added or subtracted");
            rv->type = getArithType(&rv->type, &rve.type);
            rv->isCtVal = rv->isLVal = 0;
            if (exprAddPrim(rv))
            {
                return 1;
            }
        }
    }
    return 1;
}

int exprAdd(RetVal* rv)
{
    Token* startTk = crtTk;
    if (exprMul(rv))
    {
        if (exprAddPrim(rv))
        {
            return 1;
        }
    }
    crtTk = startTk;
    return 0;
}

// exprMul: exprMul ( MUL | DIV ) exprCast | exprCast ;
int exprMulPrim(RetVal* rv)
{
    RetVal rve;
    if (consume(MUL) || consume(DIV))
    {
        Token* tkop = consumedTk;
        if (exprCast(&rve))
        {
            if (rv->type.nElements > -1 || rve.type.nElements > -1)
                tkerr(crtTk, "an array cannot be multiplied or divided");
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be multiplied or divided");
            rv->type = getArithType(&rv->type, &rve.type);
            rv->isCtVal = rv->isLVal = 0;
            if (exprMulPrim(rv))
            {
                return 1;
            }
        }
    }
    return 1;
}

int exprMul(RetVal* rv)
{
    Token* startTk = crtTk;
    if (exprCast(rv))
    {
        if (exprMulPrim(rv))
        {
            return 1;
        }
    }
    crtTk = startTk;
    return 0;
}

// exprCast: LPAR typeName RPAR exprCast | exprUnary ;
int exprCast(RetVal* rv)
{
    Token* startTk = crtTk;
    Type t;
    RetVal rve;
    if (consume(LPAR))
    {
        if (typeName(&t))
        {
            if (consume(RPAR))
            {
                if (exprCast(&rve))
                {
                    cast(&t, &rve.type);
                    rv->type = t;
                    rv->isCtVal = rv->isLVal = 0;
                    return 1;
                }
            }
            else tkerr(crtTk, "invalid type casting or missing ')'");
        }
    }
    else if (exprUnary(rv))
    {
        return 1;
    }
    crtTk = startTk;
    return 0;
}

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix ;
int exprUnary(RetVal* rv)
{
    Token* startTk = crtTk;
    if (consume(SUB) || consume(NOT))
    {
        if (exprUnary(rv))
        {
            Token* tkop = consumedTk;
            if (tkop->code == SUB) {
                if (rv->type.nElements >= 0)
                    tkerr(crtTk, "unary '-' cannot be applied to an array");
                if (rv->type.typeBase == TB_STRUCT)
                    tkerr(crtTk, "unary '-' cannot be applied to a struct");
            }
            else {  // NOT
                if (rv->type.typeBase == TB_STRUCT)
                    tkerr(crtTk, "'!' cannot be applied to a struct");
                rv->type = createType(TB_INT, -1);
            }
            rv->isCtVal = rv->isLVal = 0;
            return 1;
        }
        else tkerr(crtTk, "missing unary expression");
    }
    else if (exprPostfix(rv))
    {
        return 1;
    }
    crtTk = startTk;
    return 0;
}

/* exprPostfix: exprPostfix LBRACKET expr RBRACKET
                            | exprPostfix DOT ID
                            | exprPrimary ; */
int exprPostfixPrim(RetVal* rv)
{
    RetVal rve;
    if (consume(LBRACKET))
    {
        if (expr(&rve))
        {
            if (rv->type.nElements < 0)
                tkerr(crtTk, "only an array can be indexed");
            Type typeInt = createType(TB_INT, -1);
            cast(&typeInt, &rve.type);
            rv->type = rv->type;    // <---------- Nu stiu de ce e asa! Cred ca ar trebui rv->type = typeInt
            rv->type.nElements = -1;
            rv->isLVal = 1;
            rv->isCtVal = 0;
            if (consume(RBRACKET))
            {
                if (exprPostfixPrim(rv))
                {
                    return 1;
                }
            }
            else tkerr(crtTk, "invalid expression or missing '}'");
        }
        else tkerr(crtTk, "missing expression after'{'");
    }
    else if (consume(DOT))
    {
        if (consume(ID))
        {
            Token* tkName = consumedTk;
            Symbol* sStruct = rv->type.s;
            Symbol* sMember = findSymbol(&sStruct->members, tkName->text);
            if (!sMember)
                tkerr(crtTk, "struct %s does not have a member %s", sStruct->name, tkName->text);
            rv->type = sMember->type;
            rv->isLVal = 1;
            rv->isCtVal = 0;
            if (exprPostfixPrim(rv))
            {
                return 1;
            }
        }
        else tkerr(crtTk, "invalid or missing ID after '.'");
    }
    return 1;
}

int exprPostfix(RetVal* rv)
{
    Token* startTk = crtTk;
    if (exprPrimary(rv))
    {
        if (exprPostfixPrim(rv))
        {
            return 1;
        }
    }
    crtTk = startTk;
    return 0;
}

/* exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
                | CT_INT
                | CT_REAL
                | CT_CHAR
                | CT_STRING
                | LPAR expr RPAR ; */
int exprPrimary(RetVal* rv)
{
    Token* startTk = crtTk;
    if (consume(ID))
    {
        Token* tkName = consumedTk;
        Symbol* s = findSymbol(&symbols, tkName->text);
        RetVal arg;
        if (!s)
            tkerr(crtTk, "undefined symbol %s", tkName->text);
        rv->type = s->type;
        rv->isCtVal = 0;
        rv->isLVal = 1;
        if (consume(LPAR))
        {
            Symbol** crtDefArg = s->args.begin;
            if (s->cls != CLS_FUNC && s->cls != CLS_EXTFUNC)
                tkerr(crtTk, "call of the non-function %s", tkName->text);
            if (expr(&arg))
            {
                if (crtDefArg == s->args.end)
                    tkerr(crtTk, "too many arguments in call");
                cast(&(*crtDefArg)->type, &arg.type);
                crtDefArg++;
                for (;;)
                {
                    if (consume(COMMA))
                    {
                        if (expr(&arg)) {
                            if (crtDefArg == s->args.end)
                                tkerr(crtTk, "too many arguments in call");
                            cast(&(*crtDefArg)->type, &arg.type);
                            crtDefArg++;
                            return 1;
                        }
                        else tkerr(crtTk, "expected expression after ','");
                    }
                    else break;
                }
            }
            if (consume(RPAR))
            {
                if (crtDefArg != s->args.end)
                    tkerr(crtTk, "too few arguments in call");
                rv->type = s->type;
                rv->isCtVal = rv->isLVal = 0;
                return 1;
            }
            else tkerr(crtTk, "invalid argument or missing ')'");
        }
        else
        {
            if (s->cls == CLS_FUNC || s->cls == CLS_EXTFUNC)
                tkerr(crtTk, "missing call for function %s", tkName->text);
            return 1;
        }
    }
    else if (consume(CT_INT))
    {
        Token* tki = consumedTk;
        rv->type = createType(TB_INT, -1);
        rv->ctVal.i = tki->i;
        rv->isCtVal = 1;
        rv->isLVal = 0;
        return 1;
    }
    else if (consume(CT_REAL))
    {
        Token* tkr = consumedTk;
        rv->type = createType(TB_DOUBLE, -1);
        rv->ctVal.d = tkr->r;
        rv->isCtVal = 1;
        rv->isLVal = 0;
        return 1;
    }
    else if (consume(CT_CHAR))
    {
        Token* tkc = consumedTk;
        rv->type = createType(TB_CHAR, -1);
        rv->ctVal.i = tkc->i;
        rv->isCtVal = 1;
        rv->isLVal = 0;
        return 1;
    }
    else if (consume(CT_STRING))
    {
        Token* tks = consumedTk;
        rv->type = createType(TB_CHAR, 0);
        rv->ctVal.str = tks->text;
        rv->isCtVal = 1;
        rv->isLVal = 0;
        return 1;
    }
    else if (consume(LPAR))
    {
        if (expr(rv))
        {
            if (consume(RPAR))
            {
                return 1;
            }
            else tkerr(crtTk, "invalid expression or missing ')'");
        }
        else tkerr(crtTk, "missing expression after '('");
    }
    crtTk = startTk;
    return 0;
}