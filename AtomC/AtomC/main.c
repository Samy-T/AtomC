#include "main.h"

void readFile()
{
    FILE* file_input = fopen("../TestFiles/9.c", "r");
    if (file_input== NULL)
    {
        printf("Error with file input!\n");
    }
    char* buf = (char*)malloc(sizeof(char) * 3000);
    int n = fread(buf, 1, 3000, file_input);
    buf[n] = '\0';
    fclose(file_input);
    pch = buf;
}

void analizorLexical()
{
    while (getNextTk() != END);
}

void analizorSintactic()
{
    crtTk = tokens;
    initSymbols(&symbols);
    addExtFuncs();
    if (unit())
        printf("Syntax OK\n");
    else
        tkerr(crtTk, "Syntax Error\n");
}

void masinaVirtuala()
{
    addExtMVFuncs();
    mvTest();
    run(instructions);
}

int main(int argc, char* argv[])
{
    //readFile();
    //analizorLexical();    
    //analizorSintactic();
    //showAtoms(tokens);
    //terminare(tokens);
    masinaVirtuala();
    return 0;
}