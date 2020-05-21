#include "main.h"

void readFile()
{
    FILE* file_input;
    if (fopen_s(&file_input, "../TestFiles/test.c", "r") != 0)
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
    if (unit())
        printf("Syntax OK\n");
    else
        tkerr(crtTk, "Syntax Error\n");
}

void masinaVirtuala()
{
    //mvTest();
    run(instructions);
}

int main(int argc, char* argv[])
{
    initSymbols(&symbols);
    addExtFuncs();

    readFile();
    analizorLexical();
    analizorSintactic(); 
    masinaVirtuala();

    showAtoms(tokens);
    terminare(tokens);
    return 0;
}