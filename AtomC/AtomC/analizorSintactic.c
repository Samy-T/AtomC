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
            if (consume(LACC))
            {
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
    if (typeBase())
    {
        if (consume(ID))
        {
            isDV = arrayDecl();
            for (;;)
            {
                if (consume(COMMA))
                {
                    isDV = 1;
                    if (consume(ID))
                    {
                        arrayDecl();
                    }
                    else tkerr(crtTk, "expected variable name after ','");
                }
                else break;
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
int typeBase()
{
    Token* startTk = crtTk;
    if (consume(INT)) {
        return 1;
    }
    else if (consume(DOUBLE))
    {
        return 1;
    }
    else if (consume(CHAR))
    {
        return 1;
    }
    else if (consume(STRUCT)) {
        if (consume(ID)) {
            return 1;
        }
        else tkerr(crtTk, "missing struct name after STRUCT");
        crtTk = startTk;
    }
    return 0;
}

// arrayDecl: LBRACKET expr? RBRACKET ;
int arrayDecl()
{
    Token* startTk = crtTk;
    if (consume(LBRACKET)) {
        expr();
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
int typeName()
{
    if (typeBase())
    {
        arrayDecl();
        return 1;
    }
    return 0;
}

/* declFuncPrim: ID
                 LPAR ( funcArg ( COMMA funcArg )* )? RPAR
                 stmCompound ; */
int declFuncPrim()
{
    if (consume(ID))
    {
        if (consume(LPAR))
        {
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
                if (stmCompound())
                {
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
    if (typeBase())
    {
        consume(MUL);
        if (declFuncPrim())
        {
            return 1;
        }
    }
    else if (consume(VOID))
    {
        if (declFuncPrim())
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
    if (typeBase())
    {
        if (consume(ID))
        {
            arrayDecl();
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
    if (consume(LPAR))
    {
        if (expr())
        {
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
    if (consume(LPAR))
    {
        if (expr())
        {
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
    if (consume(LPAR))
    {
        expr();
        if (consume(SEMICOLON))
        {
            expr();
            if (consume(SEMICOLON))
            {
                expr();
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
        expr();
        if (consume(SEMICOLON))
        {
            return 1;
        }
        else tkerr(crtTk, "missing ';' after RETURN");
    }
    else if (expr())
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
    if (consume(LACC))
    {
        for (;;)
        {
            if (declVar()) {
            }
            else if (stm()) {
            }
            else break;
        }
        if (consume(RACC)) {
            return 1;
        }
        else tkerr(crtTk, "missing '}'");
    }
    crtTk = startTk;
    return 0;
}

// expr: exprAssign ;
int expr()
{
    Token* startTk = crtTk;
    if (exprAssign())
    {
        return 1;
    }
    crtTk = startTk;
    return 0;
}

// exprAssign: exprUnary ASSIGN exprAssign | exprOr ;
int exprAssign()
{
    Token* startTk = crtTk;

    if (exprUnary())             // exprUnary() este un subset al lui exprOr()
    {
        if (consume(ASSIGN))
        {
            if (exprAssign())
            {
                return 1;
            }
            else tkerr(crtTk, "invalid expression after '='");
        }
        crtTk = startTk;
    }
    if (exprOr())
    {
        return 1;
    }
    crtTk = startTk;
    return 0;
}

// exprOr: exprOr OR exprAnd | exprAnd ;
int exprOrPrim()
{
    if (consume(OR))
    {
        if (exprAnd())
        {
            if (exprOrPrim()) {
                return 1;
            }
        }
    }
    return 1;
}

int exprOr()
{
    Token* startTk = crtTk;
    if (exprAnd())
    {
        if (exprOrPrim())
        {
            return 1;
        }
    }
    crtTk = startTk;
    return 0;
}

// exprAnd: exprAnd AND exprEq | exprEq ;
int exprAndPrim()
{
    if (consume(AND))
    {
        if (exprEq())
        {
            if (exprAndPrim())
            {
                return 1;
            }
        }
    }
    return 1;
}

int exprAnd()
{
    Token* startTk = crtTk;
    if (exprEq())
    {
        if (exprAndPrim())
        {
            return 1;
        }
    }
    crtTk = startTk;
    return 0;
}

// exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel ;
int exprEqPrim()
{
    if (consume(EQUAL) || consume(NOTEQ))
    {
        if (exprRel())
        {
            if (exprEqPrim())
            {
                return 1;
            }
        }
    }
    return 1;
}

int exprEq()
{
    Token* startTk = crtTk;
    if (exprRel())
    {
        if (exprEqPrim())
        {
            return 1;
        }
    }
    crtTk = startTk;
    return 0;
}

// exprRel: exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd ;
int exprRelPrim()
{
    if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ))
    {
        if (exprAdd())
        {
            if (exprRelPrim())
            {
                return 1;
            }
        }
    }
    return 1;
}

int exprRel()
{
    Token* startTk = crtTk;
    if (exprAdd())
    {
        if (exprRelPrim())
        {
            return 1;
        }
    }
    crtTk = startTk;
    return 0;
}

// exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul ;
int exprAddPrim()
{
    if (consume(ADD) || consume(SUB))
    {
        if (exprMul())
        {
            if (exprAddPrim())
            {
                return 1;
            }
        }
    }
    return 1;
}

int exprAdd()
{
    Token* startTk = crtTk;
    if (exprMul())
    {
        if (exprAddPrim())
        {
            return 1;
        }
    }
    crtTk = startTk;
    return 0;
}

// exprMul: exprMul ( MUL | DIV ) exprCast | exprCast ;
int exprMulPrim()
{
    if (consume(MUL) || consume(DIV))
    {
        if (exprCast())
        {
            if (exprMulPrim())
            {
                return 1;
            }
        }
    }
    return 1;
}

int exprMul()
{
    Token* startTk = crtTk;
    if (exprCast())
    {
        if (exprMulPrim())
        {
            return 1;
        }
    }
    crtTk = startTk;
    return 0;
}

// exprCast: LPAR typeName RPAR exprCast | exprUnary ;
int exprCast()
{
    Token* startTk = crtTk;
    if (consume(LPAR))
    {
        if (typeName())
        {
            if (consume(RPAR))
            {
                if (exprCast())
                {
                    return 1;
                }
            }
            else tkerr(crtTk, "invalid type casting or missing ')'");
        }
    }
    else if (exprUnary())
    {
        return 1;
    }
    crtTk = startTk;
    return 0;
}

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix ;
int exprUnary()
{
    Token* startTk = crtTk;
    if (consume(SUB) || consume(NOT))
    {
        if (exprUnary())
        {
            return 1;
        }
        else tkerr(crtTk, "missing unary expression");
    }
    else if (exprPostfix())
    {
        return 1;
    }
    crtTk = startTk;
    return 0;
}

/* exprPostfix: exprPostfix LBRACKET expr RBRACKET
                            | exprPostfix DOT ID
                            | exprPrimary ; */
int exprPostfixPrim()
{
    if (consume(LBRACKET))
    {
        if (expr())
        {
            if (consume(RBRACKET))
            {
                if (exprPostfixPrim())
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
            if (exprPostfixPrim())
            {
                return 1;
            }
        }
        else tkerr(crtTk, "invalid or missing ID after '.'");
    }
    return 1;
}

int exprPostfix()
{
    Token* startTk = crtTk;
    if (exprPrimary())
    {
        if (exprPostfixPrim())
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
int exprPrimary()
{
    Token* startTk = crtTk;
    if (consume(ID))
    {
        if (consume(LPAR))
        {
            if (expr())
            {
                for (;;)
                {
                    if (consume(COMMA))
                    {
                        if (expr()) {
                            return 1;
                        }
                        else tkerr(crtTk, "expected expression after ','");
                    }
                    else break;
                }
            }
            if (consume(RPAR))
            {
                return 1;
            }
            else tkerr(crtTk, "invalid argument or missing ')'");
        }
        return 1;
    }
    else if (consume(CT_INT))
    {
        return 1;
    }
    else if (consume(CT_REAL))
    {
        return 1;
    }
    else if (consume(CT_CHAR))
    {
        return 1;
    }
    else if (consume(CT_STRING))
    {
        return 1;
    }
    else if (consume(LPAR))
    {
        if (expr())
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