#include "main.h"

// This file contains ANALIZORUL LEXICAL

const char* denumiri[] = {
        "ID",
        "END",
        "CT_INT",
        "CT_REAL",
        "CT_CHAR",
        "CT_STRING",
        "COMMA",
        "SEMICOLON",
        "LPAR",
        "RPAR",
        "LBRACKET",
        "RBRACKET",
        "LACC",
        "RACC",
        "ADD",
        "SUB",
        "MUL",
        "DIV",
        "DOT",
        "AND",
        "OR",
        "NOT",
        "NOTEQ",
        "EQUAL",
        "ASSIGN",
        "LESS",
        "LESSEQ",
        "GREATER",
        "GREATEREQ",
        "BREAK",
        "CHAR",
        "DOUBLE",
        "ELSE",
        "FOR",
        "IF",
        "INT",
        "RETURN",
        "STRUCT",
        "VOID",
        "WHILE"
};

Token* tokens = NULL;       // inceputul liste de AL
Token* lastToken = NULL;    // pointeaza spre ultimul AL din lista
int line = 1;               // line incepe de la 1
char* pch;

void err(const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, va);
    fputc('\n', stderr);
    va_end(va);
    exit(-1);
}

void tkerr(const Token* tk, const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    fprintf(stderr, "error in line %d: ", tk->line);
    vfprintf(stderr, fmt, va);
    fputc('\n', stderr);
    va_end(va);
    exit(-1);
}

Token* addTk(int code)
{
    Token* tk;
    SAFEALLOC(tk, Token); // aloca memorie pentru un tip dat (in cazul nostru, Token)
    if (tk != 0)
    {
        tk->code = code;
        tk->line = line;
        tk->next = NULL;
    }
    if (lastToken)
    {
        lastToken->next = tk;
    }
    else
    {
        tokens = tk;
    }
    lastToken = tk;

    return tk;
}

char escapeSequences(char c) // Aceasta functie este folosita pentru a prelua corect secventele escape
{
    switch (c)
    {
    case 'a':
        return '\a';
    case 'b':
        return '\b';
    case 'f':
        return '\f';
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    case 'v':
        return '\v';
    case '\'':
        return '\'';
    case '?':
        return '\?';
    case '\"':
        return '\"';
    case '\\':
        return '\\';
    case '0':
        return '\0';
    }
    return '\0';
}

char* createString(char* start, char* stop)
{

    int string_length = stop - start;
    char* new_string = (char*)malloc(string_length + 1); //Aloc cu 1 mai mult pentru a face loc si terminatorului de sir
    int i = 0;

    if (new_string != NULL)
    {
        for (int j = 0; j < string_length; j++)
        {
            if (start[j] == '\\') // In cazul in care avem secvente escape
            {
                j++;
                if (start[j] == 'a' || start[j] == 'b' || start[j] == 'f' || start[j] == 'n' || start[j] == 'r' || start[j] == 't' || start[j] == 'v' || start[j] == '\'' || start[j] == '?' || start[j] == '\"' || start[j] == '\\' || start[j] == '0')
                    new_string[i++] = escapeSequences(start[j]);
            }
            else
            {
                new_string[i++] = start[j];
            }
        }
        new_string[i] = '\0';
        return new_string;
    }
    return NULL;
}

