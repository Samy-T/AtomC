#include "main.h"

void readFile()
{
    FILE* file_input;
    if (fopen_s(&file_input, "../TestFiles/8.c", "r") != 0)
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

int main(int argc, char* argv[])
{
    readFile();
    analizorLexical();
    showAtoms(tokens);
    terminare(tokens);

    return 0;
}