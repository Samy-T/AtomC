#include "main.h"

// This file contains ANALIZORUL SINTACTIC | ANALIZA DE DOMENIU | ANALIZA DE TIPURI | GENERAREA DE COD

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

/* ------------------------------------------------- ANALIZA DE DOMENIU ------------------------------------------------- */

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

Symbol* requireSymbol(Symbols* symbols, const char* name)  // Find a symbol with given name in specified Symbols Table
{
    Symbol* s = findSymbol(symbols, name);
    if (s == NULL)
    {
        err("Undefined symbol: %s!", name);
    }
    return s;
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
    if (crtStruct || crtFunc) 
    {
        s->offset = offset;
    }
    else 
    {
        s->addr = allocGlobal(typeFullSize(&s->type));
    }
    offset += typeFullSize(&s->type);
}

/* ------------------------------------------------- ANALIZA TIPURILOR ------------------------------------------------- */

typedef union {
    int i; // int, char
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

Symbol* addExtFunc(const char* name, Type type, void* addr)
{
    Symbol* s = addSymbol(&symbols, name, CLS_EXTFUNC);
    s->type = type;
    s->addr = addr;
    initSymbols(&s->args);
    return s;
}

Symbol* addFuncArg(Symbol* func, const char* name, Type type)
{
    Symbol* a = addSymbol(&func->args, name, CLS_VAR);
    a->type = type;
    return a;
}

/* ------------------------------------------------- PREDEFINED FUNCTIONS ------------------------------------------------- */

void put_s(char s[])    // Afiseaza sirul de caractere dat
{
    printf("%s", s);
}

get_s(char s[])     // Cere de la tastatura un sir de caractere si il depune in s
{
    scanf("%s", s);
}

void put_i()        // Afiseaza intregul i
{
    printf("#%d\n", popi());
}

int get_i()  // Cere de la tastatura un numar intreg
{
    int i=0;
    scanf("%d", &i);
    pushi(i);
    return i;
}

put_d(double d)     // Afiseaza numarul real d
{
    printf("%lf\n", popd());
}

double get_d()      // Cere de la tastatura un numar real
{
    double d;
    scanf("%lf", &d);
    pushd(d);
    return d;
}

void put_c(char c)  // Afiseaza caracterul c
{
    printf("%c\n", popc());
}

char get_c()    // Cere de la tastatura un caracter
{
    char c;
    scanf("%c", &c);
    pushc(c);
    return c;
}

double seconds()
{
    time_t seconds;
    seconds = time(NULL);
    return seconds / 3600;
}

void addExtFuncs()
{
    Symbol* s, * a;
    // void put_s(char s[]) - Afiseaza sirul de caractere dat
    s = addExtFunc("put_s", createType(TB_VOID, -1), put_s);
    addFuncArg(s, "s", createType(TB_CHAR, 0));

    // void get_s(char s[]) - Cere de la tastatura un sir de caractere si il depune in s
    s = addExtFunc("get_s", createType(TB_VOID, -1), get_s);
    addFuncArg(s, "s", createType(TB_CHAR, 0));

    // void put_i(int i) - Afiseaza intregul i
    s = addExtFunc("put_i", createType(TB_VOID, -1), put_i);
    addFuncArg(s, "i", createType(TB_INT, -1));
    // or you can use the two lines below instead addFuncArg(s, "i", createType(TB_INT, -1))
    //a = addSymbol(&s->args, "i", CLS_VAR);
    //a->type = createType(TB_INT, -1);
    
    // int get_i() - Cere de la tastatura un intreg
    s = addExtFunc("get_i", createType(TB_INT, -1), get_i);

    // void put_d(double d) - Afiseaza numarul real d
    s = addExtFunc("put_d", createType(TB_VOID, -1), put_d);
    addFuncArg(s, "d", createType(TB_DOUBLE, -1));

    // double get_d() - Cere de la tastatura un numar real
    s = addExtFunc("get_d", createType(TB_DOUBLE, -1), get_d);

    // void put_c(char c) - Afiseaza caracterul c
    s = addExtFunc("put_c", createType(TB_VOID, -1), put_c);
    addFuncArg(s, "c", createType(TB_CHAR, -1));

    // char get_c() - Cere de la tastatura un caracter
    s = addExtFunc("get_c", createType(TB_CHAR, -1), get_c);

    // double seconds() - Returneaza un numar (posibil zecimal pentru precizie mai buna) de secunde.
                       // Nu se specifica de cand se calculeaza acest numar (poate fi de la inceputul rularii programului, de la 1 / 1 / 1970, ...)
    s = addExtFunc("seconds", createType(TB_DOUBLE, -1), seconds);
}

/* -------------------------------------------- FUNCTIONS FOR CODE GENERATION (COMPILER) -------------------------------------------- */

int sizeArgs, offset;
Instr* crtLoopEnd;

Instr* getRVal(RetVal* rv)
{
    if (rv->isLVal) {
        switch (rv->type.typeBase) {
        case TB_INT:
        case TB_DOUBLE:
        case TB_CHAR:
        case TB_STRUCT:
            addInstrI(O_LOAD, typeArgSize(&rv->type));
            break;
        default:tkerr(crtTk, "unhandled type: %d", rv->type.typeBase);
        }
    }
    return lastInstruction;
}

void addCastInstr(Instr* after, Type* actualType, Type* neededType)
{
    if (actualType->nElements >= 0 || neededType->nElements >= 0)return;
    switch (actualType->typeBase) {
    case TB_CHAR:
        switch (neededType->typeBase) {
        case TB_CHAR:break;
        case TB_INT:addInstrAfter(after, O_CAST_C_I); break;
        case TB_DOUBLE:addInstrAfter(after, O_CAST_C_D); break;
        }
        break;
    case TB_INT:
        switch (neededType->typeBase) {
        case TB_CHAR:addInstrAfter(after, O_CAST_I_C); break;
        case TB_INT:break;
        case TB_DOUBLE:addInstrAfter(after, O_CAST_I_D); break;
        }
        break;
    case TB_DOUBLE:
        switch (neededType->typeBase) {
        case TB_CHAR:addInstrAfter(after, O_CAST_D_C); break;
        case TB_INT:addInstrAfter(after, O_CAST_D_I); break;
        case TB_DOUBLE:break;
        }
        break;
    }
}

Instr* createCondJmp(RetVal* rv)
{
    if (rv->type.nElements >= 0) {  // arrays
        return addInstr(O_JF_A);
    }
    else {                              // non-arrays
        getRVal(rv);
        switch (rv->type.typeBase) {
        case TB_CHAR:return addInstr(O_JF_C);
        case TB_DOUBLE:return addInstr(O_JF_D);
        case TB_INT:return addInstr(O_JF_I);
        default:return NULL;
        }
    }
}

int typeBaseSize(Type* type)
{
    int size = 0;
    Symbol** is;
    switch (type->typeBase) {
    case TB_INT:size = sizeof(int); break;
    case TB_DOUBLE:size = sizeof(double); break;
    case TB_CHAR:size = sizeof(char); break;
    case TB_STRUCT:
        for (is = type->s->members.begin; is != type->s->members.end; is++) {
            size += typeFullSize(&(*is)->type);
        }
        break;
    case TB_VOID:size = 0; break;
    default:err("invalid typeBase: %d", type->typeBase);
    }
    return size;
}
int typeFullSize(Type* type)
{
    return typeBaseSize(type) * (type->nElements > 0 ? type->nElements : 1);
}
int typeArgSize(Type* type)
{
    if (type->nElements >= 0)return sizeof(void*);
    return typeBaseSize(type);
}

Instr* appendInstr(Instr* i)
{
    i->next = NULL;
    i->last = lastInstruction;
    lastInstruction->next = i;
    lastInstruction = i;
    return lastInstruction;
}

/* ------------------------------------------------- IMPLEMENTAREA PREDICATELOR ------------------------------------------------- */

// unit: ( declStruct | declFunc | declVar )* END ;
int unit()
{
    Token* startTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    Instr* labelMain = addInstr(O_CALL);
    addInstr(O_HALT);
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
    labelMain->args[0].addr = requireSymbol(&symbols, "main")->addr;
    if (consume(END)) {
        return 1;
    }
    crtTk = startTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

// declStruct: STRUCT ID LACC declVar* RACC SEMICOLON
int declStruct()
{
    Token* startTk = crtTk;
    Instr* startLastInstr = lastInstruction;
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
                offset = 0;
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
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

//declVar:  typeBase ID arrayDecl? ( COMMA ID arrayDecl? )* SEMICOLON
int declVar()
{
    Token* startTk = crtTk;
    int isDV;
    Type t;
    Instr* startLastInstr = lastInstruction;
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
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

// typeBase: INT | DOUBLE | CHAR | STRUCT ID ;
int typeBase(Type* ret)
{
    Token* startTk = crtTk;
    Instr* startLastInstr = lastInstruction;
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
        deleteInstructionsAfter(startLastInstr);
    }
    return 0;
}

// arrayDecl: LBRACKET expr? RBRACKET ;
int arrayDecl(Type* ret)
{
    Token* startTk = crtTk;
    RetVal rv;
    Instr* startLastInstr = lastInstruction;
    Instr* instrBeforeExpr;
    if (consume(LBRACKET)) {
        instrBeforeExpr = lastInstruction;
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
        deleteInstructionsAfter(instrBeforeExpr);
        if (consume(RBRACKET))
        {
            return 1;
        }
        else tkerr(crtTk, "invalid expression or missing ']'");
    }
    crtTk = startTk;
    deleteInstructionsAfter(startLastInstr);
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
    Symbol** ps;
    if (consume(ID))
    {
        Token* tkName = consumedTk;
        sizeArgs = offset = 0;
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
                crtFunc->addr = addInstr(O_ENTER);
                sizeArgs = offset;
                //update args offsets for correct FP indexing
                for (ps = symbols.begin; ps != symbols.end; ps++) {
                    if ((*ps)->mem == MEM_ARG) {
                        //2*sizeof(void*) == sizeof(retAddr)+sizeof(FP)
                        (*ps)->offset -= sizeArgs + 2 * sizeof(void*);
                    }
                }
                offset = 0;
                if (stmCompound())
                {
                    deleteSymbolsAfter(&symbols, crtFunc);  // se elimina simbolurile adaugate din functie
                    ((Instr*)crtFunc->addr)->args[0].i = offset;  // setup the ENTER argument 
                    if (crtFunc->type.typeBase == TB_VOID) 
                    {
                        addInstrII(O_RET, sizeArgs, 0);
                    }
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
    Instr* startLastInstr = lastInstruction;
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
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

// funcArg: typeBase ID arrayDecl? ;
int funcArg()
{
    Token* startTk = crtTk;
    Type t;
    Instr* startLastInstr = lastInstruction;
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
            //for each "s" (the one as local var and the one as arg):
            s->offset = offset;
            //only once at the end, after "offset" is used and "s->type" is set
            offset += typeArgSize(&s->type);
            return 1;
        }
        else tkerr(crtTk, "expected variable name for function argument");
    }    
    crtTk = startTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

// IF: LPAR expr RPAR stm ( ELSE stm )? ;
int stmIF(Instr* i1, Instr* i2)
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
                i1 = createCondJmp(&rv);
                if (stm())
                {
                    if (consume(ELSE))
                    {
                        i2 = addInstr(O_JMP);
                        if (stm())
                        {   
                            i1->args[0].addr = i2->next;
                            i1 = i2;
                            return 1;
                        }
                        else tkerr(crtTk, "missing statement for ELSE");
                    }
                    i1->args[0].addr = addInstr(O_NOP);
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
int stmWHILE(Instr* i1, Instr* i2)
{
    RetVal rv;
    Instr* oldLoopEnd = crtLoopEnd;
    crtLoopEnd = createInstr(O_NOP);
    i1 = lastInstruction;
    if (consume(LPAR))
    {
        if (expr(&rv))
        {
            if (rv.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be logically tested");
            if (consume(RPAR))
            {
                i2 = createCondJmp(&rv);
                if (stm())
                {
                    addInstrA(O_JMP, i1->next);
                    appendInstr(crtLoopEnd);
                    i2->args[0].addr = crtLoopEnd;
                    crtLoopEnd = oldLoopEnd;
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
int stmFOR(Instr* i2)
{
    RetVal rv1;
    RetVal rv2;
    RetVal rv3;
    Instr* i3, * i4, * is, * ib3, * ibs;
    Instr* oldLoopEnd = crtLoopEnd;
    crtLoopEnd = createInstr(O_NOP);
    if (consume(LPAR))
    {
        if (expr(&rv1))
        {
            if (typeArgSize(&rv1.type))
                addInstrI(O_DROP, typeArgSize(&rv1.type));
        }
        if (consume(SEMICOLON))
        {
            i2 = lastInstruction;
            if (expr(&rv2))
            {   
                i4 = createCondJmp(&rv2);
                if (rv2.type.typeBase == TB_STRUCT)
                    tkerr(crtTk, "a structure cannot be logically tested");
            }
            else
            {
                i4 = NULL;
            }
            if (consume(SEMICOLON))
            {
                ib3 = lastInstruction;
                if (expr(&rv3))
                {
                    if (typeArgSize(&rv3.type))
                        addInstrI(O_DROP, typeArgSize(&rv3.type));
                }
                if (consume(RPAR))
                {
                    ibs = lastInstruction;
                    if (stm())
                    {
                        // if rv3 exists, exchange rv3 code with stm code: rv3 stm -> stm rv3
                        if (ib3 != ibs) {
                            i3 = ib3->next;
                            is = ibs->next;
                            ib3->next = is;
                            is->last = ib3;
                            lastInstruction->next = i3;
                            i3->last = lastInstruction;
                            ibs->next = NULL;
                            lastInstruction = ibs;
                        }
                        addInstrA(O_JMP, i2->next);
                        appendInstr(crtLoopEnd);
                        if (i4)
                        {
                            i4->args[0].addr = crtLoopEnd;
                        }
                        crtLoopEnd = oldLoopEnd;
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
    Instr* startLastInstr = lastInstruction;
    Instr* i, * i1, * i2;
    if (stmCompound())
    {
        return 1;
    }
    else if (consume(IF))
    {
        if (stmIF(&i1, &i2))
        {
            return 1;
        }
    }
    else if (consume(WHILE))
    {
        if (stmWHILE(&i1, &i2))
        {
            return 1;
        }
    }
    else if (consume(FOR))
    {
        if (stmFOR(&i2))
        {
            return 1;
        }
    }
    else if (consume(BREAK))
    {
        if (consume(SEMICOLON))
        {
            if (!crtLoopEnd)tkerr(crtTk, "break without for or while");
            addInstrA(O_JMP, crtLoopEnd);
            return 1;
        }
        else tkerr(crtTk, "missing ; after BREAK");
    }
    else if (consume(RETURN))
    {
        if (expr(&rv))
        {
            i = getRVal(&rv);
            addCastInstr(i, &rv.type, &crtFunc->type);
            if (crtFunc->type.typeBase == TB_VOID)
                tkerr(crtTk, "a void function cannot return a value");
            cast(&crtFunc->type, &rv.type);
        }
        if (consume(SEMICOLON))
        {
            if (crtFunc->type.typeBase == TB_VOID) {
                addInstrII(O_RET, sizeArgs, 0);
            }
            else {
                addInstrII(O_RET, sizeArgs, typeArgSize(&crtFunc->type));
            }
            return 1;
        }
        else tkerr(crtTk, "missing ';' after RETURN");
    }
    else if (expr(&rv))
    {
        if (typeArgSize(&rv.type))addInstrI(O_DROP, typeArgSize(&rv.type));     // <--------------- Am inlocuit rv1 cu rv !!!
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
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

// stmCompound: LACC ( declVar | stm )* RACC ;
int stmCompound()
{
    Token* startTk = crtTk;
    Instr* startLastInstr = lastInstruction;
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
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

// expr: exprAssign ;
int expr(RetVal* rv)
{
    Token* startTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    if (exprAssign(rv))
    {
        return 1;
    }
    crtTk = startTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

// exprAssign: exprUnary ASSIGN exprAssign | exprOr ;
int exprAssign(RetVal* rv)
{
    Token* startTk = crtTk;
    RetVal rve;
    Instr* startLastInstr = lastInstruction;
    Instr* i;
    Instr* oldLastInstr = lastInstruction;
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
                i = getRVal(&rve);
                addCastInstr(i, &rve.type, &rv->type);
                //duplicate the value on top before the dst addr
                addInstrII(O_INSERT, sizeof(void*) + typeArgSize(&rv->type), typeArgSize(&rv->type));
                addInstrI(O_STORE, typeArgSize(&rv->type));
                rv->isCtVal = rv->isLVal = 0;
                return 1;
            }
            else tkerr(crtTk, "invalid expression after '='");
        }
        crtTk = startTk;
        deleteInstructionsAfter(oldLastInstr);
    }    
    if (exprOr(rv))
    {
        return 1;
    }
    crtTk = startTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

// exprOr: exprOr OR exprAnd | exprAnd ;
int exprOrPrim(RetVal* rv)
{
    RetVal rve;
    Instr* i1, * i2; Type t, t1, t2;
    if (consume(OR))
    {
        i1 = rv->type.nElements < 0 ? getRVal(rv) : lastInstruction;
        t1 = rv->type;
        if (exprAnd(&rve))
        {
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be logically tested");
            if (rv->type.nElements >= 0)        // vectors 
            {      
                addInstr(O_OR_A);
            }
            else        // non-vectors
            {                  
                i2 = getRVal(&rve); t2 = rve.type;
                t = getArithType(&t1, &t2);
                addCastInstr(i1, &t1, &t);
                addCastInstr(i2, &t2, &t);
                switch (t.typeBase) 
                {
                    case TB_INT:addInstr(O_OR_I); break;
                    case TB_DOUBLE:addInstr(O_OR_D); break;
                    case TB_CHAR:addInstr(O_OR_C); break;
                }
            }
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
    Instr* startLastInstr = lastInstruction;
    if (exprAnd(rv))
    {
        if (exprOrPrim(rv))
        {
            return 1;
        }
    }
    crtTk = startTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

// exprAnd: exprAnd AND exprEq | exprEq ;
int exprAndPrim(RetVal* rv)
{
    RetVal rve;
    Instr* i1, * i2; Type t, t1, t2;
    if (consume(AND))
    {
        i1 = rv->type.nElements < 0 ? getRVal(rv) : lastInstruction;
        t1 = rv->type;
        if (exprEq(&rve))
        {
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be logically tested");
            if (rv->type.nElements >= 0)        // vectors
            {      
                addInstr(O_AND_A);
            }
            else        // non-vectors 
            {  
                i2 = getRVal(&rve); t2 = rve.type;
                t = getArithType(&t1, &t2);
                addCastInstr(i1, &t1, &t);
                addCastInstr(i2, &t2, &t);
                switch (t.typeBase) 
                {
                    case TB_INT:addInstr(O_AND_I); break;
                    case TB_DOUBLE:addInstr(O_AND_D); break;
                    case TB_CHAR:addInstr(O_AND_C); break;
                }
            }
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
    Instr* startLastInstr = lastInstruction;
    if (exprEq(rv))
    {
        if (exprAndPrim(rv))
        {
            return 1;
        }
    }
    crtTk = startTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

// exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel ;
int exprEqPrim(RetVal* rv)
{
    RetVal rve;
    Instr* i1, * i2; Type t, t1, t2;
    if (consume(EQUAL) || consume(NOTEQ))
    {
        Token* tkop = consumedTk;
        i1 = rv->type.nElements < 0 ? getRVal(rv) : lastInstruction;
        t1 = rv->type;
        if (exprRel(&rve))
        {
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be compared");
            if (rv->type.nElements >= 0)       // vectors
            {
                addInstr(tkop->code == EQUAL ? O_EQ_A : O_NOTEQ_A);
            }
            else      // non-vectors
            {
                i2 = getRVal(&rve); t2 = rve.type;
                t = getArithType(&t1, &t2);
                addCastInstr(i1, &t1, &t);
                addCastInstr(i2, &t2, &t);
                if (tkop->code == EQUAL) 
                {
                    switch (t.typeBase) 
                    {
                        case TB_INT:addInstr(O_EQ_I); break;
                        case TB_DOUBLE:addInstr(O_EQ_D); break;
                        case TB_CHAR:addInstr(O_EQ_C); break;
                    }
                }
                else 
                {
                    switch (t.typeBase) 
                    {
                        case TB_INT:addInstr(O_NOTEQ_I); break;
                        case TB_DOUBLE:addInstr(O_NOTEQ_D); break;
                        case TB_CHAR:addInstr(O_NOTEQ_C); break;
                    }
                }
            }
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
    Instr* startLastInstr = lastInstruction;
    if (exprRel(rv))
    {
        if (exprEqPrim(rv))
        {
            return 1;
        }
    }
    crtTk = startTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

// exprRel: exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd ;
int exprRelPrim(RetVal* rv)
{
    RetVal rve;
    Instr* i1, * i2; Type t, t1, t2;
    if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ))
    {
        Token* tkop = consumedTk;
        i1 = getRVal(rv); t1 = rv->type;
        if (exprAdd(&rve))
        {
            if (rv->type.nElements > -1 || rve.type.nElements > -1)
                tkerr(crtTk, "an array cannot be compared");
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be compared");
            i2 = getRVal(&rve); t2 = rve.type;
            t = getArithType(&t1, &t2);
            addCastInstr(i1, &t1, &t);
            addCastInstr(i2, &t2, &t);
            switch (tkop->code) 
            {
            case LESS:
                switch (t.typeBase)
                {
                    case TB_INT:addInstr(O_LESS_I); break;
                    case TB_DOUBLE:addInstr(O_LESS_D); break;
                    case TB_CHAR:addInstr(O_LESS_C); break;
                }
                break;
            case LESSEQ:
                switch (t.typeBase) 
                {
                    case TB_INT:addInstr(O_LESSEQ_I); break;
                    case TB_DOUBLE:addInstr(O_LESSEQ_D); break;
                    case TB_CHAR:addInstr(O_LESSEQ_C); break;
                }
                break;
            case GREATER:
                switch (t.typeBase) 
                {
                    case TB_INT:addInstr(O_GREATER_I); break;
                    case TB_DOUBLE:addInstr(O_GREATER_D); break;
                    case TB_CHAR:addInstr(O_GREATER_C); break;
                }
                break;
            case GREATEREQ:
                switch (t.typeBase) 
                {
                    case TB_INT:addInstr(O_GREATEREQ_I); break;
                    case TB_DOUBLE:addInstr(O_GREATEREQ_D); break;
                    case TB_CHAR:addInstr(O_GREATEREQ_C); break;
                }
                break;
            }
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
    Instr* startLastInstr = lastInstruction;
    if (exprAdd(rv))
    {
        if (exprRelPrim(rv))
        {
            return 1;
        }
    }
    crtTk = startTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

// exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul ;
int exprAddPrim(RetVal* rv)
{
    RetVal rve;
    Instr* i1, * i2; Type t1, t2;
    if (consume(ADD) || consume(SUB))
    {
        Token* tkop = consumedTk;
        i1 = getRVal(rv); t1 = rv->type;
        if (exprMul(&rve))
        {
            if (rv->type.nElements > -1 || rve.type.nElements > -1)
                tkerr(crtTk, "an array cannot be added or subtracted");
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be added or subtracted");
            rv->type = getArithType(&rv->type, &rve.type);
            i2 = getRVal(&rve); t2 = rve.type;
            addCastInstr(i1, &t1, &rv->type);
            addCastInstr(i2, &t2, &rv->type);
            if (tkop->code == ADD)
            {
                switch (rv->type.typeBase) 
                {
                    case TB_INT:addInstr(O_ADD_I); break;
                    case TB_DOUBLE:addInstr(O_ADD_D); break;
                    case TB_CHAR:addInstr(O_ADD_C); break;
                }
            }
            else 
            {
                switch (rv->type.typeBase) 
                {
                    case TB_INT:addInstr(O_SUB_I); break;
                    case TB_DOUBLE:addInstr(O_SUB_D); break;
                    case TB_CHAR:addInstr(O_SUB_C); break;
                }
            }
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
    Instr* startLastInstr = lastInstruction;
    if (exprMul(rv))
    {
        if (exprAddPrim(rv))
        {
            return 1;
        }
    }
    crtTk = startTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

// exprMul: exprMul ( MUL | DIV ) exprCast | exprCast ;
int exprMulPrim(RetVal* rv)
{
    RetVal rve;
    Instr* i1, * i2; Type t1, t2;
    if (consume(MUL) || consume(DIV))
    {
        Token* tkop = consumedTk;
        i1 = getRVal(rv); t1 = rv->type;
        if (exprCast(&rve))
        {
            if (rv->type.nElements > -1 || rve.type.nElements > -1)
                tkerr(crtTk, "an array cannot be multiplied or divided");
            if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
                tkerr(crtTk, "a structure cannot be multiplied or divided");
            rv->type = getArithType(&rv->type, &rve.type);
            i2 = getRVal(&rve); t2 = rve.type;
            addCastInstr(i1, &t1, &rv->type);
            addCastInstr(i2, &t2, &rv->type);
            if (tkop->code == MUL) 
            {
                switch (rv->type.typeBase)
                {
                    case TB_INT:addInstr(O_MUL_I); break;
                    case TB_DOUBLE:addInstr(O_MUL_D); break;
                    case TB_CHAR:addInstr(O_MUL_C); break;
                }
            }
            else 
            {
                switch (rv->type.typeBase) 
                {
                    case TB_INT:addInstr(O_DIV_I); break;
                    case TB_DOUBLE:addInstr(O_DIV_D); break;
                    case TB_CHAR:addInstr(O_DIV_C); break;
                }
            }
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
    Instr* startLastInstr = lastInstruction;
    if (exprCast(rv))
    {
        if (exprMulPrim(rv))
        {
            return 1;
        }
    }
    crtTk = startTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

// exprCast: LPAR typeName RPAR exprCast | exprUnary ;
int exprCast(RetVal* rv)
{
    Token* startTk = crtTk;
    Type t;
    RetVal rve;
    Instr* startLastInstr = lastInstruction;
    Instr* oldLastInstr = lastInstruction;
    if (consume(LPAR))
    {
        if (typeName(&t))
        {
            if (consume(RPAR))
            {
                if (exprCast(&rve))
                {
                    cast(&t, &rve.type);
                    if (rv->type.nElements < 0 && rv->type.typeBase != TB_STRUCT) 
                    {
                        switch (rve.type.typeBase) 
                        {
                        case TB_CHAR:
                            switch (t.typeBase) 
                            {
                                case TB_INT:addInstr(O_CAST_C_I); break;
                                case TB_DOUBLE:addInstr(O_CAST_C_D); break;
                            }
                            break;
                        case TB_DOUBLE:
                            switch (t.typeBase) 
                            {
                                case TB_CHAR:addInstr(O_CAST_D_C); break;
                                case TB_INT:addInstr(O_CAST_D_I); break;
                            }
                            break;
                        case TB_INT:
                            switch (t.typeBase) 
                            {
                                case TB_CHAR:addInstr(O_CAST_I_C); break;
                                case TB_DOUBLE:addInstr(O_CAST_I_D); break;
                            }
                            break;
                        }
                    }
                    rv->type = t;
                    rv->isCtVal = rv->isLVal = 0;
                    return 1;
                }
            }
            else tkerr(crtTk, "invalid type casting or missing ')'");
        }
    }
    deleteInstructionsAfter(oldLastInstr);
    if (exprUnary(rv))
    {
        return 1;
    }
    crtTk = startTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix ;
int exprUnary(RetVal* rv)
{
    Token* startTk = crtTk;
    Instr* startLastInstr = lastInstruction;
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
                getRVal(rv);
                switch (rv->type.typeBase) 
                {
                    case TB_CHAR:addInstr(O_NEG_C); break;
                    case TB_INT:addInstr(O_NEG_I); break;
                    case TB_DOUBLE:addInstr(O_NEG_D); break;
                }
            }
            else {  // NOT
                if (rv->type.typeBase == TB_STRUCT)
                    tkerr(crtTk, "'!' cannot be applied to a struct");
                if (rv->type.nElements < 0) 
                {
                    getRVal(rv);
                    switch (rv->type.typeBase) 
                    {
                        case TB_CHAR:addInstr(O_NOT_C); break;
                        case TB_INT:addInstr(O_NOT_I); break;
                        case TB_DOUBLE:addInstr(O_NOT_D); break;
                    }
                }
                else {
                    addInstr(O_NOT_A);
                }
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
    deleteInstructionsAfter(startLastInstr);
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
                addCastInstr(lastInstruction, &rve.type, &typeInt);
                getRVal(&rve);
                if (typeBaseSize(&rv->type) != 1) 
                {
                    addInstrI(O_PUSHCT_I, typeBaseSize(&rv->type));
                    addInstr(O_MUL_I);
                }
                addInstr(O_OFFSET);
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
            if (sMember->offset) 
            {
                addInstrI(O_PUSHCT_I, sMember->offset);
                addInstr(O_OFFSET);
            }
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
    Instr* startLastInstr = lastInstruction;
    if (exprPrimary(rv))
    {
        if (exprPostfixPrim(rv))
        {
            return 1;
        }
    }
    crtTk = startTk;
    deleteInstructionsAfter(startLastInstr);
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
    Instr* startLastInstr = lastInstruction;
    Instr* i;
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
                if ((*crtDefArg)->type.nElements < 0)  //only arrays are passed by addr
                {
                    i = getRVal(&arg);
                }
                else 
                {
                    i = lastInstruction;
                }
                addCastInstr(i, &arg.type, &(*crtDefArg)->type);
                crtDefArg++;
                for (;;)
                {
                    if (consume(COMMA))
                    {
                        if (expr(&arg)) 
                        {
                            if (crtDefArg == s->args.end)
                                tkerr(crtTk, "too many arguments in call");
                            cast(&(*crtDefArg)->type, &arg.type);
                            if ((*crtDefArg)->type.nElements < 0) 
                            {
                                i = getRVal(&arg);
                            }
                            else 
                            {
                                i = lastInstruction;
                            }
                            addCastInstr(i, &arg.type, &(*crtDefArg)->type);
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
                // function call
                i = addInstr(s->cls == CLS_FUNC ? O_CALL : O_CALLEXT);
                i->args[0].addr = s->addr;
                return 1;
            }
            else tkerr(crtTk, "invalid argument or missing ')'");
        }
        else
        {
            if (s->cls == CLS_FUNC || s->cls == CLS_EXTFUNC)
                tkerr(crtTk, "missing call for function %s", tkName->text);
            // variable
            if (s->depth) 
            {
                addInstrI(O_PUSHFPADDR, s->offset);
            }
            else 
            {
                addInstrA(O_PUSHCT_A, s->addr);
            }
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
        addInstrI(O_PUSHCT_I, tki->i);
        return 1;
    }
    else if (consume(CT_REAL))
    {
        Token* tkr = consumedTk;
        rv->type = createType(TB_DOUBLE, -1);
        rv->ctVal.d = tkr->r;
        rv->isCtVal = 1;
        rv->isLVal = 0;
        i = addInstr(O_PUSHCT_D); i->args[0].d = tkr->r;
        return 1;
    }
    else if (consume(CT_CHAR))
    {
        Token* tkc = consumedTk;
        rv->type = createType(TB_CHAR, -1);
        rv->ctVal.i = tkc->i;
        rv->isCtVal = 1;
        rv->isLVal = 0;
        addInstrI(O_PUSHCT_C, tkc->i);
        return 1;
    }
    else if (consume(CT_STRING))
    {
        Token* tks = consumedTk;
        rv->type = createType(TB_CHAR, 0);
        rv->ctVal.str = tks->text;
        rv->isCtVal = 1;
        rv->isLVal = 0;
        addInstrA(O_PUSHCT_A, tks->text);
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
    deleteInstructionsAfter(startLastInstr);
    return 0;
}