/// Analizorul lexical (ALEX)
int getNextTk()
{
    int state = 0;      // starea initiala = 0
    int base;           // variabila utilizata pentru precizarea bazei in care este reprezentat un int
    char* start = NULL; // inceputul un AL
    Token* tk;
    SAFEALLOC(tk, Token);
    tk->line = line;

    for (;;)
    {
        char ch = *pch; // ch pointeaza spre primul caracter din fisier
        switch (state)
        {
        case 0:
            if ('1' <= ch && ch <= '9')
            {
                start = pch;
                state = 1;
                pch++;
            }
            else if (ch == '0')
            {
                start = pch;
                state = 3;
                pch++;
            }
            else if (ch == '\'')
            {
                start = pch;
                state = 14;
                pch++;
            }
            else if (ch == '\"')
            {
                start = pch;
                state = 19;
                pch++;
            }
            else if (ch == ',')
            {
                state = 23;
                pch++;
            }
            else if (ch == ';')
            {
                state = 24;
                pch++;
            }
            else if (ch == '(')
            {
                state = 25;
                pch++;
            }
            else if (ch == ')')
            {
                state = 26;
                pch++;
            }
            else if (ch == '[')
            {
                state = 27;
                pch++;
            }
            else if (ch == ']')
            {
                state = 28;
                pch++;
            }
            else if (ch == '{')
            {
                state = 29;
                pch++;
            }
            else if (ch == '}')
            {
                state = 30;
                pch++;
            }
            else if (ch == '+')
            {
                state = 31;
                pch++;
            }
            else if (ch == '-')
            {
                state = 32;
                pch++;
            }
            else if (ch == '*')
            {
                state = 33;
                pch++;
            }
            else if (ch == '/')
            {
                state = 34;
                pch++;
            }
            else if (ch == '.')
            {
                state = 36;
                pch++;
            }
            else if (ch == '&')
            {
                state = 37;
                pch++;
            }
            else if (ch == '|')
            {
                state = 39;
                pch++;
            }
            else if (ch == '!')
            {
                state = 41;
                pch++;
            }
            else if (ch == '=')
            {
                state = 43;
                pch++;
            }
            else if (ch == '<')
            {
                state = 46;
                pch++;
            }
            else if (ch == '>')
            {
                state = 49;
                pch++;
            }
            else if (ch == '\0')
            {
                state = 56;
                pch++;
            }
            else if (isalpha(ch) || ch == '_')
            {
                start = pch;
                state = 57;
                pch++;
            }
            else if (ch == ' ' || ch == '\r' || ch == '\t')
            {
                pch++;
            }
            else if (ch == '\n')
            {
                line++;
                tk->line = line; // in caz ca in fisier exista doar un comentariu neterminat
                pch++;
            }
            else
            {
                tkerr(tk, "Eroare caz 0!\n");
            }
            break;

        case 1:
            if (isdigit(ch))
            {
                pch++;
            }
            else if (ch == '.')
            {
                state = 7;
                pch++;
            }
            else if (ch == 'e' || ch == 'E')
            {
                state = 11;
                pch++;
            }
            else
            {
                base = 10;
                state = 2;
            }
            break;

        case 2:
            tk = addTk(CT_INT);
            tk->i = strtol(createString(start, pch), NULL, base); // base reprezinta baza in care este reprezentata numarul: 10 (zecimal), 8 (octal), 16 (hexa)
            return CT_INT;

        case 3:
            if (ch == '8' || ch == '9')
            {
                state = 8;
                pch++;
            }
            else if (ch == 'x' || ch == 'X')
            {
                state = 5;
                pch++;
            }
            else
            {
                state = 4;
            }
            break;

        case 4:
            if ('0' <= ch && ch <= '7')
            {
                pch++;
            }
            else if (ch == '.')
            {
                state = 7;
                pch++;
            }
            else if (ch == '8' || ch == '9')
            {
                state = 8;
                pch++;
            }
            else if (ch == 'e' || ch == 'E')
            {
                state = 11;
                pch++;
            }
            else
            {
                base = 8;
                state = 2;
            }
            break;

        case 5:
            if (('0' <= ch && ch <= '9') || ('a' <= ch && ch <= 'f') || ('A' <= ch && ch <= 'F'))
            {
                state = 6;
                pch++;
            }
            else
            {
                tkerr(tk, "Eroare caz 5!\n");
            }
            break;

        case 6:
            if (('0' <= ch && ch <= '9') || ('a' <= ch && ch <= 'f') || ('A' <= ch && ch <= 'F'))
            {
                pch++;
            }
            else
            {
                base = 16;
                state = 2;
            }
            break;

        case 7:
            if (isdigit(ch))
            {
                pch++;
                state = 9;
            }
            else
            {
                tkerr(tk, "Eroare caz 7!\n");
            }
            break;

        case 8:
            if (isdigit(ch))
            {
                pch++;
            }
            else if (ch == '.')
            {
                state = 7;
                pch++;
            }
            else if (ch == 'e' || ch == 'E')
            {
                state = 11;
                pch++;
            }
            else
            {
                tkerr(tk, "Eroare caz 8!\n");
            }
            break;

        case 9:
            if (isdigit(ch))
            {
                pch++;
            }
            else if (ch == 'e' || ch == 'E')
            {
                state = 11;
                pch++;
            }
            else
            {
                state = 10;
            }
            break;

        case 10:
            tk = addTk(CT_REAL);
            tk->r = atof(createString(start, pch));
            return CT_REAL;

        case 11:
            if (isdigit(ch))
            {
                state = 12;
                pch++;
            }
            else if (ch == '+' || ch == '-')
            {
                state = 13;
                pch++;
            }
            else
            {
                tkerr(tk, "Eroare caz 11!\n");
            }
            break;

        case 12:
            if (isdigit(ch))
            {
                pch++;
            }
            else
            {
                state = 10;
            }
            break;

        case 13:
            if (isdigit(ch))
            {
                state = 12;
                pch++;
            }
            else
            {
                tkerr(tk, "Eroare caz 13!\n");
            }
            break;

        case 14:
            if (ch == '\\')
            {
                state = 15;
                pch++;
            }
            else if (ch != '\'') // ch!= '\\'
            {
                state = 17;
                pch++;
            }
            else
            {
                tkerr(tk, "Eroare la cazul 14!\n");
            }
            break;

        case 15:
            if (ch == 'a' || ch == 'b' || ch == 'f' || ch == 'n' || ch == 'r' || ch == 't' || ch == 'v' || ch == '\'' || ch == '?' || ch == '\"' || ch == '\\' || ch == '\0')
            {
                state = 16;
                pch++;
            }
            else
            {
                tkerr(tk, "Eroare la cazul 15!\n");
            }
            break;

        case 16:
            if (ch == '\'')
            {
                state = 18;
                pch++;
            }
            else
            {
                tkerr(tk, "Eroare la cazul 16!\n");
            }
            break;

        case 17:
            if (ch == '\'')
            {
                state = 18;
                pch++;
            }
            else
            {
                tkerr(tk, "Eroare la cazul 17!\n");
            }
            break;

        case 18:
            tk = addTk(CT_CHAR);
            char* CT_CHAR_string = createString(start, pch); // Se formeaza sirul de caractere "'c'" unde c poate sa fie orice caracter.
            tk->i = CT_CHAR_string[1];                       // Extragem caracterul care se afla pe pozitia 1 a sirului de caractere format
            free(CT_CHAR_string);                            // Se elibereaza zona de memorie pentru sirul format
            return CT_CHAR;

        case 19:
            if (ch == '\\')
            {
                state = 20;
                pch++;
            }
            else if (ch == '\"')
            {
                state = 22;
                pch++;
            }
            else
            {
                pch++;
            }
            break;

        case 20:
            if (ch == 'a' || ch == 'b' || ch == 'f' || ch == 'n' || ch == 'r' || ch == 't' || ch == 'v' || ch == '\'' || ch == '?' || ch == '\"' || ch == '\\' || ch == '\0')
            {
                state = 21;
                pch++;
            }
            else
            {
                tkerr(tk, "Eroare la cazul 20!\n");
            }
            break;

        case 21:
            if (ch == '\"')
            {
                state = 22;
                pch++;
            }
            else
            {
                state = 19;
            }
            break;

        case 22:
            tk = addTk(CT_STRING);
            tk->text = createString(start, pch);
            return CT_STRING;

        case 23:
            addTk(COMMA);
            return COMMA;

        case 24:
            addTk(SEMICOLON);
            return SEMICOLON;

        case 25:
            addTk(LPAR);
            return LPAR;

        case 26:
            addTk(RPAR);
            return RPAR;

        case 27:
            addTk(LBRACKET);
            return LBRACKET;
        case 28:
            addTk(RBRACKET);
            return RBRACKET;

        case 29:
            addTk(LACC);
            return LACC;
        case 30:
            addTk(RACC);
            return RACC;

        case 31:
            addTk(ADD);
            return ADD;

        case 32:
            addTk(SUB);
            return SUB;

        case 33:
            addTk(MUL);
            return MUL;

        case 34:
            if (ch == '/')
            {
                state = 52;
                pch++;
            }
            else if (ch == '*')
            {
                state = 53;
                pch++;
            }
            else
            {
                state = 35;
            }
            break;

        case 35:
            addTk(DIV);
            return DIV;

        case 36:
            addTk(DOT);
            return DOT;

        case 37:
            if (ch == '&')
            {
                state = 38;
                pch++;
            }
            else
            {
                tkerr(tk, "Eroare la cazul 37!\n");
            }

        case 38:
            addTk(AND);
            return AND;

        case 39:
            if (ch == '|')
            {
                state = 40;
                pch++;
            }
            else
            {
                tkerr(tk, "Eroare la cazul 39!\n");
            }
            break;

        case 40:
            addTk(OR);
            return OR;

        case 41:
            if (ch == '=')
            {
                state = 55;
                pch++;
            }
            else
            {
                state = 42;
            }
            break;

        case 42:
            addTk(NOT);
            return NOT;

        case 43:
            if (ch == '=')
            {
                state = 44;
                pch++;
            }
            else
            {
                state = 45;
            }
            break;

        case 44:
            addTk(EQUAL);
            return EQUAL;

        case 45:
            addTk(ASSIGN);
            return ASSIGN;

        case 46:
            if (ch == '=')
            {
                state = 48;
                pch++;
            }
            else
            {
                state = 47;
            }
            break;

        case 47:
            addTk(LESS);
            return LESS;

        case 48:
            addTk(LESSEQ);
            return LESSEQ;

        case 49:
            if (ch == '=')
            {
                state = 51;
                pch++;
            }
            else
            {
                state = 50;
            }
            break;

        case 50:
            addTk(GREATER);
            return GREATER;

        case 51:
            addTk(GREATEREQ);
            return GREATEREQ;

        case 52:
            if (ch != '\n' && ch != '\r' && ch != '\0')
            {
                pch++;
            }
            else
            {
                state = 0;
            }
            break;

        case 53:
            if (ch == '\n') //in caz ca avem enter intr-un comentariu
            {
                pch++;
                line++;
            }
            else if (ch == '*')
            {
                state = 54;
                pch++;
            }
            else if (ch == '\0')
            {
                tkerr(tk, "Comentariul trebuie inchis la sfarsitul fisierului!\n");
            }
            else
            {
                pch++;
            }
            break;

        case 54:
            if (ch == '*')
            {
                pch++;
            }
            else if (ch == '/')
            {
                state = 0;
                pch++;
            }
            else
            {
                state = 53;
                pch++;
            }
            break;

        case 55:
            addTk(NOTEQ);
            return NOTEQ;

        case 56:
            addTk(END);
            return END;

        case 57:
            if (isalnum(ch) || ch == '_')
            {
                pch++;
            }
            else
            {
                state = 58;
            }
            break;

        case 58:
        {
            int n = pch - start;
            if ((n == 2) && !memcmp("if", start, n))
            {
                tk = addTk(IF);
            }
            else if ((n == 3) && !memcmp("int", start, n))
            {
                tk = addTk(INT);
            }
            else if ((n == 3) && !memcmp("for", start, n))
            {
                tk = addTk(FOR);
            }
            else if ((n == 4) && !memcmp("char", start, n))
            {
                tk = addTk(CHAR);
            }
            else if ((n == 4) && !memcmp("else", start, n))
            {
                tk = addTk(ELSE);
            }
            else if ((n == 4) && !memcmp("void", start, n))
            {
                tk = addTk(VOID);
            }
            else if ((n == 5) && !memcmp("break", start, n))
            {
                tk = addTk(BREAK);
            }
            else if ((n == 5) && !memcmp("while", start, n))
            {
                tk = addTk(WHILE);
            }
            else if ((n == 6) && !memcmp("double", start, n))
            {
                tk = addTk(DOUBLE);
            }
            else if ((n == 6) && !memcmp("return", start, n))
            {
                tk = addTk(RETURN);
            }
            else if ((n == 6) && !memcmp("struct", start, n))
            {
                tk = addTk(STRUCT);
            }
            else
            {
                tk = addTk(ID);
                tk->text = createString(start, pch);
            }
            return tk->code;
        }
        default: err("Stare invalida: %d.\n", state);
        }
    }
}

void showAtoms(Token* tk) // Exemplu afisare:  [ line:    CT_* | ID: atribut ]
{
    while (tk)
    {
        printf("%d:    ", tk->line); // Afiseaza linia
        printf("%s", denumiri[tk->code]);
        switch (tk->code)
        {
        case CT_CHAR:
            printf(": %c", (char)(tk->i));
            break;
        case CT_INT:
            printf(": %ld", tk->i);
            break;
        case CT_STRING:
            printf(": %s", tk->text);
            break;
        case ID:
            printf(": %s", tk->text);
            break;
        case CT_REAL:
            printf(": %lf", tk->r);
        }
        printf("\n");
        tk = tk->next;
    }
}

void terminare(Token* tk) {
    while (tk) {
        if (tk->code == CT_STRING || tk->code == ID) {
            free(tk->text);
        }
        tk = tk->next;
    }
    free(tk);
